#include "GPUMesh.h"
#include "Shader.h"
#define STB_IMAGE_IMPLEMENTATION
#include  "stb_image.h"
#include "tiffio.h"
GPUMesh::~GPUMesh() {
    deleteBuffer(&ebo);
    deleteBuffer(&vbo);
    deleteVertexArray(&vao);
}

GPUMesh::GPUMesh() {

}

static GLuint reserve_opengl_texture(uint32_t width, uint32_t height, uint32_t border,
                                     uint32_t format, const void* data) {
    GLuint tex;
    generate_texture(&tex);           // glGenTextures
    bind_texture(GL_TEXTURE_2D, tex); // glBindTexture

    set_texture_image_2d(GL_TEXTURE_2D, // glTexImage2D
                         0, format, width, height, border, format, GL_UNSIGNED_BYTE, data);

    generate_mipmap(GL_TEXTURE_2D); // glGenerateMipmap

    set_texture_parameters(GL_TEXTURE_2D, {// glTexParameteri
                                           {GL_TEXTURE_WRAP_S, GL_REPEAT},
                                           {GL_TEXTURE_WRAP_T, GL_REPEAT},
                                           {GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR},
                                           {GL_TEXTURE_MAG_FILTER, GL_LINEAR}});

    unbind_texture(GL_TEXTURE_2D);

    return tex;
}

static GLuint load_texture_from_tiff(const std::string& filename) {
    TIFF* tif = TIFFOpen(filename.c_str(), "r");
    if (!tif) {
        std::cerr << "Failed to open TIFF: " << filename << "\n";

        return 1;
    }

    uint32_t width, height;
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);

    std::vector<uint32_t> raster(width * height);
    if (!TIFFReadRGBAImage(tif, width, height, raster.data(), 0)) {
        std::cerr << "Failed to read TIFF image data from: " << filename << "\n";
        TIFFClose(tif);
        return 2;
    }

    // TIFF is bottom-up; reverse vertically if needed
    std::vector<uint8_t> data(width * height * 4);
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            uint32_t pixel                = raster[(height - 1 - y) * width + x];
            data[4 * (y * width + x) + 0] = TIFFGetR(pixel);
            data[4 * (y * width + x) + 1] = TIFFGetG(pixel);
            data[4 * (y * width + x) + 2] = TIFFGetB(pixel);
            data[4 * (y * width + x) + 3] = TIFFGetA(pixel);
        }
    }

    TIFFClose(tif);

    return reserve_opengl_texture(width, height, 0, GL_RGBA, data.data());
}


static bool fileExtensionIs(const std::string& filename, const std::string& extension) {
    auto pos = filename.rfind(extension);

    if (pos != std::string::npos && (filename.substr(pos, filename.size()) == extension))
        return true;

    return false;
}
static GLuint load_texture_from_file(const std::string& filepath) {
    if (fileExtensionIs(filepath, ".tif")) {
        return load_texture_from_tiff(filepath);
    }
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);

    if (!data) {
        std::cerr << "Failed to load texture " << filepath << "\n";
        return 0;
    }

    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

    return reserve_opengl_texture(width, height, 0, format, data);
}


void GPUMesh::reserve_opengl_memory(MData* model_data) {
    makeVertexArray(&vao);
    makeBuffer(&vbo);
    makeBuffer(&ebo);

    bindVAO(vao);
    bindBuffer(vbo);
    bindBufferData(model_data->unique_vertices.size() * sizeof(Vertex), (void*) model_data->unique_vertices.data());
    bindElementBuffer(ebo);
    bindElementBufferData(model_data->indices.size() * sizeof(GLuint), (void*) model_data->indices.data());

    auto size = sizeof(Vertex);
    enableVAttribArray(0);
    bindVAttribPointer(0, 3, size, (void*)offsetof(Vertex, position));
    enableVAttribArray(1);
    bindVAttribPointer(1, 2, size, (void*)offsetof(Vertex, texcoord));
    enableVAttribArray(2);
    bindVAttribPointer(2, 3, size, (void*)offsetof(Vertex, normal));
    enableVAttribArray(3);
    bindVAttribPointer(3, 4, size, (void*)offsetof(Vertex, tangent));

    unbindVAO();

    
    for (auto& sm: model_data->submeshes) {
        if (!sm.mat.map_Kd.empty()) {
            GLuint id = load_texture_from_file(sm.mat.map_Kd);
#ifdef DEBUG_OBJLOADER
            std::cout << "Loaded map_Kd: " << material.map_Kd << " → ID " << id << std::endl;
#endif
            sm.mat.tex_Kd = id; // if you add it
        }

        if (!sm.mat.map_Ka.empty()) {
            GLuint id = load_texture_from_file(sm.mat.map_Ka);
#ifdef DEBUG_OBJLOADER
            std::cout << "Loaded map_Ka: " << material.map_Ka << " → ID " << id << std::endl;
#endif
            sm.mat.tex_Ka = id;
        }

        if (!sm.mat.map_Ks.empty()) {
            GLuint id = load_texture_from_file(sm.mat.map_Ks);
#ifdef DEBUG_OBJLOADER
            std::cout << "Loaded map_Ks: " << material.map_Ks << " → ID " << id << std::endl;
#endif
            sm.mat.tex_Ks = id;
        }

        if (!sm.mat.map_Bump.empty() && sm.mat.map_Bump != sm.mat.map_Kd) {
            // only load a bump map if it's a different file from the diffuse
            GLuint id             = load_texture_from_file(sm.mat.map_Bump);
            sm.mat.tex_Bump     = id;
            sm.mat.use_bump_map = true;
        } else {
            // either no bump entry, or they're re-using the diffuse as bump: disable it
            sm.mat.use_bump_map = false;
        }
    }

    // GLuint tex;
    // generate_texture(&tex);           // glGenTextures
    // bind_texture(GL_TEXTURE_2D, tex); // glBindTexture
    //
    // set_texture_image_2d(GL_TEXTURE_2D, // glTexImage2D
    //                      0, format, width, height, border, format, GL_UNSIGNED_BYTE, data);
    //
    // generate_mipmap(GL_TEXTURE_2D); // glGenerateMipmap
    //
    // set_texture_parameters(GL_TEXTURE_2D, {// glTexParameteri
    //                                        {GL_TEXTURE_WRAP_S, GL_REPEAT},
    //                                        {GL_TEXTURE_WRAP_T, GL_REPEAT},
    //                                        {GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR},
    //                                        {GL_TEXTURE_MAG_FILTER, GL_LINEAR}});
    //
    // unbind_texture(GL_TEXTURE_2D);

    // return tex;

}
