#pragma once

#include "RenderSystem/Common.h"
#include "Core/Utils/TCDynamicArray.h"
#include "Core/Utils/TCConstantArray.h"
#include "Core/VKEConfig.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CGraphicsContext;
        class CRenderingPipeline;
        class CRenderPass;
        class CTexture;
        class CTextureView;

        namespace Managers
        {
            struct SBackBufferManagerDesc
            {
                uint32_t    backBufferCount = 3;
            };

            class VKE_API CBackBufferManager final
            {
                friend class CGraphicsContext;

                template<class HandleType, class DataType>
                using DataContainer = vke_hash_map< HandleType, DataType >;

                template<class HandleType, class DataType>
                using ContainerArray = DataContainer< HandleType, DataType >[ Config::MAX_BACK_BUFFER_COUNT ];
                using CustomDataArray = ContainerArray< uint32_t, void* >;
                using CustomDataContainer = DataContainer< uint32_t, void* >;
                
                struct SBackBuffers
                {
                    CustomDataArray    aCustomData;
                };

                struct SBackBuffer
                {
                    CustomDataContainer     CustomData;
                };

                struct SBackBufferDataPointers
                {
                    CRenderingPipeline*     pRenderingPipeline = nullptr;
                };

                struct SBackBufferDataHandles
                {
                    //RenderingPipelineHandle hRenderingPipeline = NULL_HANDLE;
                };

                public:

                    CBackBufferManager(CGraphicsContext* pCtx);
                    ~CBackBufferManager();

                    Result Create(const SBackBufferManagerDesc& Desc);
                    void Destroy();

                    uint32_t AcquireNextBuffer();
                    
                    uint32_t GetBackBufferCount() const { return m_Desc.backBufferCount;}

                    uint32_t AddCustomData(uint8_t* aData, uint32_t elementSize);
                    void* GetCustomData(uint32_t idx);
                    void UpdateCustomData(uint32_t backBufferIdx, uint32_t dataIdx, void* pData);
                    void UpdateCustomData(uint32_t dataIdx, uint8_t* aData, uint32_t elementSize);

                    template<class T>
                    uint32_t AddCustomData(T* aData)
                    {
                        return AddCustomData( reinterpret_cast< uint8_t* >( aData ), sizeof( T ) );
                    }

                    template<class T>
                    void UpdateCustomData( uint32_t dataIdx, T* aData )
                    {
                        UpdateCustomData( dataIdx, reinterpret_cast< uint8_t* >( aData ), sizeof( T ) );
                    }

                    template<class T>
                    void UpdateCustomData(uint32_t backBufferIdx, uint32_t dataIdx, T* aData )
                    {
                        UpdateCustomData( backBufferIdx, dataIdx, reinterpret_cast< uint8_t* >( aData ), sizeof( T ) );
                    }

                    template<class T>
                    T* GetCustomData(uint32_t dataIdx)
                    {
                        return reinterpret_cast< T* >( GetCustomData( dataIdx ) );
                    }

                protected:

                    void _Destroy();

                protected:

                    SBackBufferManagerDesc  m_Desc;
                    CGraphicsContext*       m_pCtx = nullptr;
                    SBackBuffer             m_BackBuffers[ Config::MAX_BACK_BUFFER_COUNT ];
                    uint32_t                m_customDataIdx = 0;
                    SBackBufferDataPointers m_CurrBackBufferPtrs;
                    SBackBufferDataHandles  m_CurrBackBufferHandles;
                    uint16_t                m_currBackBufferIndex = 0;
                    uint16_t                m_backBufferCount = 0;
            };
        } // Managers
    } // RenderSystem
} // VKE