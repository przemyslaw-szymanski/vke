#if VKE_VULKAN_RENDER_SYSTEM
#include "RenderSystem/Vulkan/Managers/CTextureManager.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/Vulkan/Managers/CDeviceMemoryManager.h"
#include "Core/Managers/CImageManager.h"
#include "RenderSystem/CRenderSystem.h"
#include "CVkEngine.h"
#include "Core/Resources/CImage.h"
#include "RenderSystem/Managers/CStagingBufferManager.h"
#include "RenderSystem/CContextBase.h"
#include "RenderSystem/CTransferContext.h"
#include "Core/Threads/CThreadPool.h"

#define VKE_LOG_TEXTURE_MANAGER 1
#if VKE_LOG_TEXTURE_MANAGER
#define VKE_LOG_TMGR( _msg ) VKE_LOG( _msg )
#else
#define VKE_LOG_TMGR( _msg )
#endif


namespace VKE
{
    namespace RenderSystem
    {

        TEXTURE_ASPECT ConvertTextureUsageToAspect(const TEXTURE_USAGE& usage)
        {
            TEXTURE_ASPECT ret = TextureAspects::UNKNOWN;
            if( usage & TextureUsages::COLOR_RENDER_TARGET ||
                usage & TextureUsages::FILE_IO )
            {
                ret = TextureAspects::COLOR;
            }
            else if( usage & TextureUsages::DEPTH_STENCIL_RENDER_TARGET )
            {
                ret = TextureAspects::DEPTH_STENCIL;
            }

            return ret;
        }

        void IsDepthStencilFormat(const TEXTURE_FORMAT& format, bool* pIsDepthOut, bool* pIsStencilOut)
        {
            *pIsDepthOut = *pIsStencilOut = false;
            switch (format)
            {
                case Formats::D16_UNORM:
                case Formats::D32_SFLOAT:
                case Formats::X8_D24_UNORM_PACK32:
                {
                    *pIsDepthOut = true;
                    break;
                }
                case Formats::D16_UNORM_S8_UINT:
                case Formats::D24_UNORM_S8_UINT:
                case Formats::D32_SFLOAT_S8_UINT:
                {
                    *pIsDepthOut = *pIsStencilOut = true;
                    break;
                }
                case Formats::S8_UINT:
                {
                    *pIsStencilOut = true;
                    break;
                }
            }
        }

        TEXTURE_ASPECT DetermineTextureAspect(const TEXTURE_FORMAT& format)
        {
            bool isDepth, isStencil, isColor;
            IsDepthStencilFormat(format, &isDepth, &isStencil);
            isColor = !isDepth && !isStencil;
            const uint8_t bits = ((uint8_t)isColor << 2) | ((uint8_t)isDepth << 1) | ((uint8_t)isStencil);

            static const TEXTURE_ASPECT aAspects[5] =
            {
                TextureAspects::UNKNOWN, // 000
                TextureAspects::STENCIL, // 001
                TextureAspects::DEPTH, // 010
                TextureAspects::DEPTH_STENCIL, // 011
                TextureAspects::COLOR // 100
            };
            const auto ret = aAspects[bits];
            VKE_ASSERT2(ret != TextureAspects::UNKNOWN, "");
            return ret;
        }

        TEXTURE_VIEW_TYPE DetermineViewType(const STextureDesc& Desc)
        {
            static const TEXTURE_VIEW_TYPE aTypes[2][TextureViewTypes::_MAX_COUNT] =
            {
                {
                    TextureViewTypes::VIEW_1D, // texture 1d
                    TextureViewTypes::VIEW_2D, // texture 2d
                    TextureViewTypes::VIEW_3D, // 3d
                    TextureViewTypes::VIEW_CUBE
                },
                {
                    TextureViewTypes::VIEW_1D_ARRAY, // 1d + array
                    TextureViewTypes::VIEW_2D_ARRAY, // 2d + array
                    TextureViewTypes::VIEW_3D, // 3d
                    TextureViewTypes::VIEW_CUBE_ARRAY
                }
            };
            const bool isArray = Desc.arrayElementCount > 1;
            return aTypes[isArray][Desc.type];
        }

        CTextureManager::CTextureManager( CDeviceContext* pCtx ) :
            m_pCtx{ pCtx }
        {

        }

        CTextureManager::~CTextureManager()
        {
            Destroy();
        }

