cmake_minimum_required(VERSION 3.0)
project(StaticJSON)

if (NOT MSVC)
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG(-std=c++17 CXX17_COMPILER)
    CHECK_CXX_COMPILER_FLAG(-std=c++14 CXX14_COMPILER)
    if (CXX17_COMPILER)
        add_compile_options(-std=c++17)
        add_definitions(-DSTATICJSON_OPTIONAL)
    elseif (CXX14_COMPILER)
        add_compile_options(-std=c++14)
        add_definitions(-DSTATICJSON_OPTIONAL)
    else ()
        add_compile_options(-std=c++11)
    endif ()
    add_compile_options(-fno-rtti -Wall -Wextra -pedantic -g)
endif ()

if (NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE RelWithDebInfo)
endif ()


set (POSSIBLE_RAPIDJSON_DIR "${PROJECT_SOURCE_DIR}/rapidjson-1.1.0/include")
find_path(RAPIDJSON_INCLUDE_DIR rapidjson/rapidjson.h PATHS ${POSSIBLE_RAPIDJSON_DIR})
if (NOT RAPIDJSON_INCLUDE_DIR)
    set (RAPIDJSON_FILENAME ${PROJECT_SOURCE_DIR}/rapidjson.tgz)
    file(DOWNLOAD "https://github.com/miloyip/rapidjson/archive/v1.1.0.tar.gz" ${RAPIDJSON_FILENAME}
        SHOW_PROGRESS EXPECTED_HASH SHA256=bf7ced29704a1e696fbccf2a2b4ea068e7774fa37f6d7dd4039d0787f8bed98e)
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xvf ${RAPIDJSON_FILENAME} WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
    find_path(RAPIDJSON_INCLUDE_DIR rapidjson/rapidjson.h PATHS ${POSSIBLE_RAPIDJSON_DIR})
endif()
include_directories(SYSTEM ${RAPIDJSON_INCLUDE_DIR})

include_directories(include autojsoncxx)
set(SOURCE_FILES src/staticjson.cpp)
add_library(staticjson ${SOURCE_FILES})


