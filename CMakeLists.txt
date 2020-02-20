cmake_minimum_required(VERSION 3.12)

project(Reprodyne LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(reprodyne
user-include/reprodyne.h

errorstuff.h

scopehandlers.h
scopehandlers.cpp

scopecontainers.h
scopecontainers.cpp

fileformat.h
programhandlers.h
programhandlers.cpp

reprodyne.cpp

schema_generated.h
)

set_target_properties(reprodyne PROPERTIES PUBLIC_HEADER "user-include/reprodyne.h")
target_compile_definitions(reprodyne PUBLIC REPRODYNE_AVAILABLE)

find_package(ZLIB REQUIRED)
find_package(Flatbuffers REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Catch2)

target_link_libraries(reprodyne ${ZLIB_LIBRARIES})
target_link_libraries(reprodyne ${OPENSSL_CRYPTO_LIBRARY})

if(DEFINED Catch2_FOUND)
    add_executable(reprodyne_tests user-include/reprodyne.h tests.cpp catchmain.cpp)
    target_link_libraries(reprodyne_tests reprodyne)
endif()


add_custom_command(DEPENDS schema.fbs
                   COMMAND flatbuffers::flatc --cpp schema.fbs
                   OUTPUT schema_generated.h
                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

#You'll definitely want to build this target if you wanna work with the code.
#This gets rebuilt every time you compile but the flatbuffers compiler is p.fast.
add_custom_target(generated_code DEPENDS schema_generated.h)
add_dependencies(reprodyne generated_code)

install(TARGETS reprodyne EXPORT reprodyne
        ARCHIVE
            DESTINATION lib/reprodyne1 #reprodyne MAJOR_VERSION...
        PUBLIC_HEADER
            DESTINATION include/reprodyne1)

#Why can't I just add this to the above? It's a   ~ m y s t e r y ~
install(FILES reprodyne-config.cmake DESTINATION lib/cmake/reprodyne)

#Why can't I just add this to the above? It's a   ~ m y s t e r y ~
install(EXPORT reprodyne DESTINATION lib/cmake/reprodyne)

