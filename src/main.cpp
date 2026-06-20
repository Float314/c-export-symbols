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
#include <matjson.hpp>
 
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cctype>

struct config_json {
    std::vector<std::string> files;
    std::string export_dir;
};

static bool loadConfig(const std::string& config_path, config_json& cfg) {
    std::ifstream in(config_path);
    if (!in) {
        std::cerr << "error: could not open config file '" << config_path << "'\n";
        return false;
    }
 
    auto result = matjson::parse(in); // matjson::parse(std::istream&) -> Result<Value, ParseError>
    if (!result) {
        std::cerr << "error: invalid JSON in '" << config_path << "': "
                   << result.unwrapErr().message << "\n";
        return false;
    }
    matjson::Value json = result.unwrap();
 
    if (json.contains("files") && json["files"].isArray()) {
        for (auto& entry : json["files"]) {
            auto str_result = entry.asString();
            if (str_result) {
                cfg.files.push_back(str_result.unwrap());
            }
        }
    }
 
    if (json.contains("exports-symbol-directory")) {
        auto dir_result = json["exports-symbol-directory"].asString();
        cfg.export_dir = dir_result ? dir_result.unwrap() : ".";
    } else {
        cfg.export_dir = ".";
    }
 
    return true;
}

static std::string makeHeaderGuard(const std::filesystem::path& header_path) {
    std::string guard = header_path.filename().string();
    for (char& c : guard) {
        c = std::isalnum(static_cast<unsigned char>(c)) ? static_cast<char>(std::toupper(c)) : '_';
    }
    return guard;
}

static bool writeHeader(const std::filesystem::path& header_path, const std::vector<function_unit>& functions) {
    std::ofstream out(header_path);
    if (!out) {
        std::cerr << "error: could not write '" << header_path.string() << "'\n";
        return false;
    }
 
    std::string guard = makeHeaderGuard(header_path);
    out << "#ifndef " << guard << "\n#define " << guard << "\n\n";
 
    for (const auto& fn : functions) {
        out << fn.declaration << ";\n";
    }
 
    out << "\n#endif // " << guard << "\n";
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 3 || std::string(argv[1]) != "export") {
        std::cerr << "Usage: " << argv[0] << " export <config.json>\n";
        return 1;
    }
 
    config_json cfg;
    if (!loadConfig(argv[2], cfg)) {
        return 1;
    }
 
    if (cfg.files.empty()) {
        std::cerr << "warning: no files listed in config.\n";
        return 0;
    }
 
    std::filesystem::create_directories(cfg.export_dir);
 
    int failures = 0;
    for (const auto& src : cfg.files) {
        std::vector<function_unit> functions;
        CXTranslationUnit unit; // filled in by parse_functions on success
 
        if (!parse_functions(src.c_str(), functions, unit)) {
            ++failures;
            continue; // parse_functions already printed the error
        }
 
        std::filesystem::path src_path(src);
        std::filesystem::path header_path = std::filesystem::path(cfg.export_dir) / (src_path.stem().string() + ".h");
 
        if (writeHeader(header_path, functions)) {
            std::cout << "generated: " << header_path.string()
                      << " (" << functions.size() << " function"
                      << (functions.size() == 1 ? "" : "s") << ")\n";
        } else {
            ++failures;
        }
 
        // We own `unit` now -- parse_functions only disposed its CXIndex, not this.
        clang_disposeTranslationUnit(unit);
    }
 
    return failures > 0 ? 1 : 0;
 
    return 0;
}