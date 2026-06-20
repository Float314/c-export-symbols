## Choco installation of libclang

function(include_package)
    if(WIN32)
        find_package(Clang CONFIG)

        if(NOT CLANG_FOUND)
            message( "Clang not found, using manual way")
            find_library(CLANG_LIB clang PATHS "C:/Program Files/LLVM/lib" NO_DEFAULT_PATH)
            if(NOT CLANG_LIB)
                message(FATAL_ERROR "Clang library not found. Using manual Way #2.")

                target_include_directories(${PROJECT_NAME} PRIVATE "C:/Program Files/LLVM/include")
                target_link_libraries(${PROJECT_NAME} PRIVATE "C:/Program Files/LLVM/lib/libclang.lib")
            endif()
        endif()
    endif()
endfunction()