cmake_minimum_required(VERSION 3.14)
project(mini_range_coder_test)

# GoogleTest requires at least C++11
set(CMAKE_CXX_STANDARD 11)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -Wall -Wextra")

enable_testing()

add_executable(
  mini_range_coder_test
  test.cpp
  ../MiniRangeCoder.c
)
target_link_libraries(
  mini_range_coder_test
  gtest_main
)

include(GoogleTest)
gtest_discover_tests(mini_range_coder_test)
