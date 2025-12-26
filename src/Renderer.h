
#include "Model.h"
#include "Light.h"
#include "Shader.h"
#include "GPULight.h"
#include "GPUMesh.h"
#include <memory>
#include "EntityID.h"
class Renderer {
public:
    Renderer();
    
    void render(const glm::mat4& view, const glm::mat4& projection,const std::vector<std::unique_ptr<Light>>& lights,const std::vector<std::unique_ptr<Models::Model>>& models);
    
    //methods to allocate things on the gpu
    void upload_model(Models::Model* model);
    void upload_light(Light* light);
    //could add a function to check if what i will render is all allocated

    void initialise_shaders();
    void init(int screen_width,int screen_height);

private:
    std::vector<std::shared_ptr<Shader>> shaders;
    std::unordered_map<EntityID,std::unique_ptr<GPULight>> allocated_lights;
    std::unordered_map<EntityID,std::unique_ptr<GPUMesh>> allocated_models;
    std::shared_ptr<Shader> get_shader_by_name(const std::string& shader_name);
    void draw(Models::Model* model,const glm::mat4& view, const glm::mat4& projection);
    void draw_model_depth(Models::Model* model,std::shared_ptr<Shader> shader);
    void draw_light(Light* light,int index,std::shared_ptr<Shader> shader);
    void draw_light_depth(Light* light,const std::vector<std::unique_ptr<Models::Model>>& models);
    void draw_lights(const std::vector<std::unique_ptr<Light>>& lights);
    void draw_lights_depth(const std::vector<std::unique_ptr<Light>>& lights,const std::vector<std::unique_ptr<Models::Model>>& models);
};
