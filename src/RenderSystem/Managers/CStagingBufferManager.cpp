#include "CStagingBufferManager.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/Vulkan/Managers/CDeviceMemoryManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        Result CStagingBufferManager::Create( const SStagingBufferManagerDesc& Desc )
        {
            Result ret = VKE_OK;
            m_Desc = Desc;
            return ret;
        }

        void CStagingBufferManager::Destroy()
        {

        }

        Result CStagingBufferManager::GetBuffer( const SBufferRequirementInfo& Info, SBufferData* pData )
        {
            Result ret = VKE_FAIL;
            handle_t hAllocation = NULL_HANDLE;
            SBindMemoryInfo BindInfo;
            auto& DeviceMemMgr = Info.pCtx->_GetDeviceMemoryManager();
            const uint32_t bufferSize = std::max( m_Desc.bufferSize, Info.size );

            for( uint32_t i = 0; i < m_vpBuffers.GetCount(); ++i )
            {
                
            }
            if( hAllocation == NULL_HANDLE )
            {
                SCreateBufferDesc BufferDesc;
                BufferDesc.Create.async = false;
                BufferDesc.Create.stages = Resources::StageBits::FULL_LOAD;
                BufferDesc.Buffer.memoryUsage = MemoryUsages::STAGING;
                BufferDesc.Buffer.size = bufferSize;
                BufferDesc.Buffer.usage = BufferUsages::TRANSFER_SRC;
                BufferRefPtr pBuffer = Info.pCtx->CreateBuffer( BufferDesc ); 
            }
            return ret;
        }

        uint32_t CStagingBufferManager::_FindBuffer( const SBufferRequirementInfo& Info )
        {
            uint32_t idx = UNDEFINED_U32;
            
            return idx;
        }
    } // RenderSystem
} // VKE