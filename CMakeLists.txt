cmake_minimum_required(VERSION 3.10)
project(OpenCVProjects)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add subdirectories
add_subdirectory(task1_conveyor_inspection)
add_subdirectory(task2_formula_recognition)

# Print project information
message(STATUS "========================================")
message(STATUS "  OpenCV Projects - Multi-Task Build")
message(STATUS "========================================")
message(STATUS "Task 1: Conveyor Inspection System")
message(STATUS "Task 2: Formula Recognition System")
message(STATUS "========================================")
