cmake_minimum_required(VERSION 3.14.0)
set(CMAKE_CXX_STANDARD 11)

#==============================================================================
# Load MicroCbor from git
include(FetchContent)

FetchContent_Declare(
  kargmap
  GIT_REPOSITORY https://github.com/glenne/kargmap.git
  #GIT_REPOSITORY file:///Users/glenne/git/kargmap
  GIT_TAG        main
)
FetchContent_MakeAvailable(kargmap)
#==============================================================================

add_executable(CborExample
               CborExample.cpp
              )
target_link_libraries(CborExample
     KArgMap
     KArgMapCbor
)
