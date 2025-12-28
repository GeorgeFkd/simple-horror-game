#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_set>
#include "MData.h"
#include "loaders/ModelLoader.h"

std::string serialize_vertex(const Vertex& v) {
    std::ostringstream oss;
    oss << "Vertex({"
        << v.position.x << "f," << v.position.y << "f," << v.position.z << "f}, {"
        << v.normal.x << "f," << v.normal.y << "f," << v.normal.z << "f}, {"
        << v.texcoord.x << "f," << v.texcoord.y << "f})";
    return oss.str();
}

std::string serialize_material(const Material& m) {
    std::ostringstream oss;
    oss << "Material({"
        << m.Ka.x << "f," << m.Ka.y << "f," << m.Ka.z << "f}, {"
        << m.Kd.x << "f," << m.Kd.y << "f," << m.Kd.z << "f}, {"
        << m.Ks.x << "f," << m.Ks.y << "f," << m.Ks.z << "f}, {"
        << m.Ke.x << "f," << m.Ke.y << "f," << m.Ke.z << "f}, "
        << m.Ns << "f, " << m.d << "f, " << m.Ni << "f, "
        << (m.use_bump_map ? "true" : "false") << ", "
        << m.tex_Ka << ", " << m.tex_Kd << ", " << m.tex_Ks << ", " << m.tex_Bump
        << ")";
    return oss.str();
}

std::string serialize_submesh(const SubMesh& sm) {
    std::ostringstream oss;
    oss << "SubMesh(" << sm.index_offset << ", " << sm.index_count << ", "
        << serialize_material(sm.mat) << ")";
    return oss.str();
}

std::string serialize_mdata(const MData* data) {
    std::ostringstream oss;

    // Serialize indices
    oss << "std::vector<GLuint>{";
    for (size_t i = 0; i < data->indices.size(); ++i) {
        oss << data->indices[i];
        if (i + 1 < data->indices.size()) oss << ",";
    }
    oss << "}, ";

    // Serialize vertices
    oss << "std::vector<Vertex>{";
    for (size_t i = 0; i < data->unique_vertices.size(); ++i) {
        oss << serialize_vertex(data->unique_vertices[i]);
        if (i + 1 < data->unique_vertices.size()) oss << ",";
    }
    oss << "}, ";

    // Serialize submeshes
    oss << "std::vector<SubMesh>{";
    for (size_t i = 0; i < data->submeshes.size(); ++i) {
        oss << serialize_submesh(data->submeshes[i]);
        if (i + 1 < data->submeshes.size()) oss << ",";
    }
    oss << "}";

    return oss.str();
}



// std::string serialize_mdata(const MData* data) {
//     // TODO: implement the serialization of indices, vertices, submeshes, etc.
//     return "{}"; // placeholder
// }

std::vector<std::string> extract_quoted_strings(const std::string& source) {
    std::vector<std::string> result;

    size_t i = 0;
    while (i < source.size()) {
        if (source[i] == '"') {
            size_t start = i + 1;
            size_t end   = start;

            while (end < source.size()) {
                if (source[end] == '"' && source[end - 1] != '\\') {
                    break;
                }
                ++end;
            }

            if (end < source.size()) {
                result.push_back(source.substr(start, end - start));
                i = end + 1;
            } else {
                break;
            }
        } else {
            ++i;
        }
    }

    return result;
}

std::vector<std::string> parse_files_to_mdata_strings(const std::vector<std::string>& filepaths) {
    std::vector<std::string> result;
    for (const auto& path : filepaths) {
        // Parse the OBJ file into an MData object
        auto model_data = ModelLoader::load_or_get_cached(path); // Returns MData
        // Convert the MData object to a string representation
        result.push_back(serialize_mdata(model_data.get()));
    }
    return result;
}



std::string generate_output(const std::vector<std::string>& preloaded_models) {
    // First, parse the OBJ files into MData string representations
    std::vector<std::string> mdata_strings = parse_files_to_mdata_strings(preloaded_models);

    std::string output;

    // Header includes
    output += "#include \"ModelLoader.h\"\n";
    output += "#include \"MData.h\"\n";
    output += "#include \"Model.h\"\n\n";

    // Begin initialize method
    output += "void ModelLoader::initialize() {\n";

    for (size_t i = 0; i < preloaded_models.size(); ++i) {
        const auto& path = preloaded_models[i];
        const auto& mdata_str = mdata_strings[i];

        // Replace invalid characters in variable name
        std::string obj_var_name = path;
        for (auto& c : obj_var_name) {
            if (!isalnum(c)) c = '_';
        }

        output += "    // Preloading: " + path + "\n";
        output += "    {\n";
        output += "        auto " + obj_var_name + " = std::make_shared<MData>(" + mdata_str + ");\n";
        output += "        model_registry[\"" + path + "\"] = " + obj_var_name + ";\n";
        output += "    }\n";
    }

    output += "}\n";

    return output;
}


int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: generator <input_file> <output_folder> <output_filename>\n";
        return 1;
    }

    const std::string input_file    = argv[1];
    const std::string output_folder = argv[2];
    const std::string output_file   = argv[3];

    // ---- read input file ----
    std::ifstream in(input_file, std::ios::in | std::ios::binary);
    if (!in) {
        std::cerr << "Failed to open input file\n";
        return 1;
    }

    std::string source(
        (std::istreambuf_iterator<char>(in)),
        std::istreambuf_iterator<char>()
    );

    std::vector<std::string> all_strings = extract_quoted_strings(source);

    const std::unordered_set<std::string> prefixes = {
        "assets/models",
    };

    std::vector<std::string> filtered;
    for (const auto& s : all_strings) {
        for (const auto& p : prefixes) {
            if (s.rfind(p, 0) == 0) { // starts with prefix
                filtered.push_back(s);
                break;
            }
        }
    }
    for(const auto& s: filtered) {
        std::cout << "Filtered string to process: " << s << "\n";
    }

    std::string output_code = generate_output(filtered);

    std::string full_path = output_folder + "/" + output_file;
    std::ofstream out(full_path, std::ios::out | std::ios::binary);
    if (!out) {
        std::cerr << "Failed to open output file\n";
        return 1;
    }

    out << output_code;
    std::cout << "Generated file at: " << full_path << " with size: " << output_code.size() << ".\n";
    out.close();

    return 0;
}
