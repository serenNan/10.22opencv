cmake_minimum_required(VERSION 3.10)
project(ConveyorInspection)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find OpenCV
find_package(OpenCV REQUIRED)

# Include directories
include_directories(${OpenCV_INCLUDE_DIRS})

# Add main executable
add_executable(conveyor_inspection_cli
    main.cpp
    conveyor_inspector.cpp
)

# Add HSV tuner tool
add_executable(hsv_tuner
    hsv_tuner.cpp
)

add_executable(hsv_tuner_headless
    hsv_tuner_headless.cpp
)

# Link libraries
target_link_libraries(conveyor_inspection_cli ${OpenCV_LIBS})
target_link_libraries(hsv_tuner ${OpenCV_LIBS})
target_link_libraries(hsv_tuner_headless ${OpenCV_LIBS})

# Print build targets
message(STATUS "Build targets: conveyor_inspection_cli, hsv_tuner, hsv_tuner_headless")

# Print OpenCV information
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
