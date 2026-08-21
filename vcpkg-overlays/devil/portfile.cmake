vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO DentonW/DevIL
    REF v1.8.0
    SHA512 4aed5e50a730ece8b1eb6b2f6204374c6fb6f5334cf7c880d84c0f79645ea7c6b5118f57a7868a487510fc59c452f51472b272215d4c852f265f58b5857e17c7
    HEAD_REF master
    PATCHES
        0001_fix-encoding.patch
        0002_fix-missing-mfc-includes.patch
        0003_fix-openexr.patch
        enable-static.patch
        0004_compatible-jasper-2-0-20.patch
        0005-fix-pkgconfig.patch
        0006-fix-ilut-header.patch
        jasper-4.patch
        0007-remove_register_keyword_cpp17.patch
)

file(REMOVE "${SOURCE_PATH}/DevIL/src-IL/cmake/FindOpenEXR.cmake")

# The MSVC .def file exports 'ilTexImageSurface', but that function is compiled
# with C++ linkage (it is not declared in any public C header / extern "C" block),
# so its real symbol is name-mangled. MSVC tolerates this, but MinGW's ld fails
# with "cannot export ilTexImageSurface: symbol not defined". Remove the stale
# export entry (the function is not part of the public C API anyway).
vcpkg_replace_string(
    "${SOURCE_PATH}/DevIL/src-IL/msvc/il.def"
    "\nilTexImageSurface\n"
    "\n"
)

# ILUT is the DevIL OpenGL/Win32 utility toolkit. It is not needed for the core
# image loader, has a stale '.def' export (ilutWinLoadUrl), and fails to link
# under MinGW because its OpenGL renderer is not linked against opengl32.
# Skip building it entirely.
vcpkg_replace_string(
    "${SOURCE_PATH}/DevIL/CMakeLists.txt"
    "add_subdirectory(src-ILUT)"
    "# add_subdirectory(src-ILUT) # disabled by overlay port"
)

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
    INVERTED_FEATURES
    libpng  IL_NO_PNG
    tiff    IL_NO_TIF
    libjpeg IL_NO_JPG
    openexr IL_NO_EXR
    jasper  IL_NO_JP2
    lcms    IL_NO_LCMS
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}/DevIL"
    DISABLE_PARALLEL_CONFIGURE
    OPTIONS
         ${FEATURE_OPTIONS}
        -DIL_NO_MNG=ON
        -DIL_USE_DXTC_NVIDIA=OFF
        -DIL_USE_DXTC_SQUISH=OFF
)

vcpkg_cmake_install()
vcpkg_copy_pdbs()

vcpkg_fixup_pkgconfig()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

# Handle copyright
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
