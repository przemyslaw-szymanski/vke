#ifndef __VKE_PREPROCESSOR_H__
#define __VKE_PREPROCESSOR_H__

#if defined(DEBUG) || defined(_DEBUG)
#   define VKE_DEBUG 1
#endif // DEBUG

#if VKE_WINDOWS
#   define VKE_OS VKE_WINDOWS
#elif VKE_LINUX
#   define VKE_OS VKE_LINUX
#elif VKE_ANDROID
#   define VKE_OS VKE_ANDROID
#endif // VKE_WINDOWS

#ifndef VKE_OS
#   error "Unsupported system or system not defined in CMakeLists.txt"
#endif // VKE_OS

#if WIN64 || _WIN64 || __x86_64__ || __ppc64__
#   define VKE_ARCHITECTURE_64 1
#   define VKE_ARCHITECTURE VKE_ARCHITECTURE_64
#else
#   define VKE_ARCHITECTURE_32 1
#   define VKE_ARCHITECTURE VKE_ARCHITECTURE_32
#endif

#if defined(_MSC_VER)
#   define VKE_COMPILER_VISUAL_STUDIO 1
#   define VKE_COMPILER VKE_COMPILER_VISUAL_STUDIO
#elif defined(__GNUC__)
#   if defined(__MINGW32__)
#       define VKE_COMPILER_MINGW 1
#       define VKE_COMPILER VKE_COMPILER_MINGW
#   else // defined(__MINGW32__)
#       define VKE_COMPILER_GCC 1
#       define VKE_COMPILER VKE_COMPILER_GCC
#   endif // defined(__MINGW32__)
#else
#   error "Unsupported compiler"
#endif // MSC_VER

#define VKE_NEW_NT new(std::nothrow)

#if VKE_COMPILER_VISUAL_STUDIO
#   define VKE_NEW_DBG new(_NORMAL_BLOCK, __FILE__, __LINE__)
#   define VKE_MALLOC_DBG(_size) _malloc_dbg((_size), _NORMAL_BLOCK, __FILE__, __LINE__)
#   define VKE_FREE_DBG(_ptr) _free_dbg((_ptr), _NORMAL_BLOCK)
#elif VKE_COMPILER_GCC
#   define VKE_NEW_DBG new
#   define VKE_MALLOC_DBG(_size) malloc((_size))
#   define VKE_FREE_DBG free
#endif

#if VKE_DEBUG
#   define VKE_NEW VKE_NEW_DBG
#   define VKE_MALLOC VKE_MALLOC_DBG
#   define VKE_FREE VKE_FREE_DBG
#else
#   define VKE_NEW VKE_NEW_NT
#   define VKE_MALLOC malloc
#   define VKE_FREE free
#endif // VK_DEBUG

#define VKE_CODE(_exp) do{ _exp }while(0,0)
#define VKE_DELETE(_ptr) VKE_CODE( delete( _ptr); (_ptr) = nullptr; )
#define VKE_DELETE_ARRAY(_ptr) VKE_CODE( delete[] (_ptr); (_ptr) = nullptr; )

#define VKE_SUCCEEDED(_exp) (_exp) == VKE::VKE_OK
#define VKE_FAILED(_exp) (_exp) != VKE::VKE_OK
#define VKE_RETURN_IF_FAILED(_exp) VKE_CODE(Result err = (_exp); if(VKE_FAILED(err)) { return err; })

#define VKE_TO_STRING(_exp) #_exp
#define VKE_CONCAT(_a, _b) _a##_b

#define VKE_LINE __LINE__
#define VKE_FILE __FILE__
#if VKE_COMPILER_VISUAL_STUDIO
#   define VKE_FUNCTION __FUNCTION__
#elif VKE_COMPILER_GCC
#   define VKE_FUNCTION __func__
#else
#   define VKE_FUNCTION "unknown"
#endif // COMPILER

#define vke_inline inline
#if VKE_COMPILER_VISUAL_STUDIO
#   define vke_force_inline __forceinline
#elif VKE_COMPILER_GCC || VKE_COMPILER_MINGW
#   define vke_force_inline __attribute__((always_inline))
#endif // COMPILER

#if VKE_COMPILER_VISUAL_STUDIO
#   define VKE_DLL_EXPORT   __declspec(dllexport)
#   define VKE_DLL_IMPORT   __declspec(dllimport)
#   define VKE_TEMPLATE_EXPORT(_type) template _type VKE_API
#   define VKE_TEMPLATE_IMPORT(_type) extern template _type VKE_API
#else
#   define VKE_DLL_EXPORT
#   define VKE_DLL_IMPORT
#   define VKE_TEMPLATE_EXPORT(_type)
#   define VKE_TEMPLATE_IMPORT(_type)
#endif // CPMPILER

#if VKE_API_EXPORT
#   define VKE_API  VKE_DLL_EXPORT
#   define VKE_TEMPLATE_OBJ_API(_type) VKE_TEMPLATE_EXPORT(_type)
#else
#   define VKE_API  VKE_DLL_IMPORT
#   define VKE_TEMPLATE_OBJ_API(_type) VKE_TEMPLATE_IMPORT(_type)
#endif // VKE_API_EXPORT

#if VKE_DEBUG
#   define VKE_STL_TRY(_exp, _ret) try { _exp; }catch(std::exception&) { return _ret; }
#else
#   define VKE_STL_TRY(_exp, _ret) VKE_CODE( _exp; )
#endif // DEBUG

// Warnings
#if VKE_COMPILER_VISUAL_STUDIO
#pragma warning( once : 4251 ) // template needs to have dll-interface to be used by clients of class
#pragma warning( disable: 4251 )
#endif // VKE_COMPILER_VISUAL_STUDIO

#define VKE_ZERO_MEM(_data) VKE::ZeroMem(_data)

#if VKE_DEBUG
#   define VKE_DEBUG_CODE(_code)  _code
#else
#   define VKE_DEBUG_CODE(_code)
#endif // VKE_DEBUG

#if VKE_WINDOWS
#   define VKE_USE_DIRECTX_MATH 1
#endif

#ifndef VULKAN_RENDERER
#define VULKAN_RENDERER 1
#endif // VULKAN_RENDERER

#define BEGIN_ENGINE_NAMESPACE namespace VKE {
#define END_ENGINE_NAMESPACE } // VKE

#define BEGIN_NAMESPACE(_ns) \
    BEGIN_ENGINE_NAMESPACE \
        namespace _ns {

#define END_NAMESPACE \
        } \
        END_ENGINE_NAMESPACE


#endif // __VKE_PREPROCESSOR_H__