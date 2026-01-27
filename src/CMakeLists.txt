add_executable(cc30
    driver/main.cpp
)

target_link_libraries(cc30
    PRIVATE
    LLVM::Core
    LLVM::Support
)

# Semantic Analysis libraries
# add_library(cc30_semantic ...)
# target_link_libraries(cc30 PRIVATE cc30_semantic)
