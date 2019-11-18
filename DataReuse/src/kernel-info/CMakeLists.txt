cmake_minimum_required(VERSION 2.8)

add_library(kernel-info MODULE
    KernelInfo.cpp
)

if(LLVM_ENABLE_PLUGINS AND (WIN32 OR CYGWIN))
    target_link_libraries(kernel-info ${cmake_2_8_12_PRIVATE}
        clangAST
        clangBasic
        clangFrontend
        LLVMSupport
    )
endif()
