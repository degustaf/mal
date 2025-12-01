cmake_minimum_required (VERSION 3.21)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(
    mal
    LANGUAGES CXX
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")  # Save all runtime files to this directory.

file(
    GLOB_RECURSE LIB_FILES
    CONFIGURE_DEPENDS  # Automatically reconfigure if source files are added/removed.
    ${PROJECT_SOURCE_DIR}/lib/*.cpp
    ${PROJECT_SOURCE_DIR}/lib/*.hpp
    ${PROJECT_SOURCE_DIR}/include/*.hpp
)

file(
    GLOB_RECURSE SOURCE_FILES
    CONFIGURE_DEPENDS  # Automatically reconfigure if source files are added/removed.
    ${PROJECT_SOURCE_DIR}/src/*.cpp
    ${PROJECT_SOURCE_DIR}/include/*.hpp
)

# Enable warnings recommended for new projects.
if (MSVC)
    add_compile_options(/W2 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wconversion -Werror -pedantic -g)
endif()

add_library("${PROJECT_NAME}_lib" ${LIB_FILES})
target_include_directories("${PROJECT_NAME}_lib" PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
set_property(TARGET "${PROJECT_NAME}_lib" PROPERTY CXX_STANDARD 17)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

target_link_libraries(${PROJECT_NAME} "${PROJECT_NAME}_lib")
