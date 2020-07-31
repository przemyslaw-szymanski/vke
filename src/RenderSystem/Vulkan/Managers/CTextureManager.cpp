#if VKE_VULKAN_RENDERER
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

        TEXTURE_VIEW_TYPE DetermineViewType(const STextureDesc& Desc)
        {
            static const TEXTURE_VIEW_TYPE aTypes[][2] =
            {
                // no array
                TextureViewTypes::VIEW_1D, // texture 1d
                TextureViewTypes::VIEW_2D, // texture 2d
                TextureViewTypes::VIEW_3D, // 3d
                TextureViewTypes::VIEW_CUBE, // cube
                // array
                TextureViewTypes::VIEW_1D_ARRAY, // 1d + array
                TextureViewTypes::VIEW_2D_ARRAY, // 2d + array
                TextureViewTypes::VIEW_3D, // 3d
                TextureViewTypes::VIEW_CUBE_ARRAY, // cube + array
            };
            const bool isArray = Desc.arrayElementCount > 1;
            return aTypes[Desc.usage][isArray];
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
            for(uint32_t i = 0; i < m_Textures.FreeResources.GetCount(); ++i)
            {
                auto& pCurr = m_Textures.FreeResources[i];
                _DestroyTexture( &pCurr );
            }
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
            VKE_ASSERT(ppInOut != nullptr && *ppInOut != nullptr, "");
            //m_Textures.Free( (*ppInOut)->GetHandle() );
            m_Textures.AddFree((*ppInOut)->GetHandle().handle, (*ppInOut));
            *ppInOut = nullptr;
        }

        CTexture* CTextureManager::_CreateTextureTask(const STextureDesc& Desc)
        {
            CTexture* pTex = nullptr;
            VKE_ASSERT( Desc.aName[0] != 0, "Teture should be named." );
            if( Desc.aName[0] == 0 )
            {
                VKE_LOG_ERR( "TextureDesc should have aName parameter set." );
                goto ERR;
            }

            hash_t hash = CTexture::CalcHash( Desc.aName );
            TextureHandle hTex = TextureHandle{static_cast<handle_t>(hash)};

            if (VKE_SUCCEEDED(Memory::CreateObject(&m_TexMemMgr, &pTex, this)))
            {
                //handle = (uint32_t)m_Textures.Add((pTex));
                if( !m_Textures.Add( hTex.handle, pTex ) )
                {
                    Memory::DestroyObject(&m_TexMemMgr, &pTex);
                    goto ERR;
                }
            }
            else
            {
                VKE_LOG_ERR("Unable to create memory for Texture.");
                goto ERR;
            }

            if (pTex != nullptr)
            {
                pTex->Init(Desc);
                {
                    if (pTex->GetDDIObject() == DDI_NULL_HANDLE)
                    {
                        pTex->m_hDDIObject = m_pCtx->_GetDDI().CreateTexture(Desc, nullptr);
                    }
                    if (pTex->m_hDDIObject != DDI_NULL_HANDLE)
                    {
                        // Create memory for buffer
                        SAllocateDesc AllocDesc;
                        AllocDesc.Memory.hDDITexture = pTex->GetDDIObject();
                        AllocDesc.Memory.memoryUsages = Desc.memoryUsage | MemoryUsages::TEXTURE;
                        AllocDesc.Memory.size = 0;
                        AllocDesc.poolSize = VKE_MEGABYTES(10);
                        pTex->m_hMemory = m_pCtx->_GetDeviceMemoryManager().AllocateTexture(AllocDesc);
                        if( pTex->m_hMemory != INVALID_HANDLE )
                        {
                            pTex->m_hObject = hTex;

                            STextureViewDesc ViewDesc;
                            ViewDesc.format = Desc.format;
                            ViewDesc.hTexture = hTex;
                            ViewDesc.type = DetermineViewType( Desc );
                            ViewDesc.SubresourceRange.aspect = ConvertTextureUsageToAspect( Desc.usage );
                            ViewDesc.SubresourceRange.beginArrayLayer = 0;
                            ViewDesc.SubresourceRange.beginMipmapLevel = 0;
                            ViewDesc.SubresourceRange.layerCount = 1;
                            ViewDesc.SubresourceRange.mipmapLevelCount = Desc.mipmapCount;

                            pTex->m_hView = CreateTextureView( ViewDesc );
                            if( pTex->m_hView == INVALID_HANDLE )
                            {
                                _FreeTexture( &pTex );
                                goto ERR;
                            }
                        }
                        else
                        {
                            goto ERR;
                        }
                    }
                    else
                    {
                        goto ERR;
                    }
                }
            }

            return pTex;
        ERR:
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

        TextureHandle CTextureManager::LoadTexture(const Core::SLoadFileInfo& Info)
        {
            TextureHandle hRet = INVALID_HANDLE;
            /// TODO: support for async
            CTexture* pTexture = _LoadTextureTask(Info);
            if (pTexture != nullptr)
            {
                hRet = pTexture->GetHandle();
            }
            return hRet;
        }

        bool vke_force_inline IsDDSFileExt(cstr_t pFileName)
        {
            const auto len = strlen( pFileName );
            cstr_t pName = pFileName + len - 3;
            return strcmp( pName, "dds" ) == 0 || strcmp( pName, "DDS" );
        }

        CTexture* CTextureManager::_LoadTextureTask(const Core::SLoadFileInfo& Info)
        {
            CTexture* pTex = nullptr;
            if( Info.FileInfo.pFileName != nullptr )
            {
                bool isDDS = IsDDSFileExt(Info.FileInfo.pFileName);
                // USe fastpath
                if( isDDS )
                {

                }
                auto pImgMgr = m_pCtx->GetRenderSystem()->GetEngine()->GetImageManager();
                auto hImg = pImgMgr->Load( Info );
                if( hImg != INVALID_HANDLE )
                {
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
                    TexDesc.SetName(Info.FileInfo.pFileName);

                    pTex = _CreateTextureTask( TexDesc );
                    if( pTex != nullptr )
                    {
                        SUpdateMemoryInfo UpdateInfo;
                        UpdateInfo.dataSize = pImg->GetDataSize();
                        UpdateInfo.pData = pImg->GetData();
                        VKE_RENDER_SYSTEM_SET_DEBUG_INFO( UpdateInfo, Info.FileInfo.pFileName, SColor::GREEN );

                        if( VKE_SUCCEEDED( _UpdateTextureTask( UpdateInfo, &pTex ) ) )
                        {

                        }
                        else
                        {
                            _FreeTexture( &pTex );
                        }
                    }
                }
            }
            return pTex;
        }

        Result CTextureManager::_UpdateTextureTask(const SUpdateMemoryInfo& Info, CTexture** ppInOut)
        {
            Result ret = VKE_FAIL;
            VKE_ASSERT(ppInOut != nullptr && *ppInOut != nullptr, "");
            CTexture* pTex = *ppInOut;

            const auto& TexDesc = pTex->GetDesc();
            if (TexDesc.memoryUsage & MemoryUsages::GPU_ACCESS)
            {
                SStagingBufferInfo BufferInfo;
                ret = m_pCtx->UploadMemoryToStagingBuffer(Info, &BufferInfo);
                if (VKE_SUCCEEDED(ret))
                {
                    auto pTransferCmdBuffer = m_pCtx->GetTransferContext()->GetCommandBuffer();
                    VKE_RENDER_SYSTEM_BEGIN_DEBUG_INFO(pTransferCmdBuffer, Info);

                    STextureBarrierInfo BarrierInfo;
                    /*BarrierInfo.hDDITexture = pTex->GetDDIObject();
                    BarrierInfo.currentState = pTex->GetState();
                    BarrierInfo.newState = TextureStates::TRANSFER_DST;
                    BarrierInfo.srcMemoryAccess = MemoryAccessTypes::SHADER_READ;
                    BarrierInfo.dstMemoryAccess = MemoryAccessTypes::DATA_TRANSFER_WRITE;
                    BarrierInfo.SubresourceRange = Region.TextureSubresource;*/

                    if( VKE_SUCCEEDED( pTex->SetState(TextureStates::TRANSFER_DST, &BarrierInfo ) ) )
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
                    Region.textureWidth = TexDesc.Size.height;
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

                    /*pTex->SetState(TextureStates::SHADER_READ, &BarrierInfo);

                    pTransferCmdBuffer->Barrier(BarrierInfo);
                    VKE_RENDER_SYSTEM_END_DEBUG_INFO(pTransferCmdBuffer);;*/
                }
            }

            return ret;
        }

        Result CTextureManager::UpdateTexture(const SUpdateMemoryInfo& Info, TextureHandle* phInOut)
        {
            CTexture* pTexture = GetTexture( *phInOut ).Get();
            return _UpdateTextureTask(Info, &pTexture);
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
                    CTexture* pTmp = pTex.Release();
                    m_Textures.AddFree( hTex.handle, pTmp );
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
                    TexDesc.format = Desc.format;
                    TexDesc.memoryUsage = Desc.memoryUsage;
                    TexDesc.mipmapCount = Desc.mipLevelCount;
                    TexDesc.multisampling = Desc.multisampling;
                    TexDesc.Size = Desc.Size;
                    TexDesc.type = Desc.type;
                    TexDesc.usage = Desc.usage;
                    auto hTex = CreateTexture( TexDesc );
                    if( hTex != INVALID_HANDLE )
                    {
                        ViewDesc.format = Desc.format;
                        ViewDesc.hTexture = hTex;
                        ViewDesc.SubresourceRange.aspect = ConvertTextureUsageToAspect( Desc.usage );
                        ViewDesc.SubresourceRange.beginArrayLayer = 0;
                        ViewDesc.SubresourceRange.beginMipmapLevel = 0;
                        ViewDesc.SubresourceRange.layerCount = 1;
                        ViewDesc.SubresourceRange.mipmapLevelCount = Desc.mipLevelCount;
                        auto hView = CreateTextureView( ViewDesc );
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
            return TextureRefPtr{ m_Textures[ ( hTexture.handle )] };
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
            VKE_ASSERT( ppInOut != nullptr && *ppInOut != nullptr, "" );
            CSampler* pSampler = *ppInOut;
            m_pCtx->DDI().DestroySampler( &pSampler->m_hDDIObject, nullptr );
            pSampler->_Destroy();
            Memory::DestroyObject( &m_SamplerMemMgr, &pSampler );
            *ppInOut = nullptr;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER