#ifndef VP_LIGHT_HPP
#define VP_LIGHT_HPP

#ifndef GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_LEFT_HANDED
#endif

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

enum class LightType : uint8_t
{
  POINT = 0,
  SPOT,
  DIRECTIONAL,
  AREA
};

struct alignas(32) LightUBO
{
  alignas(4)  float     intensity;
  alignas(4)  float     range;
  alignas(4)  float     spotAngle;
  alignas(16) glm::vec3 position;
  alignas(16) glm::vec3 color;
  alignas(16) glm::vec3 forward;
};

struct VPLight
{
  VPLight() : idx(0) {};
  VPLight(uint32_t _idx) : idx(_idx) {};
  VPLight(LightType _type, uint32_t _idx, LightUBO& _ubo) :
    type(_type),
    idx(_idx),
    ubo(_ubo)
  {};

  LightType type;
  const uint32_t  idx;
  LightUBO  ubo;
};

#endif