        void CTextureManager::Destroy()
        {
            for( auto& Pair : m_Samplers.Container )
            {
                auto& pCurr = Pair.second;
                if( pCurr )
                {
                    m_pCtx->DDI().DestroySampler( &pCurr->m_hDDIObject, nullptr );
                }
            }

            for( uint32_t i = 1; i < m_TextureViews.vPool.GetCount(); ++i )
            {
                auto& pCurr = m_TextureViews[i];
                if( pCurr )
                {
                    m_pCtx->DDI().DestroyTextureView( &pCurr->m_hDDIObject, nullptr );
                }
            }
            /*for( uint32_t i = 1; i < m_Textures.vPool.GetCount(); ++i )
            {
                auto& pCurr = m_Textures[i];
                if( pCurr )
                {
                    m_pCtx->DDI().DestroyTexture( &pCurr->m_hDDIObject, nullptr );
                }
            }*/
            /*for(uint32_t i = 0; i < m_Textures.FreeResources.GetCount(); ++i)
            {
                auto& pCurr = m_Textures.FreeResources[i];
                _DestroyTexture( &pCurr );
            }*/
            for( auto& Itr : m_Textures.Resources.Container )
            {
                auto& pCurr = Itr.second;
                _DestroyTexture( &pCurr );
            }

            m_TextureViews.Clear();
            m_Textures.Clear();
            m_RenderTargets.Clear();
            m_Samplers.Clear();

            m_SamplerMemMgr.Destroy();
            m_RenderTargetMemMgr.Destroy();
            m_TexViewMemMgr.Destroy();
            m_TexMemMgr.Destroy();
        }

        Result CTextureManager::Create( const STextureManagerDesc& Desc )
        {
            Result ret = VKE_OK;

            //m_Textures.Add( {} );
            m_TextureViews.Add( {} );
            m_RenderTargets.Add( {} );
            m_Samplers[0] = {};

            uint32_t count = Config::RenderSystem::Texture::MAX_COUNT;
            ret = m_TexMemMgr.Create( count, sizeof( CTexture ), 1 );
            if( VKE_SUCCEEDED( ret ) )
            {
                count = Config::RenderSystem::TextureView::MAX_COUNT;
                ret = m_TexViewMemMgr.Create( count, sizeof( CTextureView ), 1 );
                if( VKE_SUCCEEDED( ret ) )
                {
                    count = Config::RenderSystem::RenderTarget::MAX_COUNT;
                    ret = m_RenderTargetMemMgr.Create( count, sizeof( CRenderTarget ), 1 );
                    if( VKE_SUCCEEDED( ret ) )
                    {
                        count = Config::RenderSystem::Sampler::MAX_COUNT;
                        ret = m_SamplerMemMgr.Create( count, sizeof( CSampler ), 1 );
                    }
                }
            }
            return ret;
        }

        void CTextureManager::_FreeTexture(CTexture** ppInOut)
        {
            VKE_ASSERT2(ppInOut != nullptr && *ppInOut != nullptr, "");
            //m_Textures.Free( (*ppInOut)->GetHandle() );
            m_Textures.AddFree((*ppInOut)->GetHandle().handle);
            *ppInOut = nullptr;
        }

        Result CTextureManager::LoadTexture( const Core::SLoadFileInfo& Info, TextureHandle* phOut )
        {
            Result ret = VKE_FAIL;
            *phOut = INVALID_HANDLE;
            if( ( Info.CreateInfo.flags & Core::CreateResourceFlags::ASYNC ) == Core::CreateResourceFlags::ASYNC )
            {
                m_SyncObj.Lock();
                auto pTask = &m_LoadTaskPool.Get();
                m_SyncObj.Unlock();
                pTask->TaskData.LoadFileInfo = Info;
                pTask->Flags = Info.CreateInfo.TaskFlags;
#if VKE_DEBUG
                Utils::TCString DbgName( "Load Texture: " );
                DbgName += Info.FileInfo.FileName;
                pTask->SetName( DbgName );
#endif
                pTask->Func = [ & ]( Threads::ITask* pTask )
                {
                    uint32_t ret = TaskStateBits::FAIL;
                    Result res = VKE_FAIL;
                    auto pThisTask = ( LoadTextureTask* )pTask;
                    CTexture* pTex = pThisTask->TaskData.pTexture;
                    // If there is already a texture just upload image to gpu
                    
                    if(pTex != nullptr)
                    {
                        if( !pTex->IsReady() )
                        {
                            auto pImg = pTex->m_pImage;
                            VKE_ASSERT2( pImg.IsValid(), 0 );
                            VKE_LOG_TMGR( "Callback, UploadTextureMemory: " << pTex->GetDesc().Name );
                            res = _UploadTextureMemoryTask( StagingBufferFlags::OUT_OF_SPACE_DO_NOTHING, pImg.Get(),
                                                            &pTex );
                        }
                        else
                        {
                            res = VKE_OK;
                        }
                    }
                    else
                    {
                        VKE_LOG_TMGR( "Callback, LoadTextureTask: " << pThisTask->TaskData.LoadFileInfo.FileInfo.FileName );
                        res = this->_LoadTextureTask( pThisTask->TaskData.LoadFileInfo, &pTex );
                        pThisTask->TaskData.pTexture = pTex;
                    }
                    {
                        if( VKE_SUCCEEDED( res ) )
                        {
                            ret = TaskStateBits::OK;
                        }
                        else if( res == VKE_ENOTREADY )
                        {
                            ret = TaskStateBits::WAIT;
                        }
                        if( ( ret & TaskStateBits::WAIT ) != TaskStateBits::WAIT )
                        {
                            auto& CreateInfo = pThisTask->TaskData.LoadFileInfo.CreateInfo;
                            if( CreateInfo.pfnCallback )
                            {
                                CreateInfo.pfnCallback( &pThisTask->TaskData, pTex );
                            }
                            CreateInfo.pOutput = pTex;
                            if( CreateInfo.pResult )
                            {
                                CreateInfo.pResult->result = res;
                                CreateInfo.pResult->pData = pTex;
                            }
                        }
                    }
                    return ret;
                };
                ret = m_pCtx->GetRenderSystem()->GetEngine()->GetThreadPool()->AddTask(
                    Threads::ThreadUsages::RESOURCE_PREPARE, pTask );
            }
            else
            {
                CTexture* pTexture;
                ret = _LoadTextureTask( Info, &pTexture );
                if( VKE_SUCCEEDED( ret ) )
                {
                    *phOut = pTexture->GetHandle();
                    if(Info.CreateInfo.pfnCallback)
                    {
                        SLoadTextureTaskData TaskData;
                        TaskData.LoadFileInfo = Info;
                        TaskData.pTexture = pTexture;
                        Info.CreateInfo.pfnCallback(&TaskData, pTexture);
                    }
                }
            }
            return ret;
        }

