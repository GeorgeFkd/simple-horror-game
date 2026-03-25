#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

using namespace clang::ast_matchers;
using namespace clang::tooling;

struct CliInput {
    char* sourceCodeDir;
    char* assetsDir;
    char* compileCommandsDir;
};

class StringLiteralCollector : public MatchFinder::MatchCallback {
  public:
    void run(const MatchFinder::MatchResult& result) override {
        if (const auto* lit = result.Nodes.getNodeAs<clang::StringLiteral>("str")) {
            strings.push_back(lit->getString().str());
        }
    }
    std::vector<std::string> strings;
};

std::vector<std::string> generate_asset_file_list(const CliInput& input) {
    std::vector<std::string> result      = {};
    std::vector<std::string> sourceFiles = {};
    for (const auto& entry : std::filesystem::recursive_directory_iterator(input.sourceCodeDir)) {
        auto ext = entry.path().extension();
        if (ext == ".cpp" || ext == ".hpp" || ext == ".h") {
            sourceFiles.push_back(entry.path().string());
        }
    }

    StringLiteralCollector collector;
    MatchFinder            finder;
    finder.addMatcher(stringLiteral().bind("str"), &collector);

    std::string errorFromCompileCommandsDetection;
    auto        db = CompilationDatabase::autoDetectFromDirectory(input.compileCommandsDir,
                                                                  errorFromCompileCommandsDetection);
    if (!errorFromCompileCommandsDetection.empty()) {
        std::cerr << "Something happened when trying to autodetect compilation database from dir: "
                  << input.sourceCodeDir << "\n";
        std::cerr << "ERROR: " << errorFromCompileCommandsDetection << "\n";
        return {};
    }
    ClangTool tool(*db, sourceFiles);
    tool.run(newFrontendActionFactory(&finder).get());
    for (const auto& entry : std::filesystem::recursive_directory_iterator(input.assetsDir)) {
        if (!entry.is_regular_file())
            continue;
        auto filename = entry.path().filename().string();
        for (const auto& s : collector.strings) {
            if (s.find(filename) != std::string::npos) {
                result.push_back(entry.path().string());
            }
        }
    }

    return result;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0]
                  << " [source code directory] [assets directory] [output folder for assets]";
        return EXIT_FAILURE;
    }
    auto sourceCodeDir      = argv[1];
    auto assetsDir          = argv[2];
    auto outputFile         = argv[3];
    auto compileCommandsDir = argv[4];

    auto input  = CliInput{.sourceCodeDir      = sourceCodeDir,
                           .assetsDir          = assetsDir,
                           .compileCommandsDir = compileCommandsDir};
    auto result = generate_asset_file_list(input);
    if (result.empty()) {
        return EXIT_FAILURE;
    }
    std::cout << "Generating " << outputFile << " file from" << assetsDir
              << "assets folder and source code in " << sourceCodeDir << "\n";

    std::ofstream output;
    output.open(outputFile);
    for (auto file : result) {
        output << file << "\n";
    }
    return EXIT_SUCCESS;
}
