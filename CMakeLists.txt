// Copyright 2020 Iris Chase
//
// Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.

cmake_minimum_required(VERSION 3.12)

project(Reprodyne LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(REPRODYNE_MAJOR_VERSION 1)
set(REPRODYNE_MINOR_VERSION 0)
set(REPRODYNE_PATCH_VERSION 0)

set(REPRODYNE_API_VERSION ${REPRODYNE_MAJOR_VERSION}.${REPRODYNE_MINOR_VERSION})
set(REPRODYNE_VERSION ${REPRODYNE_API_VERSION}.${REPRODYNE_PATCH_VERSION})

set(REPRODYNE_USER_INCLUDE_DIRS ${CMAKE_INSTALL_PREFIX}/include/reprodyne${REPRODYNE_API_VERSION})

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

${CMAKE_CURRENT_BINARY_DIR}/schema_generated.h
)

set_target_properties(reprodyne PROPERTIES PUBLIC_HEADER "${PROJECT_SOURCE_DIR}/user-include/reprodyne.h")
target_compile_definitions(reprodyne PUBLIC REPRODYNE_AVAILABLE)
target_include_directories(reprodyne PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

find_package(ZLIB REQUIRED)

find_package(Flatbuffers REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Catch2)

target_link_libraries(reprodyne ${ZLIB_LIBRARIES})
target_link_libraries(reprodyne ${OPENSSL_CRYPTO_LIBRARY})

add_custom_command(DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/schema.fbs
                   COMMAND flatbuffers::flatc --cpp ${CMAKE_CURRENT_SOURCE_DIR}/schema.fbs
                   OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/schema_generated.h )

if(DEFINED Catch2_FOUND)
    add_executable(reprodyne_tests
        tests/catchmain.cpp
        tests/oopsiewhoopsie.h

        tests/tests.cpp
        tests/videotests.cpp
        )
    target_link_libraries(reprodyne_tests reprodyne)
endif()



install(TARGETS reprodyne EXPORT reprodyne-targets
        ARCHIVE
            DESTINATION lib/reprodyne${REPRODYNE_VERSION}
        PUBLIC_HEADER
            DESTINATION ${REPRODYNE_USER_INCLUDE_DIRS})

include(CMakePackageConfigHelpers)

write_basic_package_version_file(reprodyne-config-version.cmake
    VERSION ${REPRODYNE_VERSION}
    COMPATIBILITY SameMajorVersion)

configure_file(reprodyne-config.cmake.in
    ${CMAKE_BINARY_DIR}/reprodyne-config.cmake @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/reprodyne-config.cmake ${CMAKE_BINARY_DIR}/reprodyne-config-version.cmake
    DESTINATION lib/cmake/reprodyne${REPRODYNE_VERSION})

install(EXPORT reprodyne-targets
    DESTINATION lib/cmake/reprodyne${REPRODYNE_VERSION})