        bool vke_force_inline IsDDSFileExt( cstr_t pFileName )
        {
            const auto len = strlen( pFileName );
            cstr_t pName = pFileName + len - 3;
            return strcmp( pName, "dds" ) == 0 || strcmp( pName, "DDS" );
        }

        Result CTextureManager::_LoadTextureTask( const Core::SLoadFileInfo& Info, CTexture** ppOut )
        {
            Result ret = VKE_FAIL;
            CTexture* pTex = nullptr;
            if( Info.FileInfo.FileName != nullptr )
            {
                bool isDDS = IsDDSFileExt( Info.FileInfo.FileName );
                // USe fastpath
                if( isDDS )
                {
                }
                auto pImgMgr = m_pCtx->GetRenderSystem()->GetEngine()->GetImageManager();
                Core::ImageHandle hImg;
                ret = pImgMgr->Load( Info, &hImg );
                if( VKE_SUCCEEDED( ret ) )
                {
                    STAGING_BUFFER_FLAGS flags = StagingBufferFlags::OUT_OF_SPACE_DEFAULT;
                    if( ( Info.CreateInfo.flags & Core::CreateResourceFlags::DEFERRED ) ==
                        Core::CreateResourceFlags::DEFERRED )
                    {
                        flags = StagingBufferFlags::OUT_OF_SPACE_DO_NOTHING;
                    }
                    
                    ret = _CreateTexture( hImg, flags, &pTex );
                    if( VKE_SUCCEEDED(ret) )
                    {
                        // Destroy image after texture is created
                        if( (Info.CreateInfo.flags & Core::CreateResourceFlags::DO_NOT_DESTROY_STAGING_RESOURCES)
                            != Core::CreateResourceFlags::DO_NOT_DESTROY_STAGING_RESOURCES )
                        {
                            auto pImg = std::move( pTex->m_pImage );
                            //pImg->Release();
                            pImgMgr->DestroyImage( &hImg );
                        }
                    }
                }
            }
            *ppOut = pTex;
            return ret;
        }

        Result CTextureManager::_CreateTexture( const Core::ImageHandle& hImg, STAGING_BUFFER_FLAGS updateInfoFlags,
                                                CTexture** ppOut )
        {
            Result ret = VKE_FAIL;
            auto pImgMgr = m_pCtx->GetRenderSystem()->GetEngine()->GetImageManager();
            ImagePtr pImg = pImgMgr->GetImage( hImg );
            const Core::SImageDesc& ImgDesc = pImg->GetDesc();
            STextureDesc TexDesc;
            TexDesc.format = ImgDesc.format;
            TexDesc.memoryUsage = MemoryUsages::GPU_ACCESS | MemoryUsages::TEXTURE;
            TexDesc.Size = ImgDesc.Size;
            TexDesc.type = ImgDesc.type;
            TexDesc.usage = TextureUsages::SAMPLED | TextureUsages::TRANSFER_DST | TextureUsages::TRANSFER_SRC |
                            TextureUsages::FILE_IO;
            TexDesc.mipmapCount = 1;
            TexDesc.Name = pImg->GetDesc().Name;
            TexDesc.SetDebugName( pImg->GetDesc().Name.GetData() );
            CTexture* pTex = _CreateTextureTask( TexDesc );
            if( pTex != nullptr && !pTex->IsReady() )
            {
                pTex->_AddResourceState( Core::ResourceStates::LOADED );
                pTex->m_pImage = std::move(pImg);
                //pTex->m_pImage = pImg;

                /*SUpdateMemoryInfo UpdateInfo;
                UpdateInfo.dataSize = pImg->GetDataSize();
                UpdateInfo.pData = pImg->GetData();
                UpdateInfo.flags = updateInfoFlags;
                VKE_RENDER_SYSTEM_SET_DEBUG_INFO( UpdateInfo, TexDesc.Name.GetData(), SColor::GREEN );

                ret = _UploadTextureMemoryTask( UpdateInfo, &pTex );
                if( ret == VKE_ENOMEMORY && ( ( UpdateInfo.flags & StagingBufferFlags::OUT_OF_SPACE_DO_NOTHING ) ==
                                              StagingBufferFlags::OUT_OF_SPACE_DO_NOTHING ) )
                {
                    ret = VKE_ENOTREADY;
                }
                else if( VKE_FAILED( ret ) )
                {
                    _FreeTexture( &pTex );
                }
                else
                {
                    pTex->_AddResourceState( Core::ResourceStates::PREPARED );
                }*/
                VKE_LOG_TMGR( "Uploading texture: " << pTex->GetDesc().Name );
                ret = _UploadTextureMemoryTask( updateInfoFlags, pTex->m_pImage.Get(), &pTex );
                if( VKE_SUCCEEDED( ret ) )
                {
                    VKE_LOG_TMGR( "Texture: " << pTex->GetDesc().Name << " uploaded." );
                }
                else if( VKE_FAILED(ret) )
                {
                    _FreeTexture( &pTex );
                }
            }
            *ppOut = pTex;
            return ret;
        }

        CTexture* CTextureManager::_CreateTextureTask(const STextureDesc& Desc)
        {
            CTexture* pTex = nullptr;
            VKE_ASSERT2( !Desc.Name.IsEmpty(), "Teture should be named." );
            VKE_ASSERT2( ( Desc.memoryUsage & MemoryUsages::TEXTURE ) == MemoryUsages::TEXTURE, "MemoryUsages::TEXTURE bit must be set in memoryUsage flags" );
            if( Desc.Name.IsEmpty() )
            {
                VKE_LOG_ERR( "TextureDesc should have aName parameter set." );
                goto ERR;
            }
            {
                hash_t hash = Desc.Name.CalcHash();
                TextureHandle hTex = TextureHandle{ static_cast<handle_t>( hash ) };
                //Threads::SyncObject l( m_SyncObj );
                std::scoped_lock l( m_mutex );
                bool reuse = m_Textures.Find( hTex.handle, &pTex );
                if( !reuse )
                {
                    if( VKE_SUCCEEDED( Memory::CreateObject( &m_TexMemMgr, &pTex, this ) ) )
                    {
                        // handle = (uint32_t)m_Textures.Add((pTex));
                        if( !m_Textures.Add( hTex.handle, pTex ) )
                        {
                            VKE_LOG_ERR( "Texture: '" << Desc.Name.GetData() << "' already created!" );
                            Memory::DestroyObject( &m_TexMemMgr, &pTex );
                            goto ERR;
                        }
                        pTex->_AddResourceState( Core::ResourceStates::ALLOCATED );
                    }
                    else
                    {
                        VKE_LOG_ERR( "Unable to create memory for Texture." );
                        goto ERR;
                    }
                }
                else
                {
                    VKE_LOG_TMGR( "Found texture: " << Desc.Name << " hash: " << hash );
                }
                if( pTex != nullptr )
                {
                    pTex->Init( Desc );
                    {
                        if( pTex->GetDDIObject() == DDI_NULL_HANDLE )
                        {
                            pTex->m_hDDIObject = m_pCtx->_GetDDI().CreateTexture( Desc, nullptr );
                            VKE_LOG_TMGR( "Created texture: " << pTex->GetDesc().Name << " " << pTex->m_hDDIObject << " hash: " << hash );
                            pTex->_AddResourceState( Core::ResourceStates::CREATED );
                        }
                        if( pTex->m_hDDIObject != DDI_NULL_HANDLE )
                        {
                            
                            // Create memory for buffer
                            if( Desc.hNative == DDI_NULL_HANDLE && pTex->m_hMemory == INVALID_HANDLE )
                            {
                                SAllocateDesc AllocDesc;

                                AllocDesc.Memory.hDDITexture = pTex->GetDDIObject();
                                AllocDesc.Memory.memoryUsages = Desc.memoryUsage | MemoryUsages::TEXTURE;
                                AllocDesc.Memory.size = 0;
                                AllocDesc.SetDebugInfo( &Desc );
                                VKE_LOG_TMGR( "Alloc mem for: " << Desc.Name << " " << pTex->GetDDIObject() );
                                pTex->m_hMemory = m_pCtx->_GetDeviceMemoryManager().AllocateTexture( AllocDesc );
                                
                                VKE_ASSERT( pTex->m_hMemory != INVALID_HANDLE );
                                if( pTex->m_hMemory == INVALID_HANDLE )
                                {
                                    goto ERR;
                                }
                                pTex->_AddResourceState( Core::ResourceStates::INITIALIZED );
                            }
                            if( pTex->IsResourceStateSet(Core::ResourceStates::INITIALIZED) &&
                                pTex->m_hView == DDI_NULL_HANDLE )
                            {
                                STextureViewDesc ViewDesc;
                                ViewDesc.format = Desc.format;
                                ViewDesc.hTexture = hTex;
                                ViewDesc.type = DetermineViewType( Desc );
                                ViewDesc.SubresourceRange.aspect = DetermineTextureAspect(
                                    Desc.format ); // ConvertTextureUsageToAspect( Desc.usage );
                                ViewDesc.SubresourceRange.beginArrayLayer = 0;
                                ViewDesc.SubresourceRange.beginMipmapLevel = 0;
                                ViewDesc.SubresourceRange.layerCount = 1;
                                ViewDesc.SubresourceRange.mipmapLevelCount = Desc.mipmapCount;
                                ViewDesc.hNative = Desc.hNativeView;
                                ViewDesc.SetDebugName( Desc.GetDebugName() );
                                pTex->m_hView = CreateTextureView( ViewDesc );
                                if( pTex->m_hView == INVALID_HANDLE )
                                {
                                    _FreeTexture( &pTex );
                                    goto ERR;
                                }
                            }
                        }
                        else
                        {
                            goto ERR;
                        }
                        pTex->m_hObject = hTex;
                    }
                }
            }
            return pTex;
        ERR:
            pTex->_Destroy();
                    pTex = nullptr;
            return pTex;
        }

