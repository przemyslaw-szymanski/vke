#pragma once

#include "Common.h"

#include "RenderSystem/Resources/CShader.h"
#include "RenderSystem/CDescriptorSet.h"

namespace VKE::RenderSystem
{
    // Forward declarations
    class CPipelineManager;

    class VKE_API CPipelineLayout
    {
        friend class CPipelineManager;
        VKE_ADD_DDI_OBJECT( RHI::PipelineLayout );
        VKE_DECL_BASE_OBJECT( PipelineLayoutHandle );

    public:
        CPipelineLayout( CPipelineManager* pMgr ) : m_pMgr( pMgr )
        {
        }

        Result Init( const SPipelineLayoutDesc& Desc );

        const SPipelineLayoutDesc& GetDesc() const
        {
            return m_Desc;
        }

    protected:
        SPipelineLayoutDesc m_Desc;
        CPipelineManager*   m_pMgr;
    };

    using PipelineLayoutPtr    = Utils::TCWeakPtr< CPipelineLayout >;
    using PipelineLayoutRefPtr = Utils::TCObjectSmartPtr< CPipelineLayout >;

    class VKE_API CPipeline : public Core::TCResource< CPipeline >
    {
        friend class CPipelineManager;
        friend class CDeviceContext;
        friend class CGraphicsContext;
        friend class CComputeContext;
        friend class CCommandBuffer;

        VKE_ADD_DDI_OBJECT( RHI::Pipeline );
        VKE_DECL_BASE_OBJECT( PipelineHandle );

    public:
        CPipeline( CPipelineManager* );
        ~CPipeline();

        Result Init( const SPipelineDesc& Desc );

        PIPELINE_TYPE GetType() const
        {
            return m_type;
        }

        const SPipelineDesc& GetDesc() const
        {
            return m_Desc;
        }

        PipelineLayoutPtr GetLayout() const
        {
            return m_pLayout;
        }

    protected:
        void _Destroy();

    protected:
        SPipelineDesc        m_Desc;
        PipelineLayoutRefPtr m_pLayout;
        CPipelineManager*    m_pMgr;
        PIPELINE_TYPE        m_type;
        bool                 m_isActive = false;
    };

    using PipelinePtr    = Utils::TCWeakPtr< CPipeline >;
    using PipelineRefPtr = Utils::TCObjectSmartPtr< CPipeline >;

} // namespace VKE::RenderSystem
