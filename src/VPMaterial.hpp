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
struct StdMaterial
{
  StdMaterial() : pTexture(nullptr)
  {
    init(DEFAULT_VERT, DEFAULT_FRAG, DEFAULT_TEX);
  };

  StdMaterial(const char* _vert, const char* _frag, const char* _tex) : pTexture(nullptr)
  {
    init(_vert, _frag, _tex);
  };

  ~StdMaterial() { cleanUp(); }

  std::unique_ptr<Image> pTexture;
  size_t hash;

  std::vector<char> vertShaderCode;
  std::vector<char> fragShaderCode;

  static inline size_t hashFn(std::vector<const char*> _input)
  {
    std::hash<std::string> stringHash;

    std::string id;
    for (auto& c : _input) id += c;

    return stringHash(id);
  }

  inline void init(const char* _vert, const char* _frag, const char* _tex)
  {
    vertShaderCode = resourcesLoader::parseShaderFile(_vert);
    fragShaderCode = resourcesLoader::parseShaderFile(_frag);
    changeTexture(_tex);

    const char* textureCount = "1";
    hash = hashFn( {_vert, _frag, textureCount} );
  }

  inline void changeTexture(const char* _texturePath)
  {
    pTexture.reset( new Image(_texturePath) );
  }

  inline void cleanUp() { pTexture.reset(); }
};
}
#endif