        TextureHandle CTextureManager::CreateTexture( const STextureDesc& Desc )
        {
            CTexture* pTex = _CreateTextureTask(Desc);
            TextureHandle hRet = INVALID_HANDLE;
            if( pTex != nullptr )
            {
                hRet = pTex->GetHandle();
            }
            return hRet;
        }

        TextureHandle CTextureManager::CreateTexture(const Core::ImageHandle& hImg)
        {
            TextureHandle hRet = INVALID_HANDLE;
            CTexture* pTex;
            Result ret = _CreateTexture( hImg, StagingBufferFlags::OUT_OF_SPACE_FLUSH_AND_WAIT, &pTex );
            if (VKE_SUCCEEDED(ret))
            {
                hRet = pTex->GetHandle();
            }
            return hRet;
        }

        Result CTextureManager::_UploadTextureMemoryTask(STAGING_BUFFER_FLAGS flags, Core::CImage* pImg, CTexture** ppInOut)
        {
            Result ret;
            CTexture* pTex = *ppInOut;
            SUpdateMemoryInfo UpdateInfo;
            UpdateInfo.dataSize = pImg->GetDataSize();
            UpdateInfo.pData = pImg->GetData();
            UpdateInfo.flags = flags;
            VKE_RENDER_SYSTEM_SET_DEBUG_INFO( UpdateInfo, pTex->GetDesc().Name.GetData(), SColor::GREEN );
            ret = _UploadTextureMemoryTask( UpdateInfo, &pTex );
            if( ret == VKE_ENOMEMORY && ( ( UpdateInfo.flags & StagingBufferFlags::OUT_OF_SPACE_DO_NOTHING ) ==
                                          StagingBufferFlags::OUT_OF_SPACE_DO_NOTHING ) )
            {
                ret = VKE_ENOTREADY;
            }
            else if( VKE_SUCCEEDED(ret) )
            {
                pTex->_AddResourceState( Core::ResourceStates::PREPARED );
            }
            return ret;
        }

