#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/CDescriptorSet.h"

namespace VKE
{
    namespace RenderSystem
    {
        Result CDescriptorSet::Init(const SDescriptorSetDesc& Desc, hash_t hash)
        {
            Result res = VKE_OK;
            if( hash == 0 )
            {
                Hash::Combine( &hash, Desc.pLayout.Get() );
                Hash::Combine( &hash, Desc.type );
            }
            SHandle Handle;
            Handle.hash = hash;
            Handle.type = Desc.type;
            this->m_hObjHandle = Handle.value;
            return res;
        }

        Result CDescriptorSetLayout::Init(const SDescriptorSetLayoutDesc& Desc)
        {
            Result res = VKE_OK;
            SHash Hash;
            for( uint32_t i = 0; i < Desc.vBindings.GetCount(); ++i )
            {
                const auto& Binding = Desc.vBindings[ i ];
                Hash.Combine( Binding.count, Binding.idx, Binding.stages, Binding.type );
            }
            this->m_hObjHandle = Hash.value;
            return res;
        }
    } // RenderSystem
} // VKE

#endif // VKE_VULKAN_RENDERER