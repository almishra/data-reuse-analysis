cmake_minimum_required(VERSION 2.8)

add_library(data-analysis MODULE
    ForList.cpp
    DataAnalysis.cpp
)

if(LLVM_ENABLE_PLUGINS AND (WIN32 OR CYGWIN))
    target_link_libraries(data-analysis ${cmake_2_8_12_PRIVATE}
        clangAST
        clangBasic
        clangFrontend
        LLVMSupport
    )
endif()
