cmake_minimum_required(VERSION 3.10.0)
project(CG_expri4 VERSION 0.1.0 LANGUAGES C CXX)

if (MSVC)
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    add_definitions(-D_UNICODE -DUNICODE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(lib/glad/include)
add_library(glad STATIC lib/glad/src/glad.c)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(lib/glfw)

file(GLOB_RECURSE SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

if(NOT SRC_FILES)
    message(FATAL_ERROR "No source files found in src/ directory. Please add source files or update CMakeLists.txt")
endif()

add_executable(${PROJECT_NAME} ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC
    lib/glad/include
    lib
    lib/glm
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    # GLFW 的头文件目录会通过 target_link_libraries 自动添加
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    glad      # 链接我们自己创建的 glad 库
    glfw      # 链接 CMake 为我们构建的 glfw 库
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PUBLIC opengl32.lib)
endif()