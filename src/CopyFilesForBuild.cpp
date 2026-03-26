#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <assets_dir> <build_assets_dir> <required_assets.txt>\n";
        return EXIT_FAILURE;
    }

    std::filesystem::path assetsDir(argv[1]);
    std::filesystem::path buildAssetsDir(argv[2]);
    std::ifstream file(argv[3]);

    if (!file) {
        std::cerr << "Failed to open " << argv[3] << "\n";
        return EXIT_FAILURE;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::filesystem::path src(line);
        auto relative = std::filesystem::relative(src, assetsDir);
        //DOES NOT SEEM TO COPY THE FONTS
        auto dst = buildAssetsDir / relative;
        std::filesystem::create_directories(dst.parent_path());
        std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing);
    }

    return EXIT_SUCCESS;
}
