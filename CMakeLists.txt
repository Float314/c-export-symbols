cmake_minimum_required(VERSION 3.15)
project(c-export-symbols
    VERSION 1.0.0
    LANGUAGES CXX
    HOMEPAGE_URL https://github.com/Float314/c-export-symbols
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Download CPM, cause this shit doesnt come bundled in 
if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake")
    message("-- Downloading CPM v0.42.3")
    file(
        DOWNLOAD
        https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.42.3/CPM.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
    )
    message("-- CPM Successfully fetched!")
else()
    message("-- CPM is already installed")
endif()
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

include(FetchContent)

CPMAddPackage("gh:geode-sdk/json@3.3.0")

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

set(SOURCES
    src/c-parser.cpp
    src/main.cpp
)
message("SOURCES: ${SOURCES}")

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    mat-json
    clang-cpp
)