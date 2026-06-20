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
    return guard + "_H";
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