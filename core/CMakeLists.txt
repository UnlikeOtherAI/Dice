# --- Filament paths (set by build script per platform) ---
# FILAMENT_DIR must be set: -DFILAMENT_DIR=/path/to/filament
if(NOT DEFINED FILAMENT_DIR)
    message(FATAL_ERROR "FILAMENT_DIR must be set")
endif()

# --- Library target ---
add_library(dice3d STATIC
    src/geometry/polyhedra.cpp
    src/geometry/chamfer.cpp
    src/geometry/mesh_builder.cpp
    src/animation/quaternion_utils.cpp
    src/animation/animation_controller.cpp
    src/rendering/renderer.cpp
    src/rendering/face_mapper.cpp
    src/dice_scene.cpp
    src/dice3d_impl.cpp
)
target_include_directories(dice3d PUBLIC include)
target_include_directories(dice3d PRIVATE ${FILAMENT_DIR}/include)

# Enable real Filament renderer if headers are present
if(EXISTS "${FILAMENT_DIR}/include/filament/Engine.h")
    target_compile_definitions(dice3d PRIVATE DICE3D_HAVE_FILAMENT=1)
    message(STATUS "Filament found at ${FILAMENT_DIR} — full renderer enabled")
else()
    message(STATUS "Filament stub at ${FILAMENT_DIR} — renderer is a compile stub")
endif()

# --- Tests ---
if(BUILD_TESTING)
    include(FetchContent)

    # glm: used by the library headers (Filament ships its own in real builds;
    # we fetch it here for the stub/test build where FILAMENT_DIR has no headers)
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        1.0.1
    )
    FetchContent_MakeAvailable(glm)
    # Expose glm to both the library and the test executable
    target_include_directories(dice3d PRIVATE ${glm_SOURCE_DIR})

    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.2
    )
    FetchContent_MakeAvailable(Catch2)

    add_executable(dice3d_tests
        tests/test_geometry.cpp
        tests/test_chamfer.cpp
        tests/test_animation.cpp
        tests/test_face_mapper.cpp
    )
    target_include_directories(dice3d_tests PRIVATE ${glm_SOURCE_DIR})
    target_link_libraries(dice3d_tests PRIVATE dice3d Catch2::Catch2WithMain)
    include(Catch)
    catch_discover_tests(dice3d_tests)
endif()
