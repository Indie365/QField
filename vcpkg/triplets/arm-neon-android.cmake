set(VCPKG_TARGET_ARCHITECTURE arm)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
if(PORT MATCHES "^qt" OR PORT STREQUAL "openssl")
    set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()
set(VCPKG_CMAKE_SYSTEM_NAME Android)
set(VCPKG_BUILD_TYPE release)
set(VCPKG_MAKE_BUILD_TRIPLET "--host=armv7a-linux-androideabi")
set(VCPKG_CMAKE_CONFIGURE_OPTIONS -DANDROID_ABI=armeabi-v7a -DANDROID_ARM_NEON=ON)
set(ANDROID_SDK_ROOT $ENV{ANDROID_SDK_ROOT})

set(VCPKG_CXX_FLAGS "-fstack-protector-strong")
set(VCPKG_C_FLAGS "-fstack-protector-strong")
