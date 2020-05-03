#ifndef VP_VERTEX_HPP
#define VP_VERTEX_HPP

#ifndef GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_LEFT_HANDED
#endif

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

namespace vpe
{
struct Vertex
{
  bool operator==(const Vertex& other) const
  {
    return pos      == other.pos    &&
           normal   == other.normal &&
           texCoord == other.texCoord;
  }

  glm::vec2 texCoord;
  glm::vec3 pos;
  glm::vec3 normal;

  static inline VkVertexInputBindingDescription getBindingDescription()
  {
    VkVertexInputBindingDescription bd = {};
    bd.binding   = 0;
    bd.stride    = sizeof(Vertex);
    bd.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bd;
  }

  static inline std::array<VkVertexInputAttributeDescription,3> getAttributeDescriptions()
  {
    std::array<VkVertexInputAttributeDescription,3> descriptions{};
    descriptions[0].binding  = 0;
    descriptions[0].location = 0;
    descriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    descriptions[0].offset   = offsetof(Vertex, pos);

    descriptions[1].binding  = 0;
    descriptions[1].location = 1;
    descriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    descriptions[1].offset   = offsetof(Vertex, normal);

    descriptions[2].binding  = 0;
    descriptions[2].location = 2;
    descriptions[2].format   = VK_FORMAT_R32G32_SFLOAT;
    descriptions[2].offset   = offsetof(Vertex, texCoord);

    return descriptions;
  }
};
} // namespace vpe

namespace std
{
// Needed to use Vertex as keys in an unordered map
template <> struct hash<vpe::Vertex>
{
  size_t operator()(vpe::Vertex const& vertex) const
  {
    return ((hash<glm::vec3>()(vertex.pos) ^
            (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
            (hash<glm::vec2>()(vertex.texCoord) << 1);
  }
};
} // namespace std
#endif