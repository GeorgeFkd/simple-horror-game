#include "Renderer.h"
#include "GPULight.h"
#include "GPUMesh.h"
#include "Shader.h"
void Renderer::draw_light(Light* light, int index, std::shared_ptr<Shader> shader) {

    auto gpulight = allocated_lights.find(light->id);
    GLuint depth_map = 0;
    if (gpulight != allocated_lights.end()) {
        std::cout << "rendering light: " << light->id << "\n";
        depth_map = gpulight->second->depth_map;
    } else {
        std::cout << "Did not find light with id: " << light->id;
        assert(false);
        // std::cout << "Did not find it from cache, reading file normally\n";
    }

    std::string base = "lights[" + std::to_string(index) + "].";
    assert(depth_map != 0);
    if (light->type == LightType::POINT) {
        std::string base = "shadowMapCube" + std::to_string(index);
        // shader->set_int(base + "shadowMapCube", index);
        shader->set_texture(base, depth_map, GL_TEXTURE5 + index,
                            GL_TEXTURE_CUBE_MAP);
    } else {
        // spot or directional use a 2D depth map
        // shader->set_int(base + "shadowMap2D", index);
        std::string base = "shadowMap" + std::to_string(index);
        shader->set_texture(base, depth_map, GL_TEXTURE0 + index, GL_TEXTURE_2D);
    }
    // light->bind_shadow_map(shader, base, index);

    shader->set_vec3(base + "position", light->position);
    shader->set_float(base + "power", light->light_power);
    shader->set_vec3(base + "color", light->color);
    // only send direction for non-point lights
    if (light->type != LightType::POINT) {
        shader->set_vec3(base + "direction", light->direction);
    }
    shader->set_vec3(base + "ambient", light->ambient);
    shader->set_vec3(base + "diffuse", light->diffuse);
    shader->set_vec3(base + "specular", light->specular);
    if (light->type == LightType::SPOT) {
        shader->set_float(base + "cutoff", light->cutoff);
        shader->set_float(base + "outerCutoff", light->outer_cutoff);
    }
    shader->set_int(base + "type", int(light->type));
    shader->set_float(base + "attenuation_constant", light->attenuation_constant);
    shader->set_float(base + "attenuation_linear", light->attenuation_linear);
    shader->set_float(base + "attenuation_quadratic", light->attenuation_quadratic);
    shader->set_float(base + "attenuation_power", light->attenuation_power);
    shader->set_float(base + "nearPlane", light->get_near_plane());
    shader->set_float(base + "farPlane", light->get_far_plane());
    shader->set_mat4(base + "view", light->get_light_view());
    shader->set_mat4(base + "proj", light->get_light_projection());
}

