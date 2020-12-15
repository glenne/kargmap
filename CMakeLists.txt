# Debug
#function(dump_cmake_vars)
#    get_cmake_property(_variableNames VARIABLES)
#    foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}"
#    )
#    endforeach()
#endfunction()


cmake_minimum_required(VERSION 3.14.0)
set(CMAKE_CXX_STANDARD 11)
option(BUILD_KARGMAP_TESTS "Build the KArgMap testing tree." OFF)
option(BUILD_ALL_TESTS "Build the KArgMap testing tree." OFF)

#==============================================================================
include(FetchContent)

FetchContent_Declare(
  microcbor
  #GIT_REPOSITORY file:///Users/glenne/git/microcbor
  GIT_REPOSITORY https://github.com/glenne/microcbor.git
  GIT_TAG        main
)
FetchContent_MakeAvailable(microcbor)

#==============================================================================
# Set the main project name. For Visual Studio, this will create a Solution
# file.

project(KArgMap)

enable_testing()

#==============================================================================
add_library(KArgMap INTERFACE)
target_include_directories(KArgMap INTERFACE include/)

add_library(KArgMapCbor INTERFACE)
target_link_libraries(KArgMapCbor INTERFACE KArgMap microcbor)

if (BUILD_ALL_TESTS OR BUILD_KARGMAP_TESTS)
    add_subdirectory(test)
endif()

# Debug
#dump_cmake_vars()
