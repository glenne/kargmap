# Standard CMake header config.
cmake_minimum_required(VERSION 3.14.0)
set(CMAKE_CXX_STANDARD 11)

#==============================================================================
# Dependencies when enable_testing
include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.8.0
)

FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)
  FetchContent_Populate(googletest)
    # Prevent GoogleTest from overriding our compiler/linker options
    # when building with Visual Studio
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    # Prevent GoogleTest from using PThreads
    set(gtest_disable_pthreads ON CACHE BOOL "" FORCE)

    # adds the targers: gtest, gtest_main, gmock, gmock_main
  add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
  # Silence std::tr1 warning on MSVC
    if(MSVC)
      foreach(_tgt gtest gtest_main gmock gmock_main)
        target_compile_definitions(${_tgt}
          PRIVATE
            "_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING"
          )
      endforeach()
    endif()
endif()
include_directories (INTERFACE ${googletest_SOURCE_DIR}/googletest/include)

#==============================================================================
# Project definition.

add_executable(KArgMapTest
               KArgMapTest.cpp
              )

# Set the KArgMap project as a dependency to this project.
# This requires that the target is built and will use it as a library.
target_link_libraries( KArgMapTest
    PRIVATE KArgMap
    gtest_main
)

target_include_directories(KArgMapTest
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../kargmap
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${googletest_SOURCE_DIR}
)

#==============================================================================
add_executable(KArgMapCborTest
               KArgMapCborTest.cpp
              )

# Set the KArgMap project as a dependency to this project.
# This requires that the target is built and will use it as a library.
target_link_libraries( KArgMapCborTest
    PRIVATE KArgMap
    PRIVATE KArgMapCbor
    gtest_main
)

target_include_directories(KArgMapCborTest
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../kargmap
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../serializers
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${googletest_SOURCE_DIR}
)

#==============================================================================
# test config
add_test(
    NAME  KArgMapTest_UNIT_TEST
    COMMAND  "$<TARGET_FILE:KArgMapTest>" --gtest_output=xml:${CMAKE_BINARY_DIR}/KArgMapTest_UnitTest_Results.xml
)
