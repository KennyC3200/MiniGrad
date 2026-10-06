cmake_minimum_required(VERSION 3.21)
project(minigrad
    VERSION 0.1.0
    DESCRIPTION "A C++ deep learning framework modeled on PyTorch's c10 core"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_property(MINIGRAD_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

# Settings that only apply when minigrad is built directly, not when another project pulls it in
# with add_subdirectory() or FetchContent
if(PROJECT_IS_TOP_LEVEL)
    # Write compile_commands.json for clangd. Only the Ninja and Makefile generators support this,
    # the Visual Studio generator silently ignores it (use the msvc-* presets instead)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

    # Default to a Release build if build type not specified
    # Multi-config generators (Visual Studio) ignore this, pass --config Release instead
    if(NOT MINIGRAD_IS_MULTI_CONFIG AND NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    endif()
endif()

# User configuration
# --------------------------------------------------------------------------------------------------
# Tests and examples are only built by default when minigrad is the top-level project
option(MINIGRAD_BUILD_TESTS "Build unit tests" ${PROJECT_IS_TOP_LEVEL})
option(MINIGRAD_BUILD_EXAMPLES "Build example programs" ${PROJECT_IS_TOP_LEVEL})
option(MINIGRAD_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)

# Compiler Warnings
# --------------------------------------------------------------------------------------------------
# Interface for all the warnings
add_library(minigrad_warnings INTERFACE)

# Add properties to the minigrad_warnings interface depending on the compiler
if(MSVC)
    target_compile_options(minigrad_warnings INTERFACE
        /W4
        /permissive-
        /Zc:__cplusplus
        $<$<BOOL:${MINIGRAD_WARNINGS_AS_ERRORS}>:/WX>
    )
else()
    target_compile_options(minigrad_warnings INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        -Wsign-conversion
        $<$<BOOL:${MINIGRAD_WARNINGS_AS_ERRORS}>:-Werror>
    )
endif()

# Core Library
# --------------------------------------------------------------------------------------------------
file(GLOB_RECURSE MINIGRAD_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

file(GLOB_RECURSE MINIGRAD_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")

add_library(minigrad STATIC ${MINIGRAD_SOURCES} ${MINIGRAD_HEADERS})
add_library(minigrad::minigrad ALIAS minigrad)

# Anything that links minigrad also needs C++20 to compile its headers
target_compile_features(minigrad PUBLIC cxx_std_20)

target_include_directories(minigrad
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(minigrad PRIVATE minigrad_warnings)

# Threading
find_package(Threads REQUIRED)
target_link_libraries(minigrad PUBLIC Threads::Threads)

# Testing
# --------------------------------------------------------------------------------------------------
if(MINIGRAD_BUILD_TESTS)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.4
        GIT_SHALLOW TRUE # Only download the tagged commit, not the whole history
    )
    FetchContent_MakeAvailable(Catch2)

    file(GLOB_RECURSE MINIGRAD_TEST_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
    )

    if(MINIGRAD_TEST_SOURCES)
        add_executable(minigrad_tests ${MINIGRAD_TEST_SOURCES})
        target_link_libraries(minigrad_tests PRIVATE
            minigrad::minigrad minigrad_warnings Catch2::Catch2WithMain
        )

        list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
        include(Catch)
        catch_discover_tests(minigrad_tests)
    endif()
endif()

# Examples
# --------------------------------------------------------------------------------------------------
if(MINIGRAD_BUILD_EXAMPLES)
    file(GLOB EXAMPLE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp")
    foreach(example_src ${EXAMPLE_SOURCES})
        get_filename_component(example_name ${example_src} NAME_WE)
        add_executable(${example_name} ${example_src})
        target_link_libraries(${example_name} PRIVATE minigrad::minigrad minigrad_warnings)
    endforeach()
endif()

# compile_commands.json
# --------------------------------------------------------------------------------------------------
# Copy it from the build directory to the project root on every build, so clangd finds it no
# matter which build directory is in use. The copy is gitignored.
if(PROJECT_IS_TOP_LEVEL AND CMAKE_GENERATOR MATCHES "^(Ninja|.* Makefiles)$")
    add_custom_target(minigrad_copy_compile_commands ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${PROJECT_SOURCE_DIR}/compile_commands.json
        COMMENT "Copying compile_commands.json to the project root"
        VERBATIM
    )
endif()

# Summary
# --------------------------------------------------------------------------------------------------
message(STATUS "minigrad config: ")
message(STATUS "  Generator: ${CMAKE_GENERATOR}")
if(MINIGRAD_IS_MULTI_CONFIG)
    message(STATUS "  Build types: ${CMAKE_CONFIGURATION_TYPES} (pick one with --config)")
else()
    message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
endif()
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Build Tests: ${MINIGRAD_BUILD_TESTS}")
message(STATUS "  Build Examples: ${MINIGRAD_BUILD_EXAMPLES}")
message(STATUS "  Warnings as errors: ${MINIGRAD_WARNINGS_AS_ERRORS}")
