cmake_minimum_required(VERSION 3.12)

project(Reprodyne LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(reprodyne
user-include/reprodyne.h
harness.cpp
)

set_target_properties(reprodyne PROPERTIES PUBLIC_HEADER "user-include/reprodyne.h")
target_include_directories(reprodyne PRIVATE ${FLATBUFFERS_INCLUDE})

find_package(ZLIB REQUIRED)
target_link_libraries(reprodyne ZLIB)

add_executable(reprodyne_tests user-include/reprodyne.h tests.cpp)
target_link_libraries(reprodyne_tests reprodyne)


if(NOT DEFINED FLATBUFFERS_INCLUDE)
    message(WARNING "Include directory for flatbuffers not set! Unless you know what you're doing, "
                    "use -DFLATBUFFERS_INCLUDE=xxx")
endif()

if(NOT DEFINED FLATBUFFERS_COMPILER)
    message(FATAL_ERROR "Path to flatbuffers schema compiler not set! Use -DFLATBUFFERS_COMPILER=/.../flatc")
endif()

add_custom_command(DEPENDS schema.fbs
                   COMMAND ${FLATBUFFERS_COMPILER} --cpp schema.fbs
                   OUTPUT schema_generated.h
                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

#You'll definitely want to build this target if you wanna work with the code.
#This gets rebuilt every time you compile but the flatbuffers compiler is fast,
# and there's only one translation unit anyway.
add_custom_target(generated_code DEPENDS schema_generated.h)
add_dependencies(reprodyne generated_code)

install(TARGETS reprodyne
        ARCHIVE
            DESTINATION lib
        PUBLIC_HEADER
            DESTINATION include/reprodyne)
