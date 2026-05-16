cmake_minimum_required(VERSION 3.10)
project(RayTracing)

include_directories(/usr/include/eigen3)          # Standard Linux (Ubuntu, etc.)
include_directories(/usr/local/include/eigen3)    # Intel Mac
include_directories(/opt/homebrew/include/eigen3) # Apple Silicon Mac (M1/M2/M3)

## Enable C++14
set(CMAKE_CXX_STANDARD 17)
find_package(Eigen3 REQUIRED)
find_package(OpenCV REQUIRED)

# OpenMP
find_package(OpenMP)
if(OpenMP_CXX_FOUND)

    add_executable(RayTracing main.cpp Object.hpp Vector.cpp Vector.hpp Sphere.hpp global.hpp Triangle.hpp Scene.cpp
    Scene.hpp BVH.cpp BVH.hpp Bounds3.hpp Ray.hpp Material.hpp Intersection.hpp
    Renderer.cpp Renderer.hpp)

    target_link_libraries(RayTracing PUBLIC OpenMP::OpenMP_CXX)
    target_link_libraries(RayTracing PRIVATE Eigen3::Eigen ${OpenCV_LIBRARIES})

    message(STATUS "OpenMP found. Compiling with OpenMP support.")

else()
    add_executable(RayTracing main.cpp Object.hpp Vector.cpp Vector.hpp Sphere.hpp global.hpp Triangle.hpp Scene.cpp
    Scene.hpp BVH.cpp BVH.hpp Bounds3.hpp Ray.hpp Material.hpp Intersection.hpp
    Renderer.cpp Renderer.hpp)

    target_link_libraries(RayTracing PRIVATE Eigen3::Eigen ${OpenCV_LIBRARIES})

    message(STATUS "OpenMP not found. Compiling without OpenMP support.")

endif()

