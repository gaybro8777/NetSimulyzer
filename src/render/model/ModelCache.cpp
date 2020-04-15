/*
 * NIST-developed software is provided by NIST as a public service. You may use,
 * copy and distribute copies of the software in any medium, provided that you
 * keep intact this entire notice. You may improve,modify and create derivative
 * works of the software or any portion of the software, and you may copy and
 * distribute such modifications or works. Modified works should carry a notice
 * stating that you changed the software and should note the date and nature of
 * any such change. Please explicitly acknowledge the National Institute of
 * Standards and Technology as the source of the software.
 *
 * NIST-developed software is expressly provided "AS IS." NIST MAKES NO
 * WARRANTY OF ANY KIND, EXPRESS, IMPLIED, IN FACT OR ARISING BY OPERATION OF
 * LAW, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTY OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, NON-INFRINGEMENT
 * AND DATA ACCURACY. NIST NEITHER REPRESENTS NOR WARRANTS THAT THE
 * OPERATION OF THE SOFTWARE WILL BE UNINTERRUPTED OR ERROR-FREE, OR THAT
 * ANY DEFECTS WILL BE CORRECTED. NIST DOES NOT WARRANT OR MAKE ANY
 * REPRESENTATIONS REGARDING THE USE OF THE SOFTWARE OR THE RESULTS THEREOF,
 * INCLUDING BUT NOT LIMITED TO THE CORRECTNESS, ACCURACY, RELIABILITY,
 * OR USEFULNESS OF THE SOFTWARE.
 *
 * You are solely responsible for determining the appropriateness of using and
 * distributing the software and you assume all risks associated with its use,
 * including but not limited to the risks and costs of program errors,
 * compliance with applicable laws, damage to or loss of data, programs or
 * equipment, and the unavailability or interruption of operation. This
 * software is not intended to be used in any situation where a failure could
 * cause risk of injury or damage to property. The software developed by NIST
 * employees is not subject to copyright protection within the United States.
 *
 * Author: Evan Black <evan.black@nist.gov>
 */

#include "ModelCache.h"
#include "../shader/Shader.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <unordered_map>

#include <assimp/Importer.hpp>
#include <iostream>

namespace visualization {

void ModelRenderInfo::loadNode(aiNode const *node, aiScene const *scene) {
  for (auto i = 0u; i < node->mNumMeshes; i++) {
    loadMesh(scene->mMeshes[node->mMeshes[i]], scene);
  }

  for (auto i = 0u; i < node->mNumChildren; i++) {
    loadNode(node->mChildren[i], scene);
  }
}

void ModelRenderInfo::loadMesh(aiMesh const *m, aiScene const *) {
  std::vector<Vertex> vertices;
  vertices.reserve(m->mNumVertices);

  std::vector<unsigned int> indices;

  for (auto i = 0u; i < m->mNumVertices; i++) {
    Vertex v;
    v.position = {m->mVertices[i].x, m->mVertices[i].y, m->mVertices[i].z};

    // if we have at least one texture
    if (m->mTextureCoords[0])
      v.textureCoordinate = {m->mTextureCoords[0][i].x, m->mTextureCoords[0][i].y};
    else
      v.textureCoordinate = {0.0f, 0.0f};

    // Normals should point away
    v.normal = {-m->mNormals[i].x, -m->mNormals[i].y, -m->mNormals[i].z};

    vertices.emplace_back(v);
  }

  for (auto i = 0u; i < m->mNumFaces; i++) {
    const auto &face = m->mFaces[i];

    for (auto j = 0u; j < face.mNumIndices; j++) {
      indices.emplace_back(face.mIndices[j]);
    }
  }

  meshes.emplace_back(vertices.data(), indices.data(), vertices.size(), indices.size())
      .setMaterial(materials[m->mMaterialIndex]);
}

ModelRenderInfo::ModelRenderInfo(aiScene const *scene, TextureCache &textureCache) : textureCache(textureCache) {
  initializeOpenGLFunctions();
  loadMaterials(scene);
  loadNode(scene->mRootNode, scene);
}

ModelRenderInfo::ModelRenderInfo(std::vector<Mesh> meshes, TextureCache &textureCache)
    : meshes(std::move(meshes)), textureCache(textureCache) {
  initializeOpenGLFunctions();
}

ModelRenderInfo::~ModelRenderInfo() {
  clear();
}

void ModelRenderInfo::loadMaterials(aiScene const *scene) {
  auto fallbackTexture = textureCache.getFallbackTexture();

  for (auto i = 0u; i < scene->mNumMaterials; i++) {
    auto const *material = scene->mMaterials[i];
    Material m;

    if (material->GetTextureCount(aiTextureType_DIFFUSE)) {
      aiString path;

      if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
        std::string pathCppString{path.data};
        // Strips back to the last '/' character (i.e. '/home/evan/projects' -> 'projects')
        auto filepath = "resources/textures/" + pathCppString.substr(pathCppString.rfind('\\') + 1);
        m.textureId = textureCache.load(filepath);
      } else if (fallbackTexture.has_value()) {
        m.textureId = *fallbackTexture;
      } else {
        std::cerr << "No fallback texture defined!\n";
        abort();
      }
    } else if (fallbackTexture.has_value()) {
      m.textureId = *fallbackTexture;
    } else {
      std::cerr << "No fallback texture defined!\n";
    }

    material->Get(AI_MATKEY_SHININESS, m.shininess);                  // Errors Ignored
    material->Get(AI_MATKEY_SHININESS_STRENGTH, m.specularIntensity); // Errors Ignored

    materials.emplace_back(m);
  }
}

void ModelRenderInfo::render(Shader &) {
  for (auto &m : meshes) {
    // Operator [] for unordered map is not const...
    const auto &material = m.getMaterial();

    if (material.textureId)
      textureCache.use(*material.textureId);

    //    s.set_uniform_vector_1f("material.specularIntensity", material.specular_intensity);
    //    s.set_uniform_vector_1f("material.shininess", material.shininess);

    m.render();
  }
}

void ModelRenderInfo::clear() {
  meshes.clear();
}

ModelCache::ModelCache(TextureCache &textureCache) : textureCache(textureCache) {
}

ModelCache::~ModelCache() {
  clear();
}

void ModelCache::init(const std::string &fallbackModelPath) {
  initializeOpenGLFunctions();
  load(fallbackModelPath);
}

unsigned long ModelCache::load(const std::string &path) {
  auto existing = indexMap.find(path);
  if (existing != indexMap.end()) {
    return existing->second;
  }

  Assimp::Importer importer;
  const auto *const scene =
      importer.ReadFile(path.c_str(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals |
                                          aiProcess_JoinIdenticalVertices);
  if (!scene) {
    std::cerr << "Model (" << path << ") failed to load: " << importer.GetErrorString() << '\n';

    // Make sure we have a fallback model
    if (models.empty()) {
      std::cerr << "Failed loading fallback model at: " << path << '\n';
      std::abort();
    }

    return fallbackModel;
  }

  models.emplace_back(scene, textureCache);
  indexMap.emplace(path, models.size() - 1);
  return models.size() - 1;
}

ModelRenderInfo &ModelCache::get(std::size_t index) {
  return models[index];
}

void ModelCache::clear() {
}

void ModelCache::render(std::size_t index, Shader &s) {
  models[index].render(s);
}

} // namespace visualization
