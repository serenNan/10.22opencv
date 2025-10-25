cmake_minimum_required(VERSION 3.10)
project(ConveyorInspection)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenCV
find_package(OpenCV REQUIRED)

# Include directories
include_directories(${OpenCV_INCLUDE_DIRS})

# Add executables (CLI versions)
add_executable(conveyor_inspection_cli conveyor_inspection_cli.cpp)
add_executable(formula_recognition_cli formula_recognition.cpp)

# Link libraries
target_link_libraries(conveyor_inspection_cli ${OpenCV_LIBS})
target_link_libraries(formula_recognition_cli ${OpenCV_LIBS})

# Print OpenCV information
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
