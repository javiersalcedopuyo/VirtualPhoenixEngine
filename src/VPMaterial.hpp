#ifndef VP_MATERIAL_HPP
#define VP_MATERIAL_HPP

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

#include "VPImage.hpp"
#include "VPResourcesLoader.hpp"

namespace vpe
{
const char* const DEFAULT_VERT = "../src/Shaders/vert.spv";
const char* const DEFAULT_FRAG = "../src/Shaders/frag.spv";
const char* const DEFAULT_TEX  = "../Textures/Default.png";

struct Material
{
  Material() : pTexture(nullptr), pPipeline(nullptr)
  {
    init(DEFAULT_VERT, DEFAULT_FRAG, DEFAULT_TEX);
  };

  Material(const char* _vert, const char* _frag, const char* _tex) :
    pTexture(nullptr),
    pPipeline(nullptr)
  {
    init(_vert, _frag, _tex);
  };

  ~Material() { cleanUp(); }

  size_t hash;

  std::vector<char> vertShaderCode;
  std::vector<char> fragShaderCode;

  Image*      pTexture;
  VkPipeline* pPipeline;

  inline void init(const char* _vert, const char* _frag, const char* _tex)
  {
    vertShaderCode = resourcesLoader::parseShaderFile(_vert);
    fragShaderCode = resourcesLoader::parseShaderFile(_frag);
    loadTexture(_tex);

    std::hash<std::string> hashFn;

    std::string id(_vert);
    id += _frag;

    hash = hashFn(id);
  }

  inline void loadTexture(const char* _texturePath)
  {
    if (pTexture != nullptr) this->cleanUp();

    pTexture = new Image(_texturePath);
  }

  inline void cleanUp()
  {
    delete pTexture;
    pTexture = nullptr;
  }
};
}
#endif