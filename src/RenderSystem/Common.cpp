#include "RenderSystem/Common.h"

#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Vulkan.h"
#endif

namespace VKE
{
    namespace RenderSystem
    {
        const SColor SColor::ZERO(0.0f);
        const SColor SColor::ONE(1.0f);
        const SColor SColor::RED = SColor(1, 0, 0, 1);
        const SColor SColor::GREEN = SColor(1, 0, 0, 1);
        const SColor SColor::BLUE = SColor(1, 0, 0, 1);
        const SColor SColor::ALPHA = SColor(0, 0, 0, 1);

#if VKE_VULKAN_RENDERER
        void SColor::CopyToNative(void* pNativeArray) const
        {
            VkClearColorValue* pClear = reinterpret_cast< VkClearColorValue* >( pNativeArray );
            pClear->float32[ 0 ] = floats[ 0 ];
            pClear->float32[ 1 ] = floats[ 1 ];
            pClear->float32[ 2 ] = floats[ 2 ];
            pClear->float32[ 3 ] = floats[ 3 ];
        }
#endif
    } // RenderSystem
} // VKE