void Renderer::init(int screen_width, int screen_height) {
    initialize_glew();
    enable_gl_features({GL_DEPTH_TEST});
    set_viewport(0, 0, screen_width, screen_height);
    set_clear_color(0.0f, 0.0f, 0.0f, 1.0f);
    clear_buffers(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::upload_model(Models::Model* m) {
    auto model_data_entry = allocated_models.find(m->model_data->id);
    if (model_data_entry == allocated_models.end()) {
        std::cout << "Uploading model with id: " << m->model_data->id << "\n";
        auto mesh = std::make_unique<GPUMesh>();
        mesh->reserve_opengl_memory(m->model_data.get());
        allocated_models.insert({m->model_data->id, std::move(mesh)});
    } else {
        std::cout << "Model with id: " << m->model_data->id << " is already uploaded. \n";
    }
}

void Renderer::upload_light(Light* light) {
    auto light_entry = allocated_lights.find(light->id);
    if (light_entry == allocated_lights.end()) {
        std::cout << "Uploading light with id: " << light->id << "\n";
        auto gpulight = std::make_unique<GPULight>();
        gpulight->reserve_opengl_memory(light);
        allocated_lights.insert({light->id, std::move(gpulight)});
    } else {
        std::cout << "Light with id: " << light->id << "is already uploaded\n";
    }
}

void Renderer::draw_light_depth(Light*                                             light,
                                const std::vector<std::unique_ptr<Models::Model>>& models) {
    std::shared_ptr<Shader> shader;
    if (light->get_type() == LightType::POINT) {
        auto depthCube = get_shader_by_name("depth_cube");
        shader         = depthCube;
    } else {
        auto depth2D = get_shader_by_name("depth_2d");
        shader       = depth2D;
    }
    set_viewport(0, 0, light->shadow_width, light->shadow_height);
    auto gpulight = allocated_lights.find(light->id);
    if (gpulight != allocated_lights.end()) {
        bind_framebuffer(GL_FRAMEBUFFER, gpulight->second->depth_map_fbo);
    } else {
        std::cout << "Did not find light with id: " << light->id;
        assert(false);
    }

    gl_clear();

    enable_gl_features({GL_DEPTH_TEST, GL_CULL_FACE});

    set_cull_face(GL_FRONT);

    shader->use();
    if (light->type == LightType::POINT) {
        glm::mat4 proj  = light->get_light_projection();
        auto      views = light->get_point_light_views();
        // Six passes, one per cube face
        for (int face = 0; face < 6; ++face) {
            // update this face's matrix
            shader->set_vec3("lightPos", light->position);
            shader->set_float("farPlane", light->far_plane);
            shader->set_mat4("shadowMatrices[" + std::to_string(face) + "]", proj * views[face]);
            glm::mat4 VP     = proj * views[face];
            auto      planes = light->extract_frustum_planes(VP);

            // draw all models into this face
            for (auto& m : models) {
                m->in_frustum(planes);

                if (!m->is_active())
                    continue;
                if (!m->is_in_frustum())
                    continue;
                draw_model_depth(m.get(), shader);
            }
        }
    } else {
        shader->set_mat4("uView", light->get_light_view());
        shader->set_mat4("uProj", light->get_light_projection());
        glm::mat4 VP     = light->get_light_projection() * light->get_light_view();
        auto      planes = light->extract_frustum_planes(VP);

        for (auto& m : models) {
            m->in_frustum(planes);
            if (!m->is_active())
                continue;
            if (!m->is_in_frustum())
                continue;
            draw_model_depth(m.get(), shader);
        }
    }

    set_cull_face(GL_BACK);
    set_color_mask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    bind_framebuffer(GL_FRAMEBUFFER, 0);
    unbind_shader();
}

void Renderer::draw_model_depth(Models::Model* m, std::shared_ptr<Shader> shader) {
    shader->set_mat4("uModel", m->model_instance.world_transform);
    auto model_entry =allocated_models.find(m->model_data->id);
    if(model_entry != allocated_models.end()){
        shader->bindVAO(model_entry->second->vao);
    }else{
        std::cout << "No VAO found for: " << m->model_instance.label << "\n";
        assert(false);
    }
    for (auto const& sm : m->model_data->submeshes) {
        void* offset_ptr = (void*)(sm.index_offset * sizeof(GLuint));
        shader->drawElemTriangles(sm.index_count, offset_ptr);
    }
    shader->unbindVAO();


}

void Renderer::draw_lights_depth(const std::vector<std::unique_ptr<Light>>&         lights,
                                 const std::vector<std::unique_ptr<Models::Model>>& models) {
    for (auto& light : lights) {
        if (!light->is_turned_on()) {
            continue;
        }
        draw_light_depth(light.get(), models);
    }
}

void Renderer::draw_lights(const std::vector<std::unique_ptr<Light>>& lights) {
    auto shader = get_shader_by_name("blinn-phong");

    shader->use();
    shader->set_int("numLights", (GLint)lights.size());

    for (size_t i = 0; i < lights.size(); ++i) {
        auto light = lights[i].get();
        draw_light(light, i, shader);
    }
}

std::shared_ptr<Shader> Renderer::get_shader_by_name(const std::string& shader_name) {

    auto shaderPos =
        std::find_if(shaders.begin(), shaders.end(), [shader_name](std::shared_ptr<Shader> s) {
            return s->get_shader_name() == shader_name;
        });

    assert(shaderPos != shaders.end());
    return *shaderPos;
}

Renderer::Renderer() {}

void Renderer::draw(Models::Model* m, const glm::mat4& view, const glm::mat4& projection) {
    auto shader = get_shader_by_name("blinn-phong");
    shader->set_mat4("uView", view);
    shader->set_mat4("uProj", projection);
    shader->set_mat4("uModel", m->model_instance.world_transform);

    auto model_entry =allocated_models.find(m->model_data->id);
    if(model_entry != allocated_models.end()){
        shader->bindVAO(model_entry->second->vao);
    }else {
        std::cout << "No VAO found to draw model: " << m->model_instance.label << "\n";
        assert(false);
    }
    for (auto const& sm : m->model_data->submeshes) {
        shader->set_vec3("material.ambient", sm.mat.Ka);
        shader->set_vec3("material.diffuse", sm.mat.Kd);
        shader->set_vec3("material.specular", sm.mat.Ks);
        shader->set_vec3("material.emissive", sm.mat.Ke);
        shader->set_float("material.shininess", sm.mat.Ns);
        shader->set_float("material.opacity", sm.mat.d);
        shader->set_int("material.illumModel", sm.mat.illum);
        shader->set_float("material.ior", sm.mat.Ni);
        shader->set_bool("material.useBumpMap", sm.mat.use_bump_map);

        if (sm.mat.tex_Ka) {
            shader->set_texture("ambientMap", sm.mat.tex_Ka, GL_TEXTURE1);
        }
        shader->set_bool("useAmbientMap", sm.mat.tex_Ka != 0 ? true : false);

        if (sm.mat.tex_Kd) {
            shader->set_texture("diffuseMap", sm.mat.tex_Kd, GL_TEXTURE2);
        }
        shader->set_bool("useDiffuseMap", sm.mat.tex_Kd != 0 ? true : false);

        if (sm.mat.tex_Ks) {
            shader->set_texture("specularMap", sm.mat.tex_Ks, GL_TEXTURE3);
        }
        shader->set_bool("useSpecularMap", sm.mat.tex_Ks != 0 ? true : false);

        if (sm.mat.tex_Bump) {
            shader->set_texture("bumpMap", sm.mat.tex_Bump, GL_TEXTURE4);
            shader->set_float("bumpScale", 4.0f);
        }
        void* offsetPtr = (void*)(sm.index_offset * sizeof(GLuint));
        shader->drawElemTriangles(sm.index_count, offsetPtr);
    }
    shader->unbindVAO();
}

void Renderer::initialise_shaders() {
    std::vector<std::string> shader_paths = {"assets/shaders/blinnphong.vert",
                                             "assets/shaders/blinnphong.frag"};
    std::vector<GLenum>      shader_types = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
    // creating a Shader segfaults
    auto blinnphong = std::make_shared<Shader>(shader_paths, shader_types, "blinn-phong");

    shader_paths  = {"assets/shaders/depth_2d.vert", "assets/shaders/depth_2d.frag"};
    shader_types  = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
    auto depth_2d = std::make_shared<Shader>(shader_paths, shader_types, "depth_2d");

    shader_paths    = {"assets/shaders/depth_cube.vert", "assets/shaders/depth_cube.geom",
                       "assets/shaders/depth_cube.frag"};
    shader_types    = {GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER};
    auto depth_cube = std::make_shared<Shader>(shader_paths, shader_types, "depth_cube");

    shader_paths    = {"assets/shaders/text.vert", "assets/shaders/text.frag"};
    shader_types    = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
    auto textshader = std::make_shared<Shader>(shader_paths, shader_types, "text");

    shaders.push_back(blinnphong);
    shaders.push_back(depth_2d);
    shaders.push_back(depth_cube);
    shaders.push_back(textshader);
}

void Renderer::render(const glm::mat4& view, const glm::mat4& projection,
                      const std::vector<std::unique_ptr<Light>>&         lights,
                      const std::vector<std::unique_ptr<Models::Model>>& models) {
    draw_lights_depth(lights, models);

    float screen_width  = 1280.0f;
    float screen_height = 720.0f;
    set_viewport(0, 0, screen_width, screen_height);
    enable_gl_features({GL_MULTISAMPLE});
    // removing this doesnt change anything for some reason;
    clear_buffers(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    draw_lights(lights);

    for (auto const& model : models) {
        if (!model->is_active()) {
            continue;
        }

        // TODO this has a problem, in the beginning not all things are rendered properly
        //  if (!model->is_in_frustum()) {
        //      // std::cout << model->name() << std::endl;
        //      continue;
        //  }

        model->update_world_transform(glm::mat4(1.0f));
        draw(model.get(), view, projection);
    }

    disable_gl_capability(GL_MULTISAMPLE);
}
