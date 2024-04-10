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
        src/renderer/vulkan.c)
target_compile_options(vulkan_test PRIVATE -g -Wal)
target_include_directories(vulkan_test PUBLIC src)
target_link_libraries(vulkan_test Vulkan::Vulkan SDL2::SDL2 std)
