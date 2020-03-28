#ifndef VP_MATERIAL_HPP
#define VP_MATERIAL_HPP

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <vector>

#include "VPImage.hpp"

const char* const DEFAULT_VERT = "../src/Shaders/vert.spv";
const char* const DEFAULT_FRAG = "../src/Shaders/frag.spv";
const char* const DEFAULT_TEX  = "../Textures/Default.png";

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
    vertShaderCode = parseShaderFile(_vert);
    fragShaderCode = parseShaderFile(_frag);
    loadTexture(_tex);

    std::hash<std::string> hashFn;

    std::string id(_vert);
    id += _frag;

    hash = hashFn(id);
  }

  inline void loadTexture(const char* _texturePath)
  {
    if (!pTexture) pTexture = new VPImage();

    pTexture->loadFromFile(_texturePath);
  }

  inline void cleanUp()
  {
    delete pTexture;
    pTexture = nullptr;
  }

};

#endif