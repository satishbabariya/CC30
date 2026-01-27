include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Define test executable
add_executable(cc30_tests
    unit/LexerTests.cpp
)

target_link_libraries(cc30_tests
    GTest::gtest_main
    LLVM::Core
    LLVM::Support
)

include(GoogleTest)
gtest_discover_tests(cc30_tests)
