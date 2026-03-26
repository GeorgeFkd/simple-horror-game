#include "loaders/ModelLoader.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
struct CliInput {
    char* mentionedFilesInCodeFilename;
    char* outputFilename;
};

void printExecInfo(CliInput* input) {
    std::cout << "Input for dependent files finder is: " << input->mentionedFilesInCodeFilename
              << "\n";
}

std::vector<std::string> extractDependentsFrom(const std::string& filename) {
    //PROBLEM: I CANT GET FONTS FOLDER
    auto ext   = std::filesystem::path(filename).extension();
    if (ext != ".obj") {
        return {filename};
    }
    auto mdata = ModelLoader::load_or_get_cached(filename);
    

    std::vector<std::string> dependents;
    for (const auto& submesh : mdata->submeshes) {
        std::cout << "Material filename: " << submesh.mat.filename << "\n";
        dependents.push_back(submesh.mat.filename);
        for (const auto& map :
             {submesh.mat.map_Ka, submesh.mat.map_Kd, submesh.mat.map_Ks, submesh.mat.map_Bump}) {
            if (!map.empty())
                dependents.push_back(map);
        }
    }
    return dependents;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <mentionedFiles.txt> <output.txt>\n";
        return EXIT_FAILURE;
    }
    CliInput input = CliInput{.mentionedFilesInCodeFilename = argv[1], .outputFilename = argv[2]};
    printExecInfo(&input);

    std::ifstream file(input.mentionedFilesInCodeFilename);
    if (!file) {
        std::cerr << "Failed to open " << input.mentionedFilesInCodeFilename << "\n";
        return EXIT_FAILURE;
    }

    std::vector<std::string> allFiles;
    std::string              line;
    while (std::getline(file, line)) {
        if (line.empty())
            continue;
        allFiles.push_back(line);
        std::cout << "Reading file " << line << " to extract dependencies.\n";

        auto dependents = extractDependentsFrom(line);
        allFiles.insert(allFiles.end(), dependents.begin(), dependents.end());
    }

    std::ofstream output(input.outputFilename);
    if (!output) {
        std::cerr << "Failed to open output file\n";
        return EXIT_FAILURE;
    }
    for (const auto& f : allFiles) {
        output << f << "\n";
    }

    return EXIT_SUCCESS;
}
