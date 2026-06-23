#include "RenderSystem/Common.h"

#if VKE_COMPILE_VULKAN_RHI
#include "RenderSystem/Vulkan/Vulkan.h"
#endif

namespace VKE
{
    namespace RenderSystem
    {
        const SColor SColor::ZERO = SColor( 0.0f, 0.0f, 0.0f, 0.0f );
        const SColor SColor::ONE( 1.0f );
        const SColor SColor::RED   = SColor( 1, 0, 0, 1 );
        const SColor SColor::GREEN = SColor( 0, 1, 0, 1 );
        const SColor SColor::BLUE  = SColor( 0, 0, 1, 1 );
        const SColor SColor::ALPHA = SColor( 0, 0, 0, 1 );

        const SColor DebugColors::UPLOAD = SColor();

#if VKE_COMPILE_VULKAN_RHI
        void SColor::CopyToNative( void* pNativeArray ) const
        {
            VkClearColorValue* pClear = reinterpret_cast< VkClearColorValue* >( pNativeArray );
            pClear->float32[ 0 ]      = floats[ 0 ];
            pClear->float32[ 1 ]      = floats[ 1 ];
            pClear->float32[ 2 ]      = floats[ 2 ];
            pClear->float32[ 3 ]      = floats[ 3 ];
        }
#endif
    } // namespace RenderSystem
} // namespace VKE