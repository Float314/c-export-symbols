/* 
* - BSD 4-Clause "Original" or "Old" License
* - 
* - Copyright (c) 2026, Float314
* - All rights reserved.
* - 
* - Redistribution and use in source and binary forms, with or without
* - modification, are permitted provided that the following conditions are met:
* - 
* - 1. Redistributions of source code must retain the above copyright notice, this
* -    list of conditions and the following disclaimer.
* - 
* - 2. Redistributions in binary form must reproduce the above copyright notice,
* -    this list of conditions and the following disclaimer in the documentation
* -    and/or other materials provided with the distribution.
* - 
* - 3. All advertising materials mentioning features or use of this software must
* -    display the following acknowledgement:
* -      This product includes software developed by Float314 (exports-symbols).
* - 
* - 4. Neither the name of the copyright holder nor the names of its
* -    contributors may be used to endorse or promote products derived from
* -    this software without specific prior written permission.
* - 
* - THIS SOFTWARE IS PROVIDED BY COPYRIGHT HOLDER "AS IS" AND ANY EXPRESS OR
* - IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* - MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
* - EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* - SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* - PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* - OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* - WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* - OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* - ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "c-parser.hpp"
#include <iostream>
 
static std::string toString(CXString cx_str) {
    std::string result = clang_getCString(cx_str) ? clang_getCString(cx_str) : "";
    clang_disposeString(cx_str);
    return result;
}
 
struct VisitorContext {
    std::vector<function_unit>* functions;
    CXTranslationUnit unit;
};
 
static CXChildVisitResult visitor(CXCursor cursor, CXCursor /*parent*/, CXClientData client_data) {
    auto* ctx = static_cast<VisitorContext*>(client_data);
 
    
    if (clang_getCursorKind(cursor) == CXCursor_FunctionDecl &&
        clang_isCursorDefinition(cursor) &&
        clang_Location_isFromMainFile(clang_getCursorLocation(cursor))) {
 
        CXSourceRange extent = clang_getCursorExtent(cursor);
        CXSourceLocation start_loc = clang_getRangeStart(extent);
 
        CXCursor body = clang_getNullCursor();
        clang_visitChildren(cursor, [](CXCursor c, CXCursor, CXClientData data) {
            if (clang_getCursorKind(c) == CXCursor_CompoundStmt) {
                *static_cast<CXCursor*>(data) = c;
                return CXChildVisit_Break;
            }
            return CXChildVisit_Continue;
        }, &body);
 
        CXSourceLocation end_loc;
        if (!clang_Cursor_isNull(body)) {
            end_loc = clang_getRangeStart(clang_getCursorExtent(body));
        } else {
            end_loc = clang_getRangeEnd(extent); // fallback
        }
 
        CXFile file; unsigned start_off, end_off, line, col;
        clang_getSpellingLocation(start_loc, &file, &line, &col, &start_off);
        clang_getSpellingLocation(end_loc, nullptr, nullptr, nullptr, &end_off);
 
        // Pull raw text from the translation unit's buffer.
        CXToken* tokens = nullptr;
        unsigned num_tokens = 0;
        clang_tokenize(ctx->unit, extent, &tokens, &num_tokens);
 
        std::string sig;
        for (unsigned i = 0; i < num_tokens; ++i) {
            CXSourceLocation tok_loc = clang_getTokenLocation(ctx->unit, tokens[i]);
            unsigned tok_off;
            clang_getSpellingLocation(tok_loc, nullptr, nullptr, nullptr, &tok_off);
            if (tok_off >= end_off) break; // stop once we hit the body
 
            std::string tok_str = toString(clang_getTokenSpelling(ctx->unit, tokens[i]));
            if (!sig.empty()) sig += ' ';
            sig += tok_str;
        }
        clang_disposeTokens(ctx->unit, tokens, num_tokens);
 
        if (!sig.empty()) {
            ctx->functions->push_back({ sig });
        }
 
        return CXChildVisit_Continue; 
    }
 
    return CXChildVisit_Recurse;
}
 
bool parse_functions(const char* source_filename, std::vector<function_unit>& out_functions, CXTranslationUnit& unit_out) {
    CXIndex index = clang_createIndex(0, 0);
 
    CXTranslationUnit unit = clang_parseTranslationUnit(
        index, source_filename, nullptr, 0, nullptr, 0, CXTranslationUnit_None
    );
 
    if (unit == nullptr) {
        std::cerr << "Failed to parse: " << source_filename << "\n";
        clang_disposeIndex(index);
        return false;
    }
 
    CXCursor cursor = clang_getTranslationUnitCursor(unit);
    VisitorContext ctx{ &out_functions, unit };
    clang_visitChildren(cursor, visitor, &ctx);
 
    unit_out = unit;
    clang_disposeIndex(index); // safe: index only needed during parse, not after
    return true;
}