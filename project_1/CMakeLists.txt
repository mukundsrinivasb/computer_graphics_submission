cmake_minimum_required(VERSION 3.10)
project("Project - 1")

# Force C++14 or higher (Required for make_unique)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Find Eigen3
find_package(Eigen3 REQUIRED)

# 2. Find OpenGL and GLU
find_package(OpenGL REQUIRED)

# 3. Find GLFW
# (Note: On some Linux distributions, find_package(glfw3) might need pkg_check_modules instead, 
# but this is the standard cross-platform approach)
find_package(glfw3 REQUIRED)

# Create the executable (Assuming your file is named task3_cubism_animal.cpp)
add_executable(task3_cubism_animal task3_cubism_animal.cpp model.cpp model.hpp render.cpp render.hpp)

# Link all the libraries to your executable
target_link_libraries(task3_cubism_animal PRIVATE
    Eigen3::Eigen
    OpenGL::GL
    OpenGL::GLU
    glfw
)
