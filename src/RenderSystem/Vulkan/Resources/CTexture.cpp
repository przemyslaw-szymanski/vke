#include "RenderSystem/Resources/CTexture.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CCommandBuffer.h"
#include "RenderSystem/Vulkan/Managers/CTextureManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        TEXTURE_ASPECT CTexture::ConvertFormatToAspect( const TEXTURE_FORMAT& format )
        {
            TEXTURE_ASPECT ret;

            switch( format )
            {
                case Formats::D32_SFLOAT:
                case Formats::X8_D24_UNORM_PACK32:
                case Formats::D16_UNORM:
                {
                    ret = TextureAspects::DEPTH;
                }
                break;
                case Formats::D32_SFLOAT_S8_UINT:
                case Formats::D24_UNORM_S8_UINT:
                case Formats::D16_UNORM_S8_UINT:
                {
                    ret = TextureAspects::DEPTH_STENCIL;
                }
                break;
                default:
                {
                    ret = TextureAspects::COLOR;
                }
                break;
            };
            return ret;
        }

        MEMORY_ACCESS_TYPE CTexture::ConvertStateToSrcMemoryAccess( const TEXTURE_STATE& currState, const TEXTURE_STATE& newState )
        {
            static const MEMORY_ACCESS_TYPE aaTypes[TextureStates::_MAX_COUNT][TextureStates::_MAX_COUNT] =
            {
                // From undefined
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::SHADER_WRITE, // shader read
                    MemoryAccessTypes::GPU_MEMORY_READ, // transfer src
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From general
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::SHADER_WRITE, // shader read
                    MemoryAccessTypes::GPU_MEMORY_READ, // transfer src
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From color render target
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::SHADER_WRITE, // shader read
                    MemoryAccessTypes::GPU_MEMORY_READ, // transfer src
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From depth stencil render target
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::SHADER_WRITE, // shader read
                    MemoryAccessTypes::GPU_MEMORY_READ, // transfer src
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From dpeth buffer
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::SHADER_WRITE, // shader read
                    MemoryAccessTypes::GPU_MEMORY_READ, // transfer src
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From shader read
                {
                    MemoryAccessTypes::SHADER_WRITE, // undefined
                    MemoryAccessTypes::SHADER_WRITE, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_WRITE, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_WRITE, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::SHADER_WRITE, // shader read
                    MemoryAccessTypes::GPU_MEMORY_READ, // transfer src
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From transfer src
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::SHADER_WRITE, // shader read
                    MemoryAccessTypes::GPU_MEMORY_READ, // transfer src
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From transfer dst
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // shader read
                    MemoryAccessTypes::GPU_MEMORY_READ, // transfer src
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From present
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::SHADER_WRITE, // shader read
                    MemoryAccessTypes::GPU_MEMORY_READ, // transfer src
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                }
            };
            return aaTypes[currState][newState];
        }

        MEMORY_ACCESS_TYPE CTexture::ConvertStateToDstMemoryAccess( const TEXTURE_STATE& currState, const TEXTURE_STATE& newState )
        {
            static const MEMORY_ACCESS_TYPE aaTypes[TextureStates::_MAX_COUNT][TextureStates::_MAX_COUNT] =
            {
                // From undefined
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::SHADER_WRITE, // shader read
                    MemoryAccessTypes::GPU_MEMORY_READ, // transfer src
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From general
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::SHADER_WRITE, // shader read
                    MemoryAccessTypes::GPU_MEMORY_READ, // transfer src
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From color render target
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_READ, // depth buffer
                    MemoryAccessTypes::SHADER_READ, // shader read
                    MemoryAccessTypes::GPU_MEMORY_READ, // transfer src
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From depth stencil render target
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::SHADER_READ, // shader read
                    MemoryAccessTypes::GPU_MEMORY_READ, // transfer src
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From dpeth buffer
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::SHADER_READ, // shader read
                    MemoryAccessTypes::GPU_MEMORY_READ, // transfer src
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From shader read
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::SHADER_READ, // shader read
                    MemoryAccessTypes::GPU_MEMORY_READ, // transfer src
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From transfer src
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::SHADER_READ, // shader read
                    MemoryAccessTypes::GPU_MEMORY_READ, // transfer src
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From transfer dst
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::SHADER_READ, // shader read
                    MemoryAccessTypes::GPU_MEMORY_READ, // transfer src
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From present
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::SHADER_WRITE, // shader read
                    MemoryAccessTypes::GPU_MEMORY_READ, // transfer src
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                }
            };
            return aaTypes[currState][newState];
        }

        CSampler::CSampler( CTextureManager* )
        {}

        CSampler::~CSampler()
        {}

        void CSampler::_Destroy()
        {}

        void CSampler::Init( const SSamplerDesc& Desc )
        {
            m_Desc = Desc;
        }

        hash_t CSampler::CalcHash( const SSamplerDesc& Desc )
        {
            SHash Hash;
            Hash.Combine( Desc.AddressMode.U, Desc.AddressMode.V, Desc.AddressMode.W );
            Hash.Combine( Desc.borderColor, Desc.compareFunc, Desc.enableAnisotropy, Desc.enableCompare );
            Hash.Combine( Desc.Filter.mag, Desc.Filter.min, Desc.LOD.min, Desc.LOD.max );
            Hash.Combine( Desc.maxAnisotropy, Desc.mipLODBias, Desc.mipmapMode, Desc.unnormalizedCoordinates );
            return Hash.value;
        }

        CTexture::CTexture(CTextureManager* pMgr) :
            m_pMgr{ pMgr }
        {}

        CTexture::~CTexture()
        {}

        void CTexture::Init(const STextureDesc& Desc)
        {
            m_Desc = Desc;
            //m_vViews.Clear();
        }

        Result CTexture::SetState( const TEXTURE_STATE& state, STextureBarrierInfo* pOut )
        {
            Result ret = VKE_FAIL;
            if( m_state != state )
            {
                pOut->currentState = m_state;
                pOut->hDDITexture = GetDDIObject();
                pOut->newState = state;
                pOut->SubresourceRange.aspect = ConvertFormatToAspect(m_Desc.format);
                pOut->SubresourceRange.beginArrayLayer = 0;
                pOut->SubresourceRange.beginMipmapLevel = 0;
                pOut->SubresourceRange.layerCount = 1;
                pOut->SubresourceRange.mipmapLevelCount = m_Desc.mipmapCount;
                pOut->srcMemoryAccess = ConvertStateToSrcMemoryAccess(m_state, state);
                pOut->dstMemoryAccess = ConvertStateToDstMemoryAccess(m_state, state);
                m_state = state;
                ret = VKE_OK;
            }
            return ret;
        }

        TextureViewRefPtr CTexture::GetView()
        {
            return m_pMgr->GetTextureView( m_hView );
        }

        SamplerRefPtr CTexture::GetSampler()
        {
            return m_pMgr->GetSampler( m_hSampler );
        }

        hash_t CTexture::CalcHash( const STextureDesc& Desc )
        {
            SHash Hash;
            Hash.Combine( Desc.format, Desc.memoryUsage, Desc.mipmapCount, Desc.multisampling,
                Desc.Size.width, Desc.Size.height, Desc.type, Desc.usage );
            return Hash.value;
        }

        hash_t CTexture::CalcHash(cstr_t pName)
        {
            SHash Hash;
            Hash += pName;
            return Hash.value;
        }

        CTextureView::CTextureView()
        {}

        CTextureView::~CTextureView()
        {}

        void CTextureView::Init( const STextureViewDesc& Desc, TexturePtr pTexture )
        {
            m_Desc = Desc;
        }

        hash_t CTextureView::CalcHash( const STextureViewDesc& Desc )
        {
            SHash Hash;
            Hash.Combine( Desc.format, Desc.hTexture.handle, Desc.SubresourceRange.aspect,
                Desc.SubresourceRange.beginArrayLayer, Desc.SubresourceRange.beginMipmapLevel,
                Desc.SubresourceRange.layerCount, Desc.SubresourceRange.mipmapLevelCount,
                Desc.type );
            return Hash.value;
        }

        CRenderTarget::CRenderTarget()
        {}

        CRenderTarget::~CRenderTarget()
        {}

        void CRenderTarget::_Destroy()
        {

        }

        void CRenderTarget::Init( const SRenderTargetDesc& Desc )
        {
            m_Desc.beginLayout = Desc.beginState;
            m_Desc.ClearValue = Desc.ClearValue;
            m_Desc.endLayout = Desc.endState;
            m_Desc.format = Desc.format;
            m_Desc.sampleCount = Desc.multisampling;
            m_Desc.usage = Desc.clearStoreUsage;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER