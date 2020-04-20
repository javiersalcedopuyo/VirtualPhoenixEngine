#ifndef VP_RESOURCES_LOADER_HPP
#define VP_RESOURCES_LOADER_HPP

#include <tiny_obj_loader.h>
#include <stb_image.h>

#include <vector>
#include <fstream>

#include "VPVertex.hpp"

namespace vpe {
namespace resourcesLoader
{
  struct ImageData
  {
    int      width;
    int      heigth;
    int      channels;
    int      mipLevels;
    VkFormat format;
    stbi_uc* pPixels = nullptr;

    inline int size() { return width * heigth * 4; }
  };

  static inline std::vector<char> parseShaderFile(const char* _fileName)
  {
    // Read the file from the end and as a binary file
    std::ifstream file(_fileName, std::ios::ate | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("ERROR: Couldn't open file"); //%s", _fileName);

    size_t fileSize = static_cast<size_t>(file.tellg());
    // We use a vector of chars instead of a char* or a string for more simplicity during the shader module creation
    std::vector<char> buffer(fileSize);

    // Go back to the beginning of the file and read all the bytes at once
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
  }

  static inline ImageData loadImage(const char* _path)
  {
    ImageData result{};

    result.format = VK_FORMAT_R8G8B8A8_UNORM;

    result.pPixels = stbi_load(_path, &result.width, &result.heigth, &result.channels, STBI_rgb_alpha);
    result.mipLevels = std::floor(std::log2(std::max(result.width, result.heigth))) + 1;

    if (result.pPixels == nullptr)
    {
      std::string errorText("ERROR: vpe::resourcesLoader::loadImage - Failed to load image!");
      throw std::runtime_error(errorText + " (" + _path + ")");
    }

    return result;
  }

  static inline std::pair< std::vector<Vertex>, std::vector<uint32_t> > loadModel(const char* _path)
  {
    std::vector<Vertex>              vertices;
    std::vector<uint32_t>            indices;

    tinyobj::attrib_t                attributes;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;
    std::string                      warning;
    std::string                      error;

    if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &warning, &error, _path))
      throw std::runtime_error(warning + error);

    std::unordered_map<Vertex, uint32_t> uniqueVertices;
    for (const auto& shape : shapes)
    {
      for (const auto& index : shape.mesh.indices)
      {
        Vertex vertex = {};

        if (index.vertex_index >=0)
        {
          vertex.pos =
          {
            attributes.vertices[3 * index.vertex_index + 0],
            attributes.vertices[3 * index.vertex_index + 1],
            attributes.vertices[3 * index.vertex_index + 2]
          };
        }

        if (index.normal_index >= 0)
        {
          vertex.normal =
          {
            attributes.normals[3 * index.normal_index + 0],
            attributes.normals[3 * index.normal_index + 1],
            attributes.normals[3 * index.normal_index + 2]
          };
        }

        if (index.texcoord_index >= 0)
        {
          vertex.texCoord =
          {
            attributes.texcoords[2 * index.texcoord_index + 0],
            1.0f - attributes.texcoords[2 * index.texcoord_index + 1]
          };
        }

        vertex.color = {0.0f, 0.0f, 0.0f};

        if (uniqueVertices.count(vertex) == 0)
        {
          uniqueVertices[vertex] = vertices.size();
          vertices.push_back(vertex);
        }

        indices.push_back(uniqueVertices[vertex]);
      }
    }

    return std::make_pair(vertices, indices);
  }
} // namespace VPResourcesLoader
} // namespace vpe

#endif