        Result CTextureManager::_UploadTextureMemoryTask(const SUpdateMemoryInfo& Info, CTexture** ppInOut)
        {
            Result ret = VKE_FAIL;
            VKE_ASSERT2(ppInOut != nullptr && *ppInOut != nullptr, "");
            CTexture* pTex = *ppInOut;

            const auto& TexDesc = pTex->GetDesc();
            if (TexDesc.memoryUsage & MemoryUsages::GPU_ACCESS &&
                pTex->m_hFence == DDI_NULL_HANDLE )
            {
                SStagingBufferInfo BufferInfo;
                auto pTransferCtx = m_pCtx->GetTransferContext();
                pTransferCtx->Lock();
                ret = m_pCtx->UploadMemoryToStagingBuffer(Info, &BufferInfo);
                if (VKE_SUCCEEDED(ret))
                {
                    VKE_LOG_TMGR( "Uploading texture: " << pTex->GetDesc().Name );
                    auto pTransferCmdBuffer = pTransferCtx->GetCommandBuffer();
                    VKE_RENDER_SYSTEM_BEGIN_DEBUG_INFO(pTransferCmdBuffer, Info);
                    pTex->m_hFence = pTransferCmdBuffer->GetFence();
                    VKE_ASSERT( pTex->m_hFence != DDI_NULL_HANDLE );
                    pTex->_AddResourceState( Core::ResourceStates::PENDING );

                    STextureBarrierInfo BarrierInfo;
                    /*BarrierInfo.hDDITexture = pTex->GetDDIObject();
                    BarrierInfo.currentState = pTex->GetState();
                    BarrierInfo.newState = TextureStates::TRANSFER_DST;
                    BarrierInfo.srcMemoryAccess = MemoryAccessTypes::SHADER_READ;
                    BarrierInfo.dstMemoryAccess = MemoryAccessTypes::DATA_TRANSFER_WRITE;
                    BarrierInfo.SubresourceRange = Region.TextureSubresource;*/

                    if( pTex->SetState(TextureStates::TRANSFER_DST, &BarrierInfo ) )
                    {
                        pTransferCmdBuffer->Barrier( BarrierInfo );
                    }

                    SCopyBufferToTextureInfo CopyInfo;
                    CopyInfo.hDDIDstTexture = pTex->GetDDIObject();
                    CopyInfo.hDDISrcBuffer = BufferInfo.hDDIBuffer;
                    CopyInfo.textureState = pTex->GetState();
                    SBufferTextureRegion Region;
                    Region.bufferOffset = BufferInfo.offset;
                    Region.bufferRowLength = 0;
                    Region.bufferTextureHeight = 0;
                    Region.textureDepth = 1;
                    Region.textureHeight = TexDesc.Size.height;
                    Region.textureWidth = TexDesc.Size.width;
                    Region.textureOffsetX = 0;
                    Region.textureOffsetY = 0;
                    Region.textureOffsetZ = 0;
                    Region.TextureSubresource.aspect = TextureAspects::COLOR;
                    Region.TextureSubresource.beginArrayLayer = 0;
                    Region.TextureSubresource.beginMipmapLevel = 0;
                    Region.TextureSubresource.layerCount = 1;
                    Region.TextureSubresource.mipmapLevelCount = TexDesc.mipmapCount;
                    CopyInfo.vRegions.PushBack(Region);
                    pTransferCmdBuffer->Copy(CopyInfo);

                    pTex->SetState(TextureStates::SHADER_READ, &BarrierInfo);
                    pTransferCmdBuffer->Barrier(BarrierInfo);
                    //VKE_RENDER_SYSTEM_END_DEBUG_INFO(pTransferCmdBuffer);;
                    VKE_RENDER_SYSTEM_END_DEBUG_INFO( pTransferCmdBuffer );
                }
                pTransferCtx->Unlock();
            }

            return ret;
        }

        Result CTextureManager::UpdateTexture(const SUpdateMemoryInfo& Info, TextureHandle* phInOut)
        {
            CTexture* pTexture = GetTexture( *phInOut ).Get();
            return _UploadTextureMemoryTask(Info, &pTexture);
        }

        TextureViewHandle CTextureManager::CreateTextureView( const STextureViewDesc& Desc )
        {
            //hash_t hash = CTextureView::CalcHash( Desc );
            CTextureView* pView = nullptr;
            TextureViewHandle hRet = nullptr;
            uint32_t handle;

            if( VKE_SUCCEEDED( Memory::CreateObject( &m_TexViewMemMgr, &pView ) ) )
            {
                handle = m_TextureViews.Add( ( pView ) );
            }
            else
            {
                VKE_LOG_ERR( "Unable to create memory for TextureView." );
                goto ERR;
            }
            if( pView != nullptr )
            {
                TexturePtr pTex = GetTexture( Desc.hTexture );
                VKE_LOG_TMGR( "Create texture view for: " << pTex->GetDesc().Name );
                pView->Init( Desc, pTex );
                {
                    if( pView->m_hDDIObject == DDI_NULL_HANDLE )
                    {
                        pView->m_hDDIObject = m_pCtx->_GetDDI().CreateTextureView( Desc, nullptr );
                    }
                    if( pView->m_hDDIObject != DDI_NULL_HANDLE )
                    {
                        hRet.handle = handle;
                        pView->m_hObject = hRet;
                    }
                    else
                    {
                        goto ERR;
                    }
                }
            }

            return hRet;

        ERR:
            pView->_Destroy();
            Memory::DestroyObject( &m_TexViewMemMgr, &pView );
            return hRet;
        }


