#ifndef VP_RESOURCES_LOADER_HPP
#define VP_RESOURCES_LOADER_HPP

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <stb_image.h>

#include <vector>
#include <fstream>

#include "VPVertex.hpp"

namespace vpe
{
const char* const DEFAULT_VERT = "../src/Shaders/vert.spv";
const char* const DEFAULT_FRAG = "../src/Shaders/frag.spv";
const char* const DEFAULT_TEX  = "VP_DEFAULT_TEX";
const char* const EMPTY_TEX    = "VP_EMPTY_TEX";
} // namespace vpe

namespace vpe::resourcesLoader
{
  struct ImageData
  {
    int      width     = 1;
    int      heigth    = 1;
    int      channels  = 4;
    int      mipLevels = 1;
    VkFormat format    = VK_FORMAT_R8G8B8A8_UNORM;
    stbi_uc* pPixels   = nullptr;

    inline int size() { return width * heigth * channels; }
  };

  inline void loadDefaultImage(ImageData* _data)
  {
    _data->pPixels = static_cast<stbi_uc*>( malloc(4) ); // stbi uses free to clean up
    memset(_data->pPixels, 255, 4);
  }

  inline void loadEmptyImage(ImageData* _data)
  {
    _data->pPixels = static_cast<stbi_uc*>( malloc(4) ); // stbi uses free to clean up
    memset(_data->pPixels, 0, 4);
  }

  ImageData             loadImage(const char* _path);
  std::vector<char>     parseShaderFile(const char* _fileName);
  std::vector<Vertex>   extractVerticesFromMesh(const aiMesh* _pMesh);
  std::vector<uint32_t> extractIndicesFromMesh(const aiMesh* _pMesh);

  std::pair< std::vector<Vertex>, std::vector<uint32_t> > loadModel(const char* _path);

} // namespace vpe::resourcesLoader

#endif