cmake_minimum_required(VERSION 3.20)
project(sb-viewer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# macOS 26 SDK removed AGL.framework; Qt 6.8's FindWrapOpenGL still links it
# (vestigial legacy-Carbon GL API, unused by Qt6). Drop it from the interface.
if(APPLE AND TARGET WrapOpenGL::WrapOpenGL)
    set_property(TARGET WrapOpenGL::WrapOpenGL PROPERTY INTERFACE_LINK_LIBRARIES "-framework OpenGL")
endif()

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.hpp)

add_executable(${PROJECT_NAME} ${SRC_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets)
