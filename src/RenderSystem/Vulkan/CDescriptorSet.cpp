#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/CDescriptorSet.h"

namespace VKE
{
    namespace RenderSystem
    {
        Result CDescriptorSet::Init(const SDescriptorSetDesc& Desc)
        {
            Result res = VKE_OK;
            this->m_hObject = CalcHash( Desc );
            return res;
        }

        hash_t CDescriptorSet::CalcHash( const SDescriptorSetDesc& Desc )
        {
            SHash Hash;
            for( uint32_t i = 0; i < Desc.vpLayouts.GetCount(); ++i )
            {
                Hash += Desc.vpLayouts[i]->GetHandle();
            }
            Hash += Desc.vpLayouts.GetCount();
            return Hash.value;
        }

        Result CDescriptorSetLayout::Init(const SDescriptorSetLayoutDesc& Desc)
        {
            Result res = VKE_OK;
            this->m_hObject = CalcHash( Desc );
            return res;
        }

        hash_t CDescriptorSetLayout::CalcHash( const SDescriptorSetLayoutDesc& Desc )
        {
            SHash Hash;
            for( uint32_t i = 0; i < Desc.vBindings.GetCount(); ++i )
            {
                const auto& Binding = Desc.vBindings[ i ];
                Hash.Combine( Binding.count, Binding.idx, Binding.stages, Binding.type );
            }
            Hash += Desc.vBindings.GetCount();
            return Hash.value;
        }

    } // RenderSystem
} // VKE

#endif // VKE_VULKAN_RENDERER