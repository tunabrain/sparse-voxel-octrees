cmake_minimum_required(VERSION 2.8)
project(sparse-voxel-octrees)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

include(CXX11)
check_for_cxx11_compiler(CXX11_COMPILER)

if(CXX11_COMPILER)
    enable_cxx11()
else()
    message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} seems to have no C++11 support. Please try again with a more recent compiler version.")
endif()

include(OptimizeForArchitecture)
OptimizeForArchitecture()

if (MSVC)
    add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Vc_ARCHITECTURE_FLAGS}")

if (CMAKE_COMPILER_IS_GNUCXX)
    set(CXX_WARNINGS "-Wall -Wextra -Wpointer-arith -Wcast-align -fstrict-aliasing -Wno-unused-local-typedefs")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_WARNINGS} -fvisibility-inlines-hidden")
endif()

find_package(SDL REQUIRED)

include_directories(src ${SDL_INCLUDE_DIR})
file(GLOB_RECURSE Sources "src/*.cpp" "src/*.c")
if (WIN32)
    add_executable(sparse-voxel-octrees WIN32 ${Sources})
else()
    add_executable(sparse-voxel-octrees ${Sources})
endif()
target_link_libraries(sparse-voxel-octrees ${SDLMAIN_LIBRARY} ${SDL_LIBRARY})
