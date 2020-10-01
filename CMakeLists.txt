cmake_minimum_required(VERSION 3.14)
project(GrafTemplateProject)

set(CMAKE_CXX_STANDARD 17)

add_executable(
        GrafTemplateProject
        framework.cpp
        framework.h
        Skeleton.cpp
)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

# Included these two lines:

include_directories(${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})
target_link_libraries(GrafTemplateProject ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})