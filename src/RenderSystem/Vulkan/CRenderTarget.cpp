#include "RenderSystem/CRenderTarget.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/Vulkan/CResourceManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        CRenderTarget::CRenderTarget(CDeviceContext* pCtx) :
            m_pCtx(pCtx)
        {}

        CRenderTarget::~CRenderTarget()
        {
            Destroy();
        }

        void CRenderTarget::Destroy(bool destroyRenderPass)
        {
            if( m_vkFramebuffer == VK_NULL_HANDLE )
            {
                return;
            }

            const auto& VkDevice = m_pCtx->_GetDevice();
            VkDevice.DestroyObject(nullptr, &m_vkFramebuffer);
            if( destroyRenderPass )
            {
                VkDevice.DestroyObject(nullptr, &m_vkRenderPass);
            }

            auto& ResMgr = m_pCtx->GetResourceManager();

            for( uint32_t i = 0; i < m_vTextureViewHandles.GetCount(); ++i )
            {
                if( m_vTextureViewHandles[i].created )
                {
                    ResMgr.DestroyTextureView(m_vTextureViewHandles[ i ].hView);
                }
            }
            m_vTextureViewHandles.FastClear();

            for( uint32_t i = 0; i < m_vTextureHandles.GetCount(); ++i )
            {
                if( m_vTextureHandles[ i ].created )
                {
                    ResMgr.DestroyTexture(m_vTextureHandles[ i ].hTex);
                }
            }
            m_vTextureHandles.FastClear();
            m_vClearValues.FastClear();
        }

        void ConvertTexDescToAttachmentDesc(const VkImageCreateInfo& TexDesc,
                                            RENDER_TARGET_WRITE_ATTACHMENT_USAGE usage,
                                            VkAttachmentDescription* pOut)
        {
            pOut->initialLayout = Vulkan::Convert::ImageUsageToInitialLayout(TexDesc.usage);
            pOut->finalLayout = Vulkan::Convert::ImageUsageToFinalLayout(TexDesc.usage);
            pOut->format = TexDesc.format;
            pOut->loadOp = Vulkan::Convert::UsageToLoadOp(usage);
            pOut->storeOp = Vulkan::Convert::UsageToStoreOp(usage);
            pOut->samples = TexDesc.samples;
        }

        void ConvertTexDescToAttachmentDesc(const VkImageCreateInfo& TexDesc,
                                            RENDER_TARGET_READ_ATTACHMENT_USAGE usage,
                                            VkAttachmentDescription* pOut)
        {
            pOut->initialLayout = Vulkan::Convert::ImageUsageToInitialLayout(TexDesc.usage);
            pOut->finalLayout = Vulkan::Convert::ImageUsageToFinalLayout(pOut->initialLayout);
            pOut->format = TexDesc.format;
            pOut->loadOp = Vulkan::Convert::UsageToLoadOp(usage);
            pOut->storeOp = Vulkan::Convert::UsageToStoreOp(usage);
            pOut->samples = TexDesc.samples;
        }

        const VkImageCreateInfo* CRenderTarget::_AddTextureView(TextureViewHandle hTextureView)
        {
            auto& ResMgr = m_pCtx->GetResourceManager();
            assert(!hTextureView.IsNativeHandle() && "Native handles are not supported here");
            VkImageView vkView = ResMgr.GetTextureView(hTextureView);
            const auto& ViewDesc = ResMgr.GetTextureViewDesc(hTextureView);
            VkImage vkImg = ViewDesc.image;
            m_vImgViews.PushBack(vkView);
            const VkImageCreateInfo* pTexDesc = nullptr;
            auto hTex = ResMgr._FindTexture(vkImg);
            if( hTex )
            {
                STexture Tex;
                Tex.hTex = hTex;
                m_vTextureHandles.PushBack(Tex);
                pTexDesc = &ResMgr.GetTextureDesc( hTex );
            }
            else
            {
                pTexDesc = &ResMgr.GetTextureDesc(vkImg);
            }

            STextureView TexView = { hTextureView };
            m_vTextureViewHandles.PushBack(TexView);
            return pTexDesc;
        }

        const VkImageCreateInfo* CRenderTarget::_CreateTextureView(const STextureDesc& TexDesc)
        {
            auto& ResMgr = m_pCtx->GetResourceManager();
            VkImage vkImage;
            auto hTex = ResMgr.CreateTexture(TexDesc, &vkImage);
            assert(hTex);
            VkImageView vkView;
            auto hView = ResMgr.CreateTextureView(hTex, &vkView);
            assert(hView);
            const VkImageCreateInfo* pTexDesc = &ResMgr.GetTextureDesc(hTex);
            m_vImgViews.PushBack(vkView);
            m_vTextureHandles.PushBack( STexture(hTex, true) );
            m_vTextureViewHandles.PushBack( STextureView(hView, true) );
            return pTexDesc;
        }

        void CRenderTarget::Clear(const SColor& ClearColor, float clearDepth, float clearStencil)
        {
            for( uint32_t i = 0; i < m_vClearValues.GetCount(); ++i )
            {
                ClearColor.CopyToNative(&m_vClearValues[ 0 ].color);
                m_vClearValues[ 0 ].depthStencil.depth = clearDepth;
                m_vClearValues[ 0 ].depthStencil.stencil = clearStencil;
            }
        }

        Result CRenderTarget::Update(const SRenderTargetDesc& Desc)
        {
            Destroy();
            for( uint32_t i = 0; i < Desc.vWriteAttachments.GetCount(); ++i )
            {
                const auto& At = Desc.vWriteAttachments[ i ];
                if( At.hTextureView )
                {
                    _AddTextureView(At.hTextureView);
                }
                else
                {
                    _CreateTextureView(At.TexDesc);
                }
            }

            for( uint32_t i = 0; i < Desc.vReadAttachments.GetCount(); ++i )
            {
                const auto& At = Desc.vReadAttachments[ i ];
                if( At.hTextureView )
                {
                    _AddTextureView(At.hTextureView);
                }
            }

            auto& ResMgr = m_pCtx->GetResourceManager();

            if( Desc.DepthStencilAttachment.hWrite )
            {
                VkImageView vkView = ResMgr.GetTextureView(Desc.DepthStencilAttachment.hWrite);
                m_vImgViews.PushBack(vkView);
            }

            if( Desc.DepthStencilAttachment.hRead )
            {
                if( Desc.DepthStencilAttachment.hRead != Desc.DepthStencilAttachment.hWrite )
                {
                    VkImageView vkView = ResMgr.GetTextureView(Desc.DepthStencilAttachment.hRead);
                    m_vImgViews.PushBack(vkView);
                }
            }

            const auto& VkDevice = m_pCtx->_GetDevice();
            VkDevice.DestroyObject(nullptr, &m_vkFramebuffer);
            {
                VkFramebufferCreateInfo ci;
                Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
                ci.flags = 0;
                ci.attachmentCount = m_vImgViews.GetCount();
                ci.pAttachments = &m_vImgViews[ 0 ];
                ci.width = Desc.Size.width;
                ci.height = Desc.Size.height;
                ci.layers = 1;
                ci.renderPass = m_vkRenderPass;
                VK_ERR(VkDevice.CreateObject(ci, nullptr, &m_vkFramebuffer));
            }
            return VKE_OK;
        }

        Result CRenderTarget::Create(const SRenderTargetDesc& Desc)
        {
            Destroy();

            m_Desc = Desc;
            const auto& vWriteAttachments = Desc.vWriteAttachments;
            const auto& vReadAttachments = Desc.vReadAttachments;

            Utils::TCDynamicArray< const VkImageCreateInfo* > vpTexDescs;
            Utils::TCDynamicArray< VkAttachmentDescription > vVkAtDescs;      

            AttachmentRefArray vWriteColorRefs;
            VkAttachmentReference vkDepthWriteRef, vkDepthReadRef;
            AttachmentRefArray vInputRefs;

            VkAttachmentDescription vkAtDesc;
            vkAtDesc.flags = 0;
            vkAtDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            vkAtDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            uint32_t writeCount = 0, readCount = 0;

            auto& ResMgr = m_pCtx->GetResourceManager();
            const auto& VkDevice = m_pCtx->_GetDevice();

            for( uint32_t i = 0; i < vWriteAttachments.GetCount(); ++i )
            {
                const auto& AtDesc = vWriteAttachments[ i ];
                const VkImageCreateInfo* pTexDesc;

                if( !AtDesc.hTextureView )
                {
                    pTexDesc = _CreateTextureView(AtDesc.TexDesc);
                }
                else
                {
                    pTexDesc = _AddTextureView(AtDesc.hTextureView);
                }

                ConvertTexDescToAttachmentDesc(*pTexDesc, AtDesc.usage, &vkAtDesc);
                writeCount = vVkAtDescs.PushBack(vkAtDesc);

                VkAttachmentReference Ref;
                Ref.attachment = writeCount;
                Ref.layout = vkAtDesc.initialLayout;
                vWriteColorRefs.PushBack(Ref);
                VkClearValue vkClear;
                AtDesc.ClearColor.CopyToNative(&vkClear.color);
                m_vClearValues.PushBack(vkClear);
            }

            for( uint32_t i = 0; i < vReadAttachments.GetCount(); ++i )
            {
                // Find if this attachment is as write attachment
                bool bFoundWrite = false;
                for( uint32_t j = 0; j < vWriteAttachments.GetCount(); ++j )
                {
                    if( vWriteAttachments[ j ].hTextureView == vReadAttachments[ i ].hTextureView )
                    {
                        const auto& Ref = vWriteColorRefs[ j ];
                        vInputRefs.PushBack(Ref);
                        bFoundWrite = true;
                        break;
                    }
                }

                if( bFoundWrite )
                    continue;

                const auto& AtDesc = vReadAttachments[ i ];
                const auto& TexDesc = ResMgr.FindTextureDesc(AtDesc.hTextureView);
                ConvertTexDescToAttachmentDesc(TexDesc, AtDesc.usage, &vkAtDesc);
                readCount = vVkAtDescs.PushBack(vkAtDesc);

                VkAttachmentReference Ref;
                Ref.attachment = readCount;
                Ref.layout = vkAtDesc.initialLayout;
                vInputRefs.PushBack(Ref);

                VkImageView vkView = ResMgr.GetTextureView(AtDesc.hTextureView);
                m_vImgViews.PushBack(vkView);
            }

            for( uint32_t i = 0; i < vVkAtDescs.GetCount(); ++i )
            {
                const auto& AtDesc = vVkAtDescs[ i ];
            }

            if( Desc.DepthStencilAttachment.hWrite )
            {
                const auto& TexDesc = ResMgr.FindTextureDesc(Desc.DepthStencilAttachment.hWrite);
                ConvertTexDescToAttachmentDesc(TexDesc, Desc.DepthStencilAttachment.usage, &vkAtDesc);
                vkDepthWriteRef.attachment = vVkAtDescs.PushBack(vkAtDesc);
                vkDepthWriteRef.layout = vkAtDesc.finalLayout;

                VkImageView vkView = ResMgr.GetTextureView(Desc.DepthStencilAttachment.hWrite);
                m_vImgViews.PushBack(vkView);
                VkClearValue vkClear;
                vkClear.depthStencil.depth = Desc.DepthStencilAttachment.clearValue;
                vkClear.depthStencil.stencil = Desc.DepthStencilAttachment.clearValue;
                m_vClearValues.PushBack(vkClear);
            }

            if( Desc.DepthStencilAttachment.hRead )
            {
                if( Desc.DepthStencilAttachment.hRead == Desc.DepthStencilAttachment.hWrite )
                {
                    vkDepthReadRef = vkDepthWriteRef;
                    vInputRefs.PushBack(vkDepthReadRef);
                }
                else if( !Desc.DepthStencilAttachment.hWrite )
                {
                    const auto& TexDesc = ResMgr.FindTextureDesc(Desc.DepthStencilAttachment.hRead);
                    ConvertTexDescToAttachmentDesc(TexDesc, Desc.DepthStencilAttachment.usage, &vkAtDesc);
                    vkDepthReadRef.attachment = vVkAtDescs.PushBack(vkAtDesc);
                    vkDepthReadRef.layout = vkAtDesc.finalLayout;
                    vInputRefs.PushBack(vkDepthReadRef);
                    VkImageView vkView = ResMgr.GetTextureView(Desc.DepthStencilAttachment.hRead);
                    m_vImgViews.PushBack(vkView);
                }
            }

            /// @todo for now only one subpass is supported
            VkSubpassDescription vkSpDesc = { 0 };
            vkSpDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            if( !vWriteColorRefs.IsEmpty() )
            {
                vkSpDesc.colorAttachmentCount = vWriteColorRefs.GetCount();
                vkSpDesc.pColorAttachments = &vWriteColorRefs[ 0 ];
            }
            if( !vInputRefs.IsEmpty() )
            {
                vkSpDesc.inputAttachmentCount = vInputRefs.GetCount();
                vkSpDesc.pInputAttachments = &vInputRefs[ 0 ];
            }
            if( Desc.DepthStencilAttachment.hWrite )
                vkSpDesc.pDepthStencilAttachment = &vkDepthWriteRef;

            {
                VkRenderPassCreateInfo ci;
                Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);
                ci.attachmentCount = vVkAtDescs.GetCount();
                ci.pAttachments = &vVkAtDescs[ 0 ];
                ci.dependencyCount = 0;
                ci.pDependencies = nullptr;
                ci.subpassCount = 1;
                ci.pSubpasses = &vkSpDesc;
                ci.flags = 0;
                VK_ERR(VkDevice.CreateObject(ci, nullptr, &m_vkRenderPass));
            }
            {
                VkFramebufferCreateInfo ci;
                Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
                ci.flags = 0;
                ci.attachmentCount = m_vImgViews.GetCount();
                ci.pAttachments = &m_vImgViews[ 0 ];
                ci.width = Desc.Size.width;
                ci.height = Desc.Size.height;
                ci.layers = 1;
                ci.renderPass = m_vkRenderPass;
                VK_ERR(VkDevice.CreateObject(ci, nullptr, &m_vkFramebuffer));
            }

            Vulkan::InitInfo(&m_vkBeginInfo, VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);
            m_vkBeginInfo.clearValueCount = m_vClearValues.GetCount();
            m_vkBeginInfo.pClearValues = &m_vClearValues[ 0 ];
            m_vkBeginInfo.framebuffer = m_vkFramebuffer;
            m_vkBeginInfo.renderPass = m_vkRenderPass;
            
            m_vkBeginInfo.renderArea.extent.width = Desc.Size.width;
            m_vkBeginInfo.renderArea.extent.height = Desc.Size.height;
            m_vkBeginInfo.renderArea.offset.x = 0;
            m_vkBeginInfo.renderArea.offset.y = 0;

            return VKE_OK;
        }

        void CRenderTarget::Begin(const VkCommandBuffer& vkCb)
        {
            assert(m_state != State::BEGIN);
            auto& ICD = m_pCtx->_GetDevice().GetICD();

            ICD.vkCmdBeginRenderPass(vkCb, &m_vkBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
            m_state = State::BEGIN;
        }

        void CRenderTarget::End(const VkCommandBuffer& vkCb)
        {
            assert(m_state == State::BEGIN);
            auto& ICD = m_pCtx->_GetDevice().GetICD();
            ICD.vkCmdEndRenderPass(vkCb);
            m_state = State::END;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER