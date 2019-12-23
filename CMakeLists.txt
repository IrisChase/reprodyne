cmake_minimum_required(VERSION 3.12)

project(Reprodyne LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(reprodyne
user-include/reprodyne.h
schema_generated.h
harness.cpp
)

set_target_properties(reprodyne PROPERTIES PUBLIC_HEADER "user-include/reprodyne.h")

find_package(ZLIB REQUIRED)
find_package(Flatbuffers REQUIRED)
find_package(Catch2)
target_link_libraries(reprodyne ${ZLIB_LIBRARIES})


if(DEFINED Catch2_FOUND)
    add_executable(reprodyne_tests user-include/reprodyne.h tests.cpp)
    target_link_libraries(reprodyne_tests reprodyne)
endif()


add_custom_command(DEPENDS schema.fbs
                   COMMAND flatbuffers::flatc --cpp schema.fbs
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
