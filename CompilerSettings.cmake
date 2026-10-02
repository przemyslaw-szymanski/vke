add_compile_definitions(
    VKE_VULKAN=1
    VKE_D3D12=2
    VKE_METAL=3 )

function(EnableOption option)
	if( ${option} )
		add_compile_definitions(${option}=1)
	else()
		add_compile_definitions(${option}=0)
	endif()
endfunction()

if(VKE_WINDOWS)
    add_compile_definitions(VKE_WINDOWS=1)
elseif(VKE_LINUX)
    add_compile_definitions(VKE_LINUX=1)
elseif(VKE_ANDROID)
    add_compile_definitions(VKE_ANDROID=1)
else()
    message(FATAL_ERROR "Unknown system")
endif()

EnableOption(VKE_DEBUG_INFO)
EnableOption(VKE_USE_XINPUT)
EnableOption(VKE_COMPILE_VULKAN_RHI)
EnableOption(VKE_COMPILE_D3D12_RHI)
EnableOption(VKE_USE_DIRECTX_MATH)
EnableOption(VKE_RENDERER_DEBUG)
EnableOption(VKE_SCENE_DEBUG)
EnableOption(VKE_USE_SSE2)
EnableOption(VKE_USE_RIGHT_HANDED_COORDINATES)
EnableOption(VKE_USE_DEVIL)
EnableOption(VKE_USE_DIRECTXTEX)
EnableOption(VKE_USE_DIRECTX_SHADER_COMPILER)
EnableOption(VKE_USE_GLSL_COMPILER)
EnableOption(VKE_USE_HLSL_SYNTAX)
EnableOption(VKE_USE_GAINPUT)
EnableOption(VKE_USE_RAW_INPUT)
EnableOption(VKE_SCENE_TERRAIN_DEBUG)
EnableOption(VKE_ASSERT_ENABLE)
EnableOption(VKE_RENDER_SYSTEM_MEMORY_DEBUG)
EnableOption(VKE_MEMORY_DEBUG)

# Handle RHI static linking requirements
set(VKE_RENDER_SYSTEM "Vulkan" CACHE STRING "RHI used at runtime / linked statically")
set_property(CACHE VKE_RENDER_SYSTEM PROPERTY STRINGS Vulkan D3D12)

if(VKE_RENDER_SYSTEM STREQUAL "Vulkan")
    if(NOT VKE_COMPILE_VULKAN_RHI)
        message(FATAL_ERROR "VKE_RENDER_SYSTEM=Vulkan requires VKE_COMPILE_VULKAN_RHI=ON")
    endif()
    add_compile_definitions(VKE_RENDER_SYSTEM=VKE_VULKAN)
elseif(VKE_RENDER_SYSTEM STREQUAL "D3D12")
    if(NOT VKE_COMPILE_D3D12_RHI)
        message(FATAL_ERROR "VKE_RENDER_SYSTEM=D3D12 requires VKE_COMPILE_D3D12_RHI=ON")
    endif()
    add_compile_definitions(VKE_RENDER_SYSTEM=VKE_D3D12)
else()
    message(FATAL_ERROR "Unknown VKE_RENDER_SYSTEM '${VKE_RENDER_SYSTEM}' (expected Vulkan or D3D12)")
endif()

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
	add_compile_definitions(VKE_COMPILER_CLANG=1)
	set(CLANG 1)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
	add_compile_definitions(VKE_COMPILER_MINGW=1)
	set(GCC 1)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
	add_compile_definitions(VKE_COMPILER_VISUAL_STUDIO=1)
	set(MSVC 1)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CLANG OR GCC)
	if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
		# clang-cl: MSVC-compatible driver, use MSVC-style flags
		add_definitions("/W4 /WX /EHsc")

		# ignore warnings
		add_definitions("/wd4201") # nameless union/struct
		add_definitions("/wd4127") # conditional expression is constant
		add_definitions("/wd4533") # initialization of '' skipped by goto
		add_definitions("/wd4100") # unreferenced formal parameter
		add_definitions("/wd4505") # unreferenced local function has been removed
		add_definitions("/wd4221") # This object file does not define any previously undefined public symbols, so it will not be used by any link operation that consumes this library
	else()
		# GCC common and GNU-style clang driver
		add_definitions("-Wall") # Covers /W4
		add_definitions("-Wextra") # Covers /W4
		add_definitions("-Wfatal-errors") # Any warning/error/notice treat as fatal (fatal stops compilation)
	endif()

	if(VKE_DEBUG_INFO)
		add_definitions("-g")
	endif()

	# ignore warnings to match MSVC ignore by default
	add_definitions("-Wno-unused-function")
	add_definitions("-Wno-unused-variable")
	add_definitions("-Wno-unused-parameter")
	add_definitions("-Wno-unused-but-set-variable")
	
	add_definitions("-Wno-switch")                     # When switch(myEnumType) doesn't cover all ENUM values in 'case' or use 'default'.
	add_definitions("-Wno-missing-field-initializers") # Ignore incomplete initializers eg.: struct foo { int a, int b, int c }, foo x = { 1, 2 } // no c;
	add_definitions("-Wno-unused-local-typedefs")      # Defined but never used typedefs

	if(CLANG)
		# Clang (GCC frontend) specific flags to match GCC and MinGW
		add_definitions("-Wno-ignored-reference-qualifiers")               # Excessive const (when 'const' keyword has no effect)
		add_definitions("-Wno-tautological-undefined-compare")             # Logical: obvious conditions (this != nullptr, true == true)
		add_definitions("-Wno-tautological-constant-out-of-range-compare") # Logical: conditions that never reach, eg.: UINT32_MAX > UINT8_MAX
		add_definitions("-Wno-unused-template")                            # Defined template function, never used
		add_definitions("-Wno-unused-lambda-capture")                      # Defined lambda, never used
		add_definitions("-Wno-pessimizing-move")                           # Moving a temporary object prevents copy elision
		add_definitions("-Wno-microsoft-unqualified-friend")               # Unqualified friend declaration referring to type outside of the nearest enclosing namespace is a Microsoft extension
		add_definitions("-Wno-deprecated-copy-with-user-provided-copy")    # Overloaded operator= on class/struct with copy constructor
		add_definitions("-Wno-extern-c-compat")                            # Empty struct has size 0 in C, size 1 in C++
		add_definitions("-Wno-missing-braces")                             # Allows myVec3D = { 0, 0, 0 } instead of myVec3D = Vector3D{ 0, 0, 0 }
	endif()

elseif(MSVC)
	add_definitions(-DVKE_COMPILER_VISUAL_STUDIO=1)

	add_definitions("/MP /W4 /WX /EHsc")
    add_definitions("/Zc:__cplusplus /permissive /volatile:iso /utf-8")

    if(VKE_DEBUG_INFO)
        add_definitions("/Zi")
        add_definitions("/DEBUG")
    endif()

	# Allows __VA_OPT__ use in macros
	add_definitions("/Zc:preprocessor")
	
	# ignore warnings
	add_definitions("/wd4201") # nameless union/struct
	add_definitions("/wd4127") # conditional expression is constant
	add_definitions("/wd4533") # initialization of '' skipped by goto
	add_definitions("/wd4100") # unreferenced formal parameter
	add_definitions("/wd4505") # unreferenced local function has been removed
	add_definitions("/wd4221") # This object file does not define any previously undefined public symbols, so it will not be used by any link operation that consumes this library

endif()

set(PREPROCESSOR_DEFINITIONS IL_STATIC_LIB)
