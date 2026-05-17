# newest features used: FetchContent v3.11, FetchContent_MakeAvailable v3.14
cmake_minimum_required(VERSION 3.14)

# test for population first in case of use in parent project
if(NOT cmake_utils_POPULATED)
  if(NOT COMMAND FetchContent_Declare OR
      NOT COMMAND FetchContent_MakeAvailable
    )
    include(FetchContent)
  endif()
  FetchContent_Declare(cmake_utils
    GIT_REPOSITORY https://github.com/allelomorph/cmake_utils.git
    # ExternalProject_Add defaults to origin/master up to at least cmake 3.30, see:
    #   - https://cmake.org/cmake/help/v3.30/module/ExternalProject.html#git
    GIT_TAG        main  # origin/main
  )
  FetchContent_MakeAvailable(cmake_utils)
  list(APPEND CMAKE_MODULE_PATH ${cmake_utils_SOURCE_DIR})
endif()