        void CTextureManager::DestroyTexture( TextureHandle* phTexture )
        {
            TextureHandle& hTex = *phTexture;
            TextureRefPtr pTex;
            //m_Textures.Free( static_cast< uint32_t >( hTex.handle ) );
            {
                pTex = GetTexture( hTex );
                if( pTex.IsValid() )
                {
                    //CTexture* pTmp = pTex.Release();
                    m_Textures.AddFree( hTex.handle );
                }
            }
            hTex = INVALID_HANDLE;
        }

        void CTextureManager::_DestroyTexture( CTexture** ppInOut )
        {
            CTexture* pTex = *ppInOut;
            m_pCtx->_GetDDI().DestroyTexture( &pTex->m_hDDIObject, nullptr );
            Memory::DestroyObject( &m_TexMemMgr, &pTex );
            *ppInOut = nullptr;
        }


        void CTextureManager::DestroyTextureView( TextureViewHandle* phView )
        {
            TextureViewHandle& hView = *phView;
            TextureViewRefPtr pView;
            m_TextureViews.Free( static_cast< uint32_t >( hView.handle ) );
            {
                CTextureView* pTmp = pView.Release();
                _DestroyTextureView( &pTmp );

            }
            hView = INVALID_HANDLE;
        }

        void CTextureManager::_DestroyTextureView( CTextureView** ppInOut )
        {
            CTextureView* pView = *ppInOut;
            m_pCtx->_GetDDI().DestroyTextureView( &pView->m_hDDIObject, nullptr );
            Memory::DestroyObject( &m_TexViewMemMgr, &pView );
            *ppInOut = nullptr;
        }

        RenderTargetHandle CTextureManager::CreateRenderTarget( const SRenderTargetDesc& Desc )
        {
            RenderTargetHandle hRet = INVALID_HANDLE;
            STextureDesc TexDesc;
            STextureViewDesc ViewDesc;
            uint32_t handle;
            VKE_ASSERT2( !Desc.Name.IsEmpty(), "" );
            CRenderTarget*  pRT = nullptr;
            if( VKE_SUCCEEDED( Memory::CreateObject( &m_RenderTargetMemMgr, &pRT ) ) )
            {
                handle = m_RenderTargets.Add( { pRT } );
            }
            else
            {
                VKE_LOG_ERR( "Unable to create memory for RenderTarget." );
                goto ERR;
            }
            if( pRT != nullptr )
            {
                pRT->Init( Desc );
                {
                    TextureHandle hTex = Desc.hTexture;
                    TexDesc.format = Desc.format;
                    TexDesc.memoryUsage = Desc.memoryUsage;
                    TexDesc.mipmapCount = Desc.mipmapCount;
                    TexDesc.multisampling = Desc.multisampling;
                    TexDesc.Size = Desc.Size;
                    TexDesc.type = Desc.type;
                    TexDesc.usage = Desc.usage;
                    TexDesc.Name = Desc.Name;
                    TexDesc.SetDebugName( Desc.GetDebugName() );
                    
                    if( hTex == INVALID_HANDLE )
                    {
                        hTex = CreateTexture( TexDesc );
                    }
                    if( hTex != INVALID_HANDLE )
                    {
                        auto pView = GetTexture( hTex )->GetView();
                        TextureViewHandle hView;
                        if( pView.IsNull() )
                        {
                            ViewDesc.format = Desc.format;
                            ViewDesc.hTexture = hTex;
                            ViewDesc.SubresourceRange.aspect = ConvertTextureUsageToAspect( Desc.usage );
                            ViewDesc.SubresourceRange.beginArrayLayer = 0;
                            ViewDesc.SubresourceRange.beginMipmapLevel = 0;
                            ViewDesc.SubresourceRange.layerCount = 1;
                            ViewDesc.SubresourceRange.mipmapLevelCount = Desc.mipmapCount;
                            hView = CreateTextureView( ViewDesc );
                        }
                        else
                        {
                            hView = pView->GetHandle();
                        }
                        if( hView != INVALID_HANDLE )
                        {
                            hRet.handle = handle;

                            pRT->m_hObject = hRet;
                            pRT->m_hTexture = hTex;
                            pRT->m_Desc.hTextureView = hView;
                            pRT->m_Size = Desc.Size;
                        }
                    }
                    else
                    {
                        goto ERR;
                    }
                    m_mRenderTargetNames[ TexDesc.Name.GetData() ] = hRet;
                }
            }
            return hRet;
        ERR:
            _DestroyRenderTarget( &pRT );
            return hRet;
        }

        void CTextureManager::_DestroyRenderTarget( CRenderTarget** ppInOut )
        {
            CRenderTarget* pRT = *ppInOut;

            pRT->_Destroy();
            DestroyTextureView( &pRT->m_Desc.hTextureView );
            DestroyTexture( &pRT->m_hTexture );

            Memory::DestroyObject( &m_RenderTargetMemMgr, ppInOut );
            *ppInOut = nullptr;
        }

