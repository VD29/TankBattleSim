cmake_minimum_required(VERSION 3.20)
project(TankSim VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# --- Engine library (header-only ECS + systems + utils) ---
add_library(tank_engine INTERFACE)
target_include_directories(tank_engine INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# --- Main executable ---
add_executable(tank_sim src/main.cpp)
target_link_libraries(tank_sim PRIVATE tank_engine)

# --- nlohmann/json: prefer system install, fall back to FetchContent ---
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v3.11.3
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()
target_link_libraries(tank_engine INTERFACE nlohmann_json::nlohmann_json)

# --- Google Test: prefer system install, fall back to FetchContent ---
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

enable_testing()
add_subdirectory(tests)
