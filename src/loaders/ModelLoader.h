#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include <numeric>
#include "scene_objects/MData.h"
class ModelLoader {
public:
    // Returns cached model if present, otherwise loads and caches it
    static std::shared_ptr<MData> load_or_get_cached(const std::string& objFile);

    static void initialize();

private:
    static std::shared_ptr<MData> parse_from_obj_file(const std::string& objFile);
    // Cache: obj file path -> parsed model data
    inline static std::unordered_map<std::string, std::shared_ptr<MData>> cache;
};
