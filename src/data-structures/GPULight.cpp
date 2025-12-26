#include "GPULight.h"
#include "Shader.h"

GPULight::GPULight() {

}

GPULight::~GPULight() {
    delete_framebuffer(&depth_map_fbo);
    delete_texture(&depth_map);
}

void GPULight::reserve_opengl_memory(Light* light) {
    generate_framebuffer(&depth_map_fbo);
    generate_texture(&depth_map);
    if (light->type == LightType::POINT) {
        bind_texture(GL_TEXTURE_CUBE_MAP, depth_map);
        for (unsigned i = 0; i < 6; ++i) {
            set_texture_image_2d(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
                                 light->shadow_width, light->shadow_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
                                 nullptr);
        }

        set_texture_parameters(GL_TEXTURE_CUBE_MAP, {{GL_TEXTURE_MIN_FILTER, GL_NEAREST},
                                                     {GL_TEXTURE_MAG_FILTER, GL_NEAREST},
                                                     {GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE},
                                                     {GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE},
                                                     {GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE}});
        bind_framebuffer(GL_FRAMEBUFFER, depth_map_fbo);
        attach_texture_to_framebuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_map, 0);
    } else {
        // -- spot or directional: 2D depth texture --
        bind_texture(GL_TEXTURE_2D, depth_map);
        set_texture_image_2d(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, light->shadow_width, light->shadow_height, 0,
                             GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        set_texture_parameters(GL_TEXTURE_2D, {{GL_TEXTURE_MIN_FILTER, GL_NEAREST},
                                               {GL_TEXTURE_MAG_FILTER, GL_NEAREST},
                                               {GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER},
                                               {GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER}});
        float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        set_texture_parameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        bind_framebuffer(GL_FRAMEBUFFER, depth_map_fbo);
        attach_texture2d_to_framebuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                                        depth_map, 0);
    }

    validate_framebuffer();
    disable_color_buffers();
    unbind_framebuffer();


    
}
