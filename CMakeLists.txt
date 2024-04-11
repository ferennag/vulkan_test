cmake_minimum_required(VERSION 3.27)
project(vulkan_test C)

set(CMAKE_C_STANDARD 17)

include(FetchContent)
FetchContent_Declare(std
        GIT_REPOSITORY https://github.com/ferennag/c-std.git
        GIT_TAG main
)
FetchContent_MakeAvailable(std)

find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
find_package(Vulkan REQUIRED)

add_executable(vulkan_test main.c
        src/core/input.h
        src/core/input.c
        src/defines.h
        src/renderer/vulkan.h
        src/renderer/vulkan.c
        src/renderer/physical_device.c
        src/renderer/physical_device.h
        src/renderer/vulkan_types.h
        src/renderer/device.c
        src/renderer/device.h
        src/renderer/instance.c
        src/renderer/instance.h)
target_compile_options(vulkan_test PRIVATE -g -Wall)
target_include_directories(vulkan_test PUBLIC src)
target_link_libraries(vulkan_test Vulkan::Vulkan SDL2::SDL2 std)

function(add_shaders TARGET_NAME)
    set(SHADER_SOURCE_FILES ${ARGN}) # the rest of arguments to this function will be assigned as shader source files

    # Validate that source files have been passed
    list(LENGTH SHADER_SOURCE_FILES FILE_COUNT)
    if (FILE_COUNT EQUAL 0)
        message(FATAL_ERROR "Cannot create a shaders target without any source files")
    endif ()

    set(SHADER_COMMANDS)
    set(SHADER_PRODUCTS)

    foreach (SHADER_SOURCE IN LISTS SHADER_SOURCE_FILES)
        set(SHADER_COMMAND)
        cmake_path(ABSOLUTE_PATH SHADER_SOURCE NORMALIZE)
        cmake_path(GET SHADER_SOURCE FILENAME SHADER_NAME)

        # Build command
        list(APPEND SHADER_COMMAND COMMAND)
        list(APPEND SHADER_COMMAND Vulkan::glslc)
        list(APPEND SHADER_COMMAND "${SHADER_SOURCE}")
        list(APPEND SHADER_COMMAND "-o")
        list(APPEND SHADER_COMMAND "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}.spv")

        # Add product
        list(APPEND SHADER_PRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}.spv")

        list(APPEND SHADER_COMMANDS ${SHADER_COMMAND})
    endforeach ()

    add_custom_target(${TARGET_NAME} ALL
            ${SHADER_COMMANDS}
            COMMENT "Compiling Shaders [${TARGET_NAME}]"
            SOURCES ${SHADER_SOURCE_FILES}
            BYPRODUCTS ${SHADER_PRODUCTS}
    )
endfunction()

add_shaders(vulkan_demo_shaders shaders/vertex.glsl shaders/fragment.glsl)