        TextureRefPtr CTextureManager::GetTexture( TextureHandle hTexture )
        {
            auto pRet = TextureRefPtr{ m_Textures[ ( hTexture.handle )] };
            VKE_ASSERT2( pRet.IsValid(), "" );
            return pRet;
        }

        TextureViewRefPtr CTextureManager::GetTextureView( TextureViewHandle hView )
        {
            return TextureViewRefPtr{ m_TextureViews[ static_cast<uint32_t>( hView.handle ) ] };
        }

        TextureViewRefPtr CTextureManager::GetTextureView( const TextureHandle& hTexture )
        {
            return GetTexture( hTexture )->GetView();
        }

        TextureViewRefPtr CTextureManager::GetTextureView( const RenderTargetHandle& hRT )
        {
            RenderTargetPtr pRT = GetRenderTarget( hRT );
            return GetTextureView( pRT->GetTextureView() );
        }

        RenderTargetRefPtr CTextureManager::GetRenderTarget( const RenderTargetHandle& hRT )
        {
            return RenderTargetRefPtr{ m_RenderTargets[ static_cast<uint32_t>( hRT.handle ) ] };
        }

        RenderTargetRefPtr CTextureManager::GetRenderTarget( cstr_t pName )
        {
            RenderTargetRefPtr pRet;
            auto Itr = m_mRenderTargetNames.find( pName );
            if(Itr != m_mRenderTargetNames.end())
            {
                pRet = GetRenderTarget( Itr->second );
            }
            else
            {
                VKE_LOG_ERR( "Unable to find RenderTarget with name: " << pName );
            }
            return pRet;
        }

        void CTextureManager::DestroyRenderTarget( RenderTargetHandle* phRT )
        {
            CRenderTarget* pRT = GetRenderTarget( *phRT ).Release();
            _DestroyRenderTarget( &pRT );
            *phRT = INVALID_HANDLE;
        }

        SamplerHandle CTextureManager::CreateSampler( const SSamplerDesc& Desc )
        {
            SamplerHandle hRet = INVALID_HANDLE;
            hash_t hash = CSampler::CalcHash( Desc );
            CSampler* pSampler = nullptr;
            SamplerMap::Iterator Itr;

            if( !m_Samplers.Find( hash, &pSampler, &Itr ) )
            {
                if( VKE_SUCCEEDED( Memory::CreateObject( &m_SamplerMemMgr, &pSampler, this ) ) )
                {
                    m_Samplers.Insert( Itr, hash, pSampler );

                }
                else
                {
                    VKE_LOG_ERR("Unable to create memory for CSampler object.");
                    goto ERR;
                }
            }
            if( pSampler )
            {
                hRet.handle = hash;
                if( pSampler->GetDDIObject() == DDI_NULL_HANDLE )
                {
                    pSampler->Init( Desc );
                    pSampler->m_hDDIObject = m_pCtx->DDI().CreateSampler( pSampler->m_Desc, nullptr );
                    if( pSampler->m_hDDIObject != DDI_NULL_HANDLE )
                    {
                        pSampler->m_hObject = hRet;
                    }
                    else
                    {
                        hRet = INVALID_HANDLE;
                        goto ERR;
                    }
                }
                else
                {
                    hRet = INVALID_HANDLE;
                    goto ERR;
                }
            }
            return hRet;
        ERR:
            _DestroySampler( &pSampler );
            return hRet;
        }

        SamplerRefPtr CTextureManager::GetSampler( const SamplerHandle& hSampler )
        {
            SamplerRefPtr pRet;
            CSampler* pSampler;
            m_Samplers.Find( (hash_t)hSampler.handle, &pSampler );
            {
                pRet = SamplerRefPtr{ pSampler };
            }
            return pRet;
        }

        void CTextureManager::DestroySampler( SamplerHandle* phSampler )
        {
            CSampler* pSampler;
            auto Itr = m_Samplers.Find( (hash_t)(*phSampler).handle, &pSampler );
            _DestroySampler( &pSampler );
            m_Samplers.Remove( Itr );
            *phSampler = INVALID_HANDLE;
        }

        void CTextureManager::_DestroySampler( CSampler** ppInOut )
        {
            VKE_ASSERT2( ppInOut != nullptr && *ppInOut != nullptr, "" );
            CSampler* pSampler = *ppInOut;
            m_pCtx->DDI().DestroySampler( &pSampler->m_hDDIObject, nullptr );
            pSampler->_Destroy();
            Memory::DestroyObject( &m_SamplerMemMgr, &pSampler );
            *ppInOut = nullptr;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDER_SYSTEM