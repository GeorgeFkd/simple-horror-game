
#include "Model.h"
#include "Light.h"
#include "Shader.h"
#include <memory>
class Renderer {
public:
    Renderer();
    void draw(Models::Model* model,const glm::mat4& view, const glm::mat4& projection);
    void draw_light(Light* light,int index,std::shared_ptr<Shader> shader);
    void draw_light_depth(Light* light,const std::vector<std::unique_ptr<Models::Model>>& models);
    void draw_lights(const std::vector<std::unique_ptr<Light>>& lights);
    void draw_lights_depth(const std::vector<std::unique_ptr<Light>>& lights,const std::vector<std::unique_ptr<Models::Model>>& models);
    void render(const glm::mat4& view, const glm::mat4& projection,const std::vector<std::unique_ptr<Light>>& lights,const std::vector<std::unique_ptr<Models::Model>>& models);
    
    void initialise_shaders();

private:
    std::vector<std::shared_ptr<Shader>> shaders;
    std::shared_ptr<Shader> get_shader_by_name(const std::string& shader_name);
};
