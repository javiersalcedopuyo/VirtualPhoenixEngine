#ifndef VP_MATERIAL_HPP
#define VP_MATERIAL_HPP

#include <vulkan/vulkan.h>

#include <string>
#include <vector>
#include <memory>

#include "VPImage.hpp"
#include "VPResourcesLoader.hpp"

namespace vpe
{
constexpr uint8_t IMAGES_PER_MATERIAL = 2;

struct StdMaterial
{
  StdMaterial() = delete;

  StdMaterial(const char* _vert, const char* _frag) :
    pTexture(nullptr),
    pNormalMap(nullptr)
  {
    vertShaderCode = resourcesLoader::parseShaderFile(_vert);
    fragShaderCode = resourcesLoader::parseShaderFile(_frag);
    changeTexture(DEFAULT_TEX);
    changeNormalMap(EMPTY_TEX);

    hash = hashFn( {_vert, _frag, std::to_string(IMAGES_PER_MATERIAL).c_str()} );
  };

  ~StdMaterial()
  {
    pTexture.reset();
    pNormalMap.reset();
  }

  std::unique_ptr<Image> pTexture;
  std::unique_ptr<Image> pNormalMap;
  size_t hash;

  std::vector<char> vertShaderCode;
  std::vector<char> fragShaderCode;

  static inline size_t hashFn(const std::vector<const char*>& _input)
  {
    std::hash<std::string> stringHash;

    std::string id;
    for (auto& c : _input) id += c;

    return stringHash(id);
  }

  inline void changeTexture(const char* _path) { pTexture.reset( new Image(_path) ); }
  inline void changeNormalMap(const char* _path) { pNormalMap.reset( new Image(_path) ); }
};
}
#endif