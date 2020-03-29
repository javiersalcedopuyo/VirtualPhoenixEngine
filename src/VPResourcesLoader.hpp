#ifndef VP_RESOURCES_LOADER_HPP
#define VP_RESOURCES_LOADER_HPP

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <tiny_obj_loader.h>

struct Vertex
{
  bool operator==(const Vertex& other) const
  {
    return pos      == other.pos    &&
           color    == other.color  &&
           normal   == other.normal &&
           texCoord == other.texCoord;
  }

  glm::vec2 texCoord;
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec3 normal;

  static inline VkVertexInputBindingDescription getBindingDescription()
  {
    VkVertexInputBindingDescription bd = {};
    bd.binding   = 0;
    bd.stride    = sizeof(Vertex);
    bd.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bd;
  }

  static inline std::array<VkVertexInputAttributeDescription,4> getAttributeDescriptions()
  {
    std::array<VkVertexInputAttributeDescription,4> descriptions = {};
    descriptions[0].binding  = 0;
    descriptions[0].location = 0;
    descriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    descriptions[0].offset   = offsetof(Vertex, pos);

    descriptions[1].binding  = 0;
    descriptions[1].location = 1;
    descriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    descriptions[1].offset   = offsetof(Vertex, color);

    descriptions[2].binding  = 0;
    descriptions[2].location = 2;
    descriptions[2].format   = VK_FORMAT_R32G32B32_SFLOAT;
    descriptions[2].offset   = offsetof(Vertex, normal);

    descriptions[3].binding  = 0;
    descriptions[3].location = 3;
    descriptions[3].format   = VK_FORMAT_R32G32_SFLOAT;
    descriptions[3].offset   = offsetof(Vertex, texCoord);

    return descriptions;
  }
};

// Needed to use Vertex as keys in an unordered map
namespace std {
  template <> struct hash<Vertex>
  {
    size_t operator()(Vertex const& vertex) const
    {
      return ((((hash<glm::vec3>()(vertex.pos) ^
                (hash<glm::vec3>()(vertex.color) << 1)) >>1) ^
                (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
                (hash<glm::vec2>()(vertex.texCoord) << 1);
    }
  };
}

namespace VPResourcesLoader
{
  static inline std::vector<char> parseShaderFile(const char* _fileName)
  {
    // Read the file from the end and as a binary file
    std::ifstream file(_fileName, std::ios::ate | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("ERROR: Couldn't open file"); //%s", _fileName);

    size_t fileSize = static_cast<size_t>(file.tellg());
    // We use a vector of chars instead of a char* or a string for more simplicity during the shader module creation
    std::vector<char> buffer(fileSize);

    // Go back to the beginning of the gile and read all the bytes at once
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
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
}
#endif