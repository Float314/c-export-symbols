#pragma once
 
#include <clang-c/Index.h> // r we serious rn 
#include <string>
#include <vector>
 
struct function_unit {
    std::string declaration;
};
 
// Parses `source_filename` with libclang and fills `out_functions` with every
// top-level function *definition* found in the main file.
//
// On success, `unit_out` receives ownership of the live CXTranslationUnit --
// the caller is responsible for calling clang_disposeTranslationUnit(unit_out)
// once done with it. On failure, `unit_out` is left untouched.
bool parse_functions(const char* source_filename, std::vector<function_unit>& out_functions, CXTranslationUnit& unit_out);