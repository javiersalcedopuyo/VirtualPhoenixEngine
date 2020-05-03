#include "VPResourcesLoader.hpp"
#include <iostream>

#ifndef NDEBUG
  #include <chrono>
#endif

namespace vpe::resourcesLoader
{
  std::vector<char> parseShaderFile(const char* _fileName)
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

  ImageData loadImage(const char* _path)
  {
    ImageData result;

    if (strcmp(_path, DEFAULT_TEX) == 0)
    {
      loadDefaultImage(&result);
    }
    else if (strcmp(_path, EMPTY_TEX) == 0)
    {
      loadEmptyImage(&result);
    }
    else
    {
      int dummy = 0;
      result.pPixels   = stbi_load(_path, &result.width, &result.heigth, &dummy, STBI_rgb_alpha);
      result.mipLevels = std::floor(std::log2(std::max(result.width, result.heigth))) + 1;
    }

    if (result.pPixels == nullptr) { loadEmptyImage(&result); }

    return result;
  }

  std::vector<uint32_t> extractIndicesFromMesh(const aiMesh* _pMesh)
  {
    std::vector<uint32_t> result;

    if (_pMesh == nullptr) return result;

    for (auto i=0u; i<_pMesh->mNumFaces; ++i)
    {
      const aiFace& face = _pMesh->mFaces[i];
      if (face.mNumIndices < 3) { continue; }
      if (face.mNumIndices > 3)
      {
        std::cout << "ERROR: resourcesLoader::extractIndicesFromMesh - "
                  << "Topologies with more than 3 vertices per face are not supported!" << std::endl;
        result.clear();
        break;
      }

      for(auto j=0u; j<face.mNumIndices; ++j)
        result.emplace_back(face.mIndices[j]);
    }

    return result;
  }

  std::vector<Vertex> extractVerticesFromMesh(const aiMesh* _pMesh)
  {
    std::vector<Vertex> result;

    if (_pMesh == nullptr || !_pMesh->HasPositions() || _pMesh->mNumVertices <= 0)
      return result;

    result.resize(_pMesh->mNumVertices);

    for (auto i=0u; i<_pMesh->mNumVertices; ++i)
    {
      const auto& aiPos = _pMesh->mVertices[i];
      result.at(i).pos = glm::vec3(aiPos.x, aiPos.y, aiPos.z);

      if (_pMesh->HasNormals())
      {
        const auto& N = _pMesh->mNormals[i];
        result.at(i).normal = glm::vec3(N.x, N.y, N.z);
      }

      for (auto j=0u; j<AI_MAX_NUMBER_OF_TEXTURECOORDS; ++j)
      {
        if (!_pMesh->HasTextureCoords(j)) continue;
        if (_pMesh->mNumUVComponents[j] != 2)
        {
          std::cout << "NOTE: resourcesLoader::extractVerticesFromMesh - "
                    << "Only 2D textures supported!" << std::endl;
          continue;
        }

        const auto& uvs = _pMesh->mTextureCoords[j][i];
        result.at(i).texCoord = glm::vec2(uvs.x, uvs.y);
        break;
      }
    }

    return result;
  }

  std::pair< std::vector<Vertex>, std::vector<uint32_t> > loadModel(const char* _path)
  {
#ifndef NDEBUG
    const auto startTime = std::chrono::high_resolution_clock::now();
#endif

    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;

    if (_path == nullptr) return std::make_pair(vertices, indices);

    Assimp::Importer importer;
    // Generate Smooth normals if not already present in the model
    importer.SetPropertyFloat("AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE", 80.0f);

    const auto postProcessingFlags = aiProcess_Triangulate |
                                     aiProcess_GenSmoothNormals |
                                     aiProcess_CalcTangentSpace;
    const aiScene* assimpScene = importer.ReadFile(_path, postProcessingFlags);

    // Load model into an Assimp's Scene, optimized for realtime
    //const aiScene* assimpScene = importer.ReadFile(_path, aiProcessPreset_TargetRealtime_MaxQuality);

    if (!assimpScene ||
         assimpScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
        !assimpScene->mRootNode)
    {
      std::cout << "ERROR: resourcesLoader::loadModel - " << _path
                << " model is not valid. Skipping." << std::endl;

      return std::make_pair(vertices, indices);
    }
    if (!assimpScene->HasMeshes())
    {
      std::cout << "ERROR: resourcesLoader::loadModel - " << _path
                << " has no meshes. Skipping." << std::endl;

      return std::make_pair(vertices, indices);
    }
    if (assimpScene->HasAnimations())
    {
      std::cout << "NOTE: resourcesLoader::loadModel - "
                << "No Animation support yet." << std::endl;
      // TODO:
    }
    if (assimpScene->HasLights())
    {
      std::cout << "NOTE: resourcesLoader::loadModel - "
                << "No support yet for loading lights from file." << std::endl;
      // TODO:
    }
    if (assimpScene->HasTextures() || assimpScene->HasMaterials())
    {
      std::cout << "NOTE: resourcesLoader::loadModel - "
                << "No support yet for loading materials/textures from model file." << std::endl;
      // TODO:
    }
    if (assimpScene->mNumMeshes > 1)
    {
      std::cout << "NOTE: resourcesLoader::loadModel - "
                << "No support yet for multiple meshes per file. Loading just the 1st" << std::endl;
      // TODO:
    }

    indices  = extractIndicesFromMesh(assimpScene->mMeshes[0]);
    vertices = extractVerticesFromMesh(assimpScene->mMeshes[0]);

#ifndef NDEBUG
    const auto currentTime = std::chrono::high_resolution_clock::now();
    const auto duration    = std::chrono::duration<float, std::chrono::milliseconds::period>
                             (currentTime - startTime).count();
    std::cout << _path << " loaded in " << duration << "ms." << std::endl;
#endif

    return std::make_pair(vertices, indices);
  }
} // namespace vpe::resourceslLoader