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