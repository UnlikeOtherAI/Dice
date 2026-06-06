# Android shared library: libdice3d.so = dice3d C++ core + JNI bridge, linked
# against Filament's Android native static libs (GLES backend via Engine DEFAULT).
# Driven by scripts/build_android.sh (standalone) or gradle externalNativeBuild.
cmake_minimum_required(VERSION 3.24)
project(dice3d_android LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED FILAMENT_DIR)
    message(FATAL_ERROR "FILAMENT_DIR must be set (-DFILAMENT_DIR=/path/to/filament)")
endif()
if(NOT EXISTS "${FILAMENT_DIR}/include/filament/Engine.h")
    message(FATAL_ERROR "Filament headers not found under ${FILAMENT_DIR}/include")
endif()

set(DICE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

# glm (header-only) — mirrors core/CMakeLists; Filament's binary ships no glm.
include(FetchContent)
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.1)
FetchContent_MakeAvailable(glm)

file(GLOB_RECURSE CORE_SRC ${DICE_ROOT}/core/src/*.cpp)

add_library(dice3d SHARED
    ${CORE_SRC}
    ${CMAKE_CURRENT_SOURCE_DIR}/dice3d_jni.cpp
)

target_include_directories(dice3d PRIVATE
    ${DICE_ROOT}/core/include
    ${FILAMENT_DIR}/include
    ${glm_SOURCE_DIR}
)
target_compile_definitions(dice3d PRIVATE DICE3D_HAVE_FILAMENT=1)
# Filament headers need C++17; core uses exceptions (std::runtime_error) so leave them on.

set(FL ${FILAMENT_DIR}/lib/${ANDROID_ABI})

# Filament's static archives have circular references → wrap in a link group.
target_link_libraries(dice3d
    -Wl,--start-group
    ${FL}/libfilament.a
    ${FL}/libbackend.a
    ${FL}/libbluevk.a
    ${FL}/libfilabridge.a
    ${FL}/libfilaflat.a
    ${FL}/libgeometry.a
    ${FL}/libsmol-v.a
    ${FL}/libibl.a
    ${FL}/libutils.a
    ${FL}/libabseil.a
    ${FL}/libzstd.a
    -Wl,--end-group
    android
    log
    EGL
    GLESv3
)
