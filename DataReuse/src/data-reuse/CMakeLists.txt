cmake_minimum_required(VERSION 2.8)

add_library(data-reuse MODULE
    Loop.cpp
    Kernel.cpp
    DRA_Visitor.cpp
    DRA_ASTConsumer.cpp
    DataReuse.cpp
)

if(LLVM_ENABLE_PLUGINS AND (WIN32 OR CYGWIN))
    target_link_libraries(data-reuse ${cmake_2_8_12_PRIVATE}
        clangAST
        clangBasic
        clangFrontend
        LLVMSupport
    )
endif()
