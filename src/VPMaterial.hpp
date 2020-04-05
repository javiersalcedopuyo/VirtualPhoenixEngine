#ifndef VP_MATERIAL_HPP
#define VP_MATERIAL_HPP

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

#include "VPImage.hpp"
#include "VPResourcesLoader.hpp"

const char* const DEFAULT_VERT = "../src/Shaders/vert.spv";
const char* const DEFAULT_FRAG = "../src/Shaders/frag.spv";
const char* const DEFAULT_TEX  = "../Textures/Default.png";

struct VPMaterial
{
  VPMaterial() : pTexture(nullptr), pPipeline(nullptr)
  {
    init(DEFAULT_VERT, DEFAULT_FRAG, DEFAULT_TEX);
  };

  VPMaterial(const char* _vert, const char* _frag, const char* _tex) :
    pTexture(nullptr),
    pPipeline(nullptr)
  {
    init(_vert, _frag, _tex);
  };

  ~VPMaterial() { cleanUp(); }

  size_t   hash;
  uint32_t mipLevels;

  std::vector<char> vertShaderCode;
  std::vector<char> fragShaderCode;

  VPImage*    pTexture;
  VkPipeline* pPipeline;

  inline void init(const char* _vert, const char* _frag, const char* _tex)
  {
    vertShaderCode = VPResourcesLoader::parseShaderFile(_vert);
    fragShaderCode = VPResourcesLoader::parseShaderFile(_frag);
    loadTexture(_tex);

    std::hash<std::string> hashFn;

    std::string id(_vert);
    id += _frag;

    hash = hashFn(id);
  }

  inline void loadTexture(const char* _texturePath)
  {
    if (pTexture != nullptr) this->cleanUp();

    pTexture = new VPImage();
    pTexture->loadFromFile(_texturePath);
  }

  inline void cleanUp()
  {
    delete pTexture;
    pTexture = nullptr;
  }

};

#endif