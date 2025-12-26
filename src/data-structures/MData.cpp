#include "EntityID.h"
#include "MData.h"
MData::~MData() {}


MData::MData() {
    id = next_entity_id();
}


