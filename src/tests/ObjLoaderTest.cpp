#include "loaders/OBJLoader.h"
#include <gtest/gtest.h>


TEST(ObjLoader, BasicAssertions) {

    ObjectLoader::OBJLoader loader;

    const char* filename = "./assets/test.obj";
    //should probably write a method that reads it from a string
    auto model = loader.read_from_file(filename);

    ASSERT_NE(model, nullptr);

    // ---- geometry counts ----
    EXPECT_EQ(model->m_vertices.size(), 8);
    EXPECT_EQ(model->m_texture_coords.size(), 4);
    EXPECT_EQ(model->m_vertex_normals.size(), 6);
    EXPECT_EQ(model->m_faces.size(), 6);

    // ---- materials ----
    EXPECT_EQ(model->m_materials.size(), 6);
    EXPECT_EQ(model->m_mat_name_to_id.size(), 6);

    EXPECT_TRUE(model->m_mat_name_to_id.find("mat_red")     != model->m_mat_name_to_id.end());
    EXPECT_TRUE(model->m_mat_name_to_id.find("mat_green")   != model->m_mat_name_to_id.end());
    EXPECT_TRUE(model->m_mat_name_to_id.find("mat_blue")    != model->m_mat_name_to_id.end());
    EXPECT_TRUE(model->m_mat_name_to_id.find("mat_yellow")  != model->m_mat_name_to_id.end());
    EXPECT_TRUE(model->m_mat_name_to_id.find("mat_magenta") != model->m_mat_name_to_id.end());
    EXPECT_TRUE(model->m_mat_name_to_id.find("mat_cyan")    != model->m_mat_name_to_id.end());

    // ---- groups ----
    EXPECT_EQ(model->m_groups.size(), 6);
    EXPECT_EQ(model->m_group_name_to_id.size(), 6);

    EXPECT_TRUE(model->m_group_name_to_id.find("front")  != model->m_group_name_to_id.end());
    EXPECT_TRUE(model->m_group_name_to_id.find("back")   != model->m_group_name_to_id.end());
    EXPECT_TRUE(model->m_group_name_to_id.find("top")    != model->m_group_name_to_id.end());
    EXPECT_TRUE(model->m_group_name_to_id.find("bottom") != model->m_group_name_to_id.end());
    EXPECT_TRUE(model->m_group_name_to_id.find("right")  != model->m_group_name_to_id.end());
    EXPECT_TRUE(model->m_group_name_to_id.find("left")   != model->m_group_name_to_id.end());

    // ---- face metadata sanity ----
    for (const auto& face : model->m_faces) {
        EXPECT_GE(face.material_id, 0);
        EXPECT_GE(face.group_id, 0);
        EXPECT_LT(face.material_id, static_cast<int>(model->m_materials.size()));
        EXPECT_LT(face.group_id, static_cast<int>(model->m_groups.size()));
    }
}




