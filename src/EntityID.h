#pragma once
#include <cstdint>

using ModelID = uint32_t;
using LightID = uint32_t;
using EntityID = uint32_t;
inline EntityID next_entity_id(){
    static EntityID current = 1;
    return current++;
}


