#include "RenderSystem/CRenderPass.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/Vulkan/CResourceManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        CRenderPass::CRenderPass(CDeviceContext* pCtx) :
            m_pCtx(pCtx)
            , m_VkDevice(pCtx->_GetDevice())
        {}

        CRenderPass::~CRenderPass()
        {
            Destroy();
        }

        void CRenderPass::Destroy(bool destroyRenderPass)
        {
            if (m_vkFramebuffer == VK_NULL_HANDLE)
            {
                return;
            }

            const auto& VkDevice = m_pCtx->_GetDevice();
            VkDevice.DestroyObject(nullptr, &m_vkFramebuffer);
            if (destroyRenderPass)
            {
                VkDevice.DestroyObject(nullptr, &m_vkRenderPass);
            }
        }

        int32_t FindTextureHandle(const SRenderPassDesc::AttachmentDescArray& vAttachments,
                                  const TextureViewHandle& hTexView)
        {
            int32_t res = -1;
            for( uint32_t a = 0; a < vAttachments.GetCount(); ++a )
            {
                if( hTexView == vAttachments[ a ].hTextureView )
                {
                    res = a;
                    break;
                }
            }
            return res;
        }

        bool MakeAttachmentRef(const SRenderPassDesc::AttachmentDescArray& vAttachments,
                               const SSubpassAttachmentDesc& SubpassAttachmentDesc,
                               VkAttachmentReference* pRefOut)
        {
            int32_t idx = FindTextureHandle(vAttachments, SubpassAttachmentDesc.hTextureView);
            bool res = false;
            if( idx >= 0 )
            {
                pRefOut->attachment = idx;
                pRefOut->layout = Vulkan::Map::ImageLayout(SubpassAttachmentDesc.layout);
                res = true;
            }
            return res;
        }

        Result CRenderPass::Create(const SRenderPassDesc& Desc)
        {
            Destroy();

            m_Desc = Desc;

            using VkAttachmentDescriptionArray = Utils::TCDynamicArray< VkAttachmentDescription, 8 >;
            using VkAttachmentRefArray = Utils::TCDynamicArray< VkAttachmentReference >;

            struct SSubpassDesc
            {
                VkAttachmentRefArray vInputAttachmentRefs;
                VkAttachmentRefArray vColorAttachmentRefs;
                VkAttachmentReference vkDepthStencilRef;
                VkAttachmentReference* pVkDepthStencilRef = nullptr;
            };

            using SubpassDescArray = Utils::TCDynamicArray< SSubpassDesc >;
            using VkSubpassDescArray = Utils::TCDynamicArray< VkSubpassDescription >;

            VkAttachmentDescriptionArray vVkAttachmentDescriptions;
            SubpassDescArray vSubpassDescs;
            VkSubpassDescArray vVkSubpassDescs;

            const auto& ResMgr = m_pCtx->GetResourceManager();
            m_vVkImageViews.Clear();
            m_vVkClearValues.Clear();
            m_vVkImages.Clear();

            for( uint32_t a = 0; a < Desc.vAttachments.GetCount(); ++a )
            {
                const SRenderPassAttachmentDesc& AttachmentDesc = Desc.vAttachments[a];
                const VkImageCreateInfo& vkImgInfo = ResMgr.GetTextureDesc( AttachmentDesc.hTextureView );
                VkAttachmentDescription vkAttachmentDesc;
                vkAttachmentDesc.finalLayout = Vulkan::Map::ImageLayout( AttachmentDesc.endLayout );
                vkAttachmentDesc.flags = 0;
                vkAttachmentDesc.format = vkImgInfo.format;
                vkAttachmentDesc.initialLayout = Vulkan::Map::ImageLayout(AttachmentDesc.beginLayout);
                vkAttachmentDesc.loadOp = Vulkan::Convert::UsageToLoadOp(AttachmentDesc.usage);
                vkAttachmentDesc.storeOp = Vulkan::Convert::UsageToStoreOp(AttachmentDesc.usage);
                vkAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                vkAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                vkAttachmentDesc.samples = vkImgInfo.samples;
                vVkAttachmentDescriptions.PushBack(vkAttachmentDesc);

                VkImageView vkView = ResMgr.GetTextureView(AttachmentDesc.hTextureView);
                m_vVkImageViews.PushBack(vkView);
                VkImage vkImg = ResMgr.GetTextureViewDesc( AttachmentDesc.hTextureView ).image;
                m_vVkImages.PushBack( vkImg );

                VkClearValue vkClear;
                AttachmentDesc.ClearColor.CopyToNative(&vkClear);
                m_vVkClearValues.PushBack(vkClear);
            }

            for( uint32_t s = 0; s < Desc.vSubpasses.GetCount(); ++s )
            {
                SSubpassDesc SubDesc;

                const auto& SubpassDesc = Desc.vSubpasses[ s ];
                for( uint32_t r = 0; r < SubpassDesc.vRenderTargets.GetCount(); ++r )
                {
                    const auto& RenderTargetDesc = SubpassDesc.vRenderTargets[ r ];
                    
                    // Find attachment
                    VkAttachmentReference vkRef;
                    if( MakeAttachmentRef(Desc.vAttachments, RenderTargetDesc, &vkRef) )
                    {
                        SubDesc.vColorAttachmentRefs.PushBack(vkRef);
                    }
                }

                for( uint32_t t = 0; t < SubpassDesc.vTextures.GetCount(); ++t )
                {
                    const auto& TexDesc = SubpassDesc.vTextures[ t ];
                    // Find attachment
                    VkAttachmentReference vkRef;
                    if( MakeAttachmentRef(Desc.vAttachments, TexDesc, &vkRef) )
                    {
                        SubDesc.vInputAttachmentRefs.PushBack(vkRef);
                    }
                }

                // Find attachment
                VkAttachmentReference* pVkDepthStencilRef = nullptr;
                if( SubpassDesc.DepthBuffer.hTextureView != NULL_HANDLE )
                {
                    VkAttachmentReference vkRef;
                    if( MakeAttachmentRef(Desc.vAttachments, SubpassDesc.DepthBuffer, &vkRef) )
                    {
                        SubDesc.vkDepthStencilRef = vkRef;
                        SubDesc.pVkDepthStencilRef = &SubDesc.vkDepthStencilRef;
                    }
                }

                VkSubpassDescription VkSubpassDesc;
                const auto colorCount = SubDesc.vColorAttachmentRefs.GetCount();
                const auto inputCount = SubDesc.vInputAttachmentRefs.GetCount();

                VkSubpassDesc.colorAttachmentCount = colorCount;
                VkSubpassDesc.pColorAttachments = (colorCount > 0)? &SubDesc.vColorAttachmentRefs[ 0 ] : nullptr;
                VkSubpassDesc.inputAttachmentCount = inputCount;
                VkSubpassDesc.pInputAttachments = (inputCount > 0)? &SubDesc.vInputAttachmentRefs[ 0 ] : nullptr;
                VkSubpassDesc.pDepthStencilAttachment = pVkDepthStencilRef;
                VkSubpassDesc.flags = 0;
                VkSubpassDesc.pResolveAttachments = nullptr;
                VkSubpassDesc.preserveAttachmentCount = 0;
                VkSubpassDesc.pPreserveAttachments = nullptr;
                VkSubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

                vSubpassDescs.PushBack(SubDesc);
                vVkSubpassDescs.PushBack(VkSubpassDesc);
            }

            {
                VkRenderPassCreateInfo ci;
                Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);
                ci.attachmentCount = vVkAttachmentDescriptions.GetCount();
                ci.pAttachments = &vVkAttachmentDescriptions[ 0 ];
                ci.dependencyCount = 0;
                ci.pDependencies = nullptr;
                ci.subpassCount = vVkSubpassDescs.GetCount();
                ci.pSubpasses = &vVkSubpassDescs[ 0 ];
                ci.flags = 0;
                VK_ERR(m_VkDevice.CreateObject(ci, nullptr, &m_vkRenderPass));
            }
            {
                VkFramebufferCreateInfo ci;
                Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
                ci.flags = 0;
                ci.attachmentCount = m_vVkImageViews.GetCount();
                ci.pAttachments = &m_vVkImageViews[0];
                ci.width = Desc.Size.width;
                ci.height = Desc.Size.height;
                ci.layers = 1;
                ci.renderPass = m_vkRenderPass;
                VK_ERR(m_VkDevice.CreateObject(ci, nullptr, &m_vkFramebuffer));
            }

            Vulkan::InitInfo(&m_vkBeginInfo, VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);
            m_vkBeginInfo.clearValueCount = m_vVkClearValues.GetCount();
            m_vkBeginInfo.pClearValues = &m_vVkClearValues[0];
            m_vkBeginInfo.framebuffer = m_vkFramebuffer;
            m_vkBeginInfo.renderPass = m_vkRenderPass;

            m_vkBeginInfo.renderArea.extent.width = Desc.Size.width;
            m_vkBeginInfo.renderArea.extent.height = Desc.Size.height;
            m_vkBeginInfo.renderArea.offset.x = 0;
            m_vkBeginInfo.renderArea.offset.y = 0;

            return VKE_OK;
        }

        void CRenderPass::Begin(const VkCommandBuffer& vkCb)
        {
            //assert(m_state != State::BEGIN);
            auto& ICD = m_pCtx->_GetDevice().GetICD();

            VkClearValue ClearValue;
            ClearValue.color.float32[ 0 ] = ( float )( rand() % 100 ) / 100.0f;
            ClearValue.color.float32[ 1 ] = ( float )( rand() % 100 ) / 100.0f;
            ClearValue.color.float32[ 2 ] = ( float )( rand() % 100 ) / 100.0f;
            ClearValue.color.float32[ 3 ] = ( float )( rand() % 100 ) / 100.0f;
            ClearValue.depthStencil.depth = 0.0f;
            m_vkBeginInfo.clearValueCount = 1;
            m_vkBeginInfo.pClearValues = &ClearValue;
            // $TID BeginRenderPass: rp={(void*)this}, cb={vkCb}, rp={m_vkBeginInfo.renderPass}, fb={m_vkBeginInfo.framebuffer}
            ICD.vkCmdBeginRenderPass(vkCb, &m_vkBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
            //m_state = State::BEGIN;
        }

        void CRenderPass::End(const VkCommandBuffer& vkCb)
        {
            //assert(m_state == State::BEGIN);
            auto& ICD = m_pCtx->_GetDevice().GetICD();
            ICD.vkCmdEndRenderPass(vkCb);
            //m_state = State::END;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER