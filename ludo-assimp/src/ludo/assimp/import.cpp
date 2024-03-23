/*
 * This file is part of ludo. See the LICENSE file for the full license governing this code.
 */

#include <algorithm>
#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <FreeImagePlus.h>

#include <ludo/meshes.h>

#include "animation.h"
#include "import.h"
#include "math.h"

namespace ludo
{
  struct import_object
  {
    uint32_t mesh_index = 0;
    mat4 transform;
  };

  const auto bone_data_size = max_bone_weights_per_vertex * sizeof(uint32_t) + max_bone_weights_per_vertex * sizeof(float);

  mesh_buffer_options build_mesh_buffer_options(const aiScene& assimp_scene, const std::vector<import_object>& mesh_objects, const std::string& file_name, const import_options& options);
  void find_objects(const aiNode& assimp_node, std::vector<import_object>& mesh_objects, std::vector<import_object>& rigid_body_objects, const mat4& parent_transform, bool rigid_body_shapes);
  void import_meshes(instance& instance, mesh_buffer& mesh_buffer, const aiScene& assimp_scene, const std::vector<import_object>& mesh_objects, const import_options& options, const std::string& partition);
  void write_mesh_data(mesh_buffer& mesh_buffer, const aiScene& assimp_scene, const aiMesh& assimp_mesh, const mat4& transform, uint32_t index_start, uint32_t vertex_start);
  texture* import_texture(instance& instance, const std::string& file_name, const std::string& partition);
  void import_armature(instance& instance, const aiScene& assimp_scene, const aiMesh& assimp_mesh, const std::string& partition);
  void find_bone_path(const aiNode& assimp_node, const aiMesh& assimp_mesh, std::unordered_map<const aiNode*, bool>& bone_path);
  armature to_armature(const aiNode& assimp_node, const aiMesh& assimp_mesh, std::unordered_map<const aiNode*, bool>& bone_path, const mat4& root_matrix = mat4_identity, bool root_found = false);
  int32_t find_bone_index(const aiMesh& assimp_mesh, const std::string& name);
  void import_animations(instance& instance, const aiScene& assimp_scene, const aiMesh& assimp_mesh, const std::string& partition);
  void import_rigid_body_shape(instance& instance, const aiScene& assimp_scene, const import_object& rigid_body_object, const std::string& partition);
  void validate(const aiScene& assimp_scene, const std::vector<import_object>& mesh_objects);

  auto primitives = std::unordered_map<uint8_t, mesh_primitive>
  {
    { aiPrimitiveType_POINT, mesh_primitive::POINT_LIST },
    { aiPrimitiveType_LINE, mesh_primitive::LINE_LIST },
    { aiPrimitiveType_TRIANGLE, mesh_primitive::TRIANGLE_LIST }
  };

  mesh_buffer_options build_mesh_buffer_options(const std::string& file_name, const import_options& options)
  {
    Assimp::Importer importer;
    auto assimp_scene = importer.ReadFile(file_name, aiProcessPreset_TargetRealtime_MaxQuality);
    if (assimp_scene == nullptr)
    {
      std::cout << "Assimp error: " << importer.GetErrorString() << std::endl;
      assert(false && "Assimp error");
      return { 0, 0 };
    }

    auto mesh_objects = std::vector<import_object>();
    auto rigid_body_objects = std::vector<import_object>();
    find_objects(*assimp_scene->mRootNode, mesh_objects, rigid_body_objects, mat4_identity, false);
    validate(*assimp_scene, mesh_objects);

    return build_mesh_buffer_options(*assimp_scene, mesh_objects, file_name, options);
  }

  mesh_buffer_options build_mesh_buffer_options(const aiScene& assimp_scene, const std::vector<import_object>& mesh_objects, const std::string& file_name, const import_options& options)
  {
    auto index_count = uint32_t(0);
    auto vertex_count = uint32_t(0);
    auto color_count = uint32_t(0);
    auto bone_count = uint32_t(0);
    auto has_texture = false;

    for (auto& mesh_object : mesh_objects)
    {
      auto& assimp_mesh = *assimp_scene.mMeshes[mesh_object.mesh_index];

      index_count += assimp_mesh.mNumFaces * assimp_mesh.mFaces[0].mNumIndices; // We use aiProcess_SortByPType so all faces should have the same number of indices.
      vertex_count += assimp_mesh.mNumVertices;

      color_count = std::max(color_count, assimp_mesh.GetNumColorChannels());
      bone_count = std::max(bone_count, assimp_mesh.mNumBones);

      auto assimp_material = assimp_scene.mMaterials[assimp_mesh.mMaterialIndex];
      auto texture_path = aiString();
      assimp_material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), texture_path);
      if (texture_path.length > 0)
      {
        has_texture = true;
      }
    }

    return
    {
      .instance_count = options.instance_count,
      .index_count = index_count,
      .vertex_count = vertex_count,
      .normals = true,
      .colors = color_count > 0,
      .texture_count = static_cast<uint8_t>(has_texture ? 1 : 0),
      .bone_count = bone_count,
    };
  }

  void import(instance& instance, const std::string& file_name, const import_options& options, const std::string& partition)
  {
    Assimp::Importer importer;
    auto assimp_scene = importer.ReadFile(file_name, aiProcessPreset_TargetRealtime_MaxQuality);
    if (assimp_scene == nullptr)
    {
      std::cout << "Assimp error: " << importer.GetErrorString() << std::endl;
      assert(false && "Assimp error");
      return;
    }

    auto mesh_objects = std::vector<import_object>();
    auto rigid_body_objects = std::vector<import_object>();
    find_objects(*assimp_scene->mRootNode, mesh_objects, rigid_body_objects, mat4_identity, false);
    validate(*assimp_scene, mesh_objects);

    if (!mesh_objects.empty())
    {
      auto mesh_buffer = options.mesh_buffer;
      if (!mesh_buffer)
      {
        auto primitive = primitives[assimp_scene->mMeshes[mesh_objects[0].mesh_index]->mPrimitiveTypes]; // We use aiProcess_SortByPType so all primitives should be the same.
        auto mesh_buffer_options = build_mesh_buffer_options(*assimp_scene, mesh_objects, file_name, options);
        mesh_buffer = add(instance, ludo::mesh_buffer { .primitive = primitive }, mesh_buffer_options, partition);
      }

      import_meshes(instance, *mesh_buffer, *assimp_scene, mesh_objects, options, partition);

      auto assimp_material = assimp_scene->mMaterials[assimp_scene->mMeshes[mesh_objects[0].mesh_index]->mMaterialIndex];
      auto texture_path = aiString();
      assimp_material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), texture_path);
      if (texture_path.length)
      {
        // TODO not hardcode this path...
        auto texture = import_texture(instance, std::string("assets/models/") + texture_path.C_Str(), partition);
        set_texture(*mesh_buffer, *texture, 0);
      }
    }

    for (auto& rigid_body_object : rigid_body_objects)
    {
      import_rigid_body_shape(instance, *assimp_scene, rigid_body_object, partition);
    }
  }

  void find_objects(const aiNode& assimp_node, std::vector<import_object>& mesh_objects, std::vector<import_object>& rigid_body_objects, const mat4& parent_transform, bool rigid_body_shapes)
  {
    auto transform = parent_transform * to_mat4(assimp_node.mTransformation);
    rigid_body_shapes = rigid_body_shapes || std::string(assimp_node.mName.C_Str()).find("RigidBody") != std::string::npos;

    for (auto index = uint32_t(0); index < assimp_node.mNumMeshes; index++)
    {
      if (rigid_body_shapes)
      {
        rigid_body_objects.emplace_back(import_object { .mesh_index = assimp_node.mMeshes[index], .transform = transform });
      }
      else
      {
        mesh_objects.emplace_back(import_object { .mesh_index = assimp_node.mMeshes[index], .transform = transform });
      }
    }

    for (auto index = 0; index < assimp_node.mNumChildren; index++)
    {
      find_objects(*assimp_node.mChildren[index], mesh_objects, rigid_body_objects, transform, rigid_body_shapes);
    }
  }

  void import_meshes(instance& instance, mesh_buffer& mesh_buffer, const aiScene& assimp_scene, const std::vector<import_object>& mesh_objects, const import_options& options, const std::string& partition)
  {
    auto index_count = uint32_t(0);
    auto vertex_count = uint32_t(0);
    for (auto& mesh_object : mesh_objects)
    {
      auto& assimp_mesh = *assimp_scene.mMeshes[mesh_object.mesh_index];

      write_mesh_data(mesh_buffer, assimp_scene, assimp_mesh, mesh_object.transform, options.index_start + index_count, options.vertex_start + vertex_count);

      auto mesh_index_count = assimp_mesh.mNumFaces * assimp_mesh.mFaces[0].mNumIndices; // We use aiProcess_SortByPType so all faces should have the same number of indices.
      auto mesh_vertex_count = assimp_mesh.mNumVertices;

      if (!options.merge)
      {
        add(
          instance,
          mesh
          {
            .mesh_buffer_id = mesh_buffer.id,
            .index_buffer =
            {
              .data = mesh_buffer.index_buffer.data + (options.index_start + index_count) * sizeof(uint32_t),
              .size = mesh_index_count * sizeof(uint32_t),
            },
            .vertex_buffer =
            {
              .data = mesh_buffer.vertex_buffer.data + (options.vertex_start + vertex_count) * mesh_buffer.format.size,
              .size = mesh_vertex_count * mesh_buffer.format.size
            }
          },
          partition
        );

        if (assimp_mesh.mNumBones)
        {
          import_armature(instance, assimp_scene, assimp_mesh, partition);
          import_animations(instance, assimp_scene, assimp_mesh, partition);
        }
      }

      index_count += mesh_index_count;
      vertex_count += mesh_vertex_count;
    }

    if (options.merge)
    {
      add(
        instance,
        mesh
        {
          .mesh_buffer_id = mesh_buffer.id,
          .index_buffer =
          {
            .data = mesh_buffer.index_buffer.data + options.index_start * sizeof(uint32_t),
            .size = index_count * sizeof(uint32_t),
          },
          .vertex_buffer =
          {
            .data = mesh_buffer.vertex_buffer.data + options.vertex_start * mesh_buffer.format.size,
            .size = vertex_count * mesh_buffer.format.size
          }
        },
        partition
      );
    }
  }

  void write_mesh_data(mesh_buffer& mesh_buffer, const aiScene& assimp_scene, const aiMesh& assimp_mesh, const mat4& transform, uint32_t index_start, uint32_t vertex_start)
  {
    auto has_colors = assimp_mesh.GetNumColorChannels() > 0;
    auto has_bones = assimp_mesh.mNumBones > 0;
    auto texture_path = aiString();
    assimp_scene.mMaterials[assimp_mesh.mMaterialIndex]->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), texture_path);
    auto has_texture = texture_path.length > 0;

    auto bones_offset = sizeof(vec3) + sizeof(vec3) + (has_colors ? sizeof(vec4) : 0) + (has_texture ? sizeof(vec2) : 0);
    auto vertex_size = bones_offset;
    vertex_size += has_bones ? bone_data_size : 0;

    auto byte_index = index_start * sizeof(uint32_t);
    for (auto face_index = 0; face_index < assimp_mesh.mNumFaces; face_index++)
    {
      auto& assimp_face = assimp_mesh.mFaces[face_index];
      for (auto index_index = 0; index_index < assimp_face.mNumIndices; index_index++)
      {
        write(mesh_buffer.index_buffer, byte_index, vertex_start + assimp_face.mIndices[index_index]);
        byte_index += sizeof(uint32_t);
      }
    }

    byte_index = vertex_start * vertex_size;
    for (auto vertex_index = 0; vertex_index < assimp_mesh.mNumVertices; vertex_index++)
    {
      auto position = vec3(transform * vec4(to_vec3(assimp_mesh.mVertices[vertex_index])));
      write(mesh_buffer.vertex_buffer, byte_index, position);
      byte_index += sizeof(vec3);

      auto normal = mat3(transform) * to_vec3(assimp_mesh.mNormals[vertex_index]);
      write(mesh_buffer.vertex_buffer, byte_index, normal);
      byte_index += sizeof(vec3);

      if (has_colors)
      {
        write(mesh_buffer.vertex_buffer, byte_index, to_vec4(assimp_mesh.mColors[0][vertex_index]));
        byte_index += sizeof(vec4);
      }

      if (has_texture)
      {
        write(mesh_buffer.vertex_buffer, byte_index, to_vec2(assimp_mesh.mTextureCoords[0][vertex_index]));
        byte_index += sizeof(vec2);
      }

      if (has_bones)
      {
        // Initialize bone indices and weights to 0
        std::memset(mesh_buffer.vertex_buffer.data + byte_index, 0, bone_data_size);
        byte_index += bone_data_size;
      }
    }

    if (!has_bones)
    {
      return;
    }

    for (auto bone_index = 0u; bone_index < assimp_mesh.mNumBones; bone_index++)
    {
      auto assimp_bone = assimp_mesh.mBones[bone_index];
      for (auto weight_index = 0 ; weight_index < assimp_bone->mNumWeights ; weight_index++)
      {
        auto& assimp_vertex_weight = assimp_bone->mWeights[weight_index];

        auto first_bone_index_byte_index = (vertex_start + assimp_vertex_weight.mVertexId) * vertex_size + bones_offset;
        auto bone_index_byte_index = first_bone_index_byte_index;
        auto first_bone_weight_byte_index = first_bone_index_byte_index + max_bone_weights_per_vertex * sizeof(uint32_t);
        auto bone_weight_byte_index = first_bone_weight_byte_index;
        while (read<float>(mesh_buffer.vertex_buffer, bone_weight_byte_index) != 0.0f)
        {
          bone_index_byte_index += sizeof(uint32_t);
          bone_weight_byte_index += sizeof(float);

          assert(bone_index_byte_index < first_bone_weight_byte_index && "the maximum bone weights per vertex has been exceeded");
        }

        write(mesh_buffer.vertex_buffer, bone_index_byte_index, bone_index);
        write(mesh_buffer.vertex_buffer, bone_weight_byte_index, assimp_vertex_weight.mWeight);
      }
    }
  }

  texture* import_texture(instance& instance, const std::string& file_name, const std::string& partition)
  {
    auto image = fipImage();
    image.load(file_name.c_str());

    auto bits_per_pixel = image.getBitsPerPixel();
    if (bits_per_pixel != 24 && bits_per_pixel != 32)
    {
      assert(false && "unsupported pixel datatype");
    }

    auto color_type = image.getColorType();
    if (color_type != FIC_RGB && color_type != FIC_RGBALPHA)
    {
      assert(false && "unsupported pixel components");
    }

    if (FI_RGBA_BLUE != 0)
    {
      assert(false && "unsupported platform (need to implement solution for this)");
    }

    auto texture = add(
      instance,
      ludo::texture
      {
        .components = bits_per_pixel == 32 ? pixel_components::BGRA : pixel_components::BGR,
        .width = image.getWidth(),
        .height = image.getHeight()
      },
      partition
    );

    write(*texture, reinterpret_cast<std::byte*>(image.accessPixels()));

    return texture;
  }

  void import_armature(instance& instance, const aiScene& assimp_scene, const aiMesh& assimp_mesh, const std::string& partition)
  {
    auto bone_path = std::unordered_map<const aiNode*, bool>();
    find_bone_path(*assimp_scene.mRootNode, assimp_mesh, bone_path);

    auto bone_path_iter = std::find_if(bone_path.begin(), bone_path.end(), [](const std::pair<const aiNode*, bool>& entry)
    {
      return entry.second;
    });

    if (bone_path_iter == bone_path.end())
    {
      return;
    }

    add(instance, to_armature(*assimp_scene.mRootNode, assimp_mesh, bone_path), partition);
  }

  void find_bone_path(const aiNode& assimp_node, const aiMesh& assimp_mesh, std::unordered_map<const aiNode*, bool>& bone_path)
  {
    for (auto index = 0; index < assimp_node.mNumChildren; index++)
    {
      auto& assimp_child_node = *assimp_node.mChildren[index];
      find_bone_path(assimp_child_node, assimp_mesh, bone_path);
      if (bone_path[&assimp_child_node])
      {
        bone_path[&assimp_node] = true;
      }
    }

    if (bone_path[&assimp_node])
    {
      return;
    }

    bone_path[&assimp_node] = find_bone_index(assimp_mesh, std::string(assimp_node.mName.C_Str())) != -1;
  }

  armature to_armature(const aiNode& assimp_node, const aiMesh& assimp_mesh, std::unordered_map<const aiNode*, bool>& bone_path, const mat4& root_matrix, bool root_found)
  {
    auto bone_index = find_bone_index(assimp_mesh, std::string(assimp_node.mName.C_Str()));

    if (bone_index == -1 && !root_found)
    {
      auto root_bone_node = static_cast<aiNode*>(nullptr);
      for (auto index = 0; index < assimp_node.mNumChildren; index++)
      {
        auto child = assimp_node.mChildren[index];
        if (bone_path[child])
        {
          assert(!root_bone_node && "multiple root bones found");
          root_bone_node = child;
        }
      }

      return to_armature(*root_bone_node, assimp_mesh, bone_path, root_matrix * to_mat4(assimp_node.mTransformation));
    }

    auto node = armature { .transform = to_mat4(assimp_node.mTransformation) };
    if (!root_found)
    {
      // We need to apply the combined ancestor matrices as well to get the root of the armature in global space.
      // That way we can discard the ancestor nodes/matrices.
      node.transform = root_matrix * node.transform;
    }

    if (bone_index != -1)
    {
      node.bone_index = bone_index;

      // The bone offset matrices are in global space.
      // Since we transformed our mesh from mesh space -> global space already, we need to undo that transformation here.
      // If we don't it will result in a 'double application' of the transformation from mesh space -> global space.
      // Here I am actually transforming from global space -> armature space, which is the same in my example file.
      // I don't think this is actually correct since the bones are actually part of the mesh...
      auto root_matrix_inverse = root_matrix;
      invert(root_matrix_inverse);
      node.bone_offset = to_mat4(assimp_mesh.mBones[bone_index]->mOffsetMatrix) * root_matrix_inverse;
    }

    for (auto index = 0; index < assimp_node.mNumChildren; index++)
    {
      auto child = assimp_node.mChildren[index];
      if (bone_path[child])
      {
        node.children.emplace_back(to_armature(*child, assimp_mesh, bone_path, root_matrix, true));
      }
    }

    return node;
  }

  int32_t find_bone_index(const aiMesh& assimp_mesh, const std::string& name)
  {
    for (auto index = 0; index < assimp_mesh.mNumBones; index++)
    {
      auto assimp_bone = assimp_mesh.mBones[index];
      if (assimp_bone->mName.C_Str() == name)
      {
        return index;
      }
    }

    return -1;
  }

  void import_animations(instance& instance, const aiScene& assimp_scene, const aiMesh& assimp_mesh, const std::string& partition)
  {
    for (auto index = 0; index < assimp_scene.mNumAnimations; index++)
    {
      auto assimp_animation = assimp_scene.mAnimations[index];
      auto nodes = std::vector<animation_node>();

      for (auto node_anim_index = 0; node_anim_index < assimp_animation->mNumChannels; node_anim_index++)
      {
        auto assimp_node_anim = assimp_animation->mChannels[node_anim_index];
        auto node = animation_node();

        nodes.emplace_back(to_animation_node(*assimp_node_anim, find_bone_index(assimp_mesh, std::string(assimp_node_anim->mNodeName.C_Str()))));
      }

      add(
        instance,
        animation
        {
          .name = assimp_animation->mName.C_Str(),
          .ticks = static_cast<float>(assimp_animation->mDuration),
          // Sometimes Assimp cannot determine the ticks per second, we default to 24 in this case (the default from Blender)
          .ticks_per_second = static_cast<float>(assimp_animation->mTicksPerSecond == 0.0 ? 24.0 : assimp_animation->mTicksPerSecond),
          .nodes = nodes
        },
        partition
      );
    }
  }

  void import_rigid_body_shape(instance& instance, const aiScene& assimp_scene, const import_object& rigid_body_object, const std::string& partition)
  {
    auto& assimp_mesh = *assimp_scene.mMeshes[rigid_body_object.mesh_index];

    auto points = std::vector<vec3>();

    for (auto index = 0; index < assimp_mesh.mNumVertices; index++)
    {
      auto position = vec3(rigid_body_object.transform * vec4(to_vec3(assimp_mesh.mVertices[index])));

      auto point_exists = false;
      for (auto& point : points)
      {
        if (near(point, position))
        {
          point_exists = true;
          break;
        }
      }

      if (point_exists)
      {
        continue;
      }

      points.emplace_back(position);
    }

    add(instance, points, partition + "-rigid-body-shapes");
  }

  void validate(const aiScene& assimp_scene, const std::vector<import_object>& mesh_objects)
  {
    auto& first_assimp_mesh = *assimp_scene.mMeshes[mesh_objects[0].mesh_index];
    auto assimp_primitive = first_assimp_mesh.mPrimitiveTypes; // We use aiProcess_SortByPType so all primitives should be the same.
    auto assimp_material = assimp_scene.mMaterials[first_assimp_mesh.mMaterialIndex];

    for (auto& mesh_object : mesh_objects)
    {
      auto& assimp_mesh = *assimp_scene.mMeshes[mesh_object.mesh_index];

      assert(assimp_mesh.mPrimitiveTypes != aiPrimitiveType_POLYGON && "primitives must be points, lines or triangles");
      assert(assimp_mesh.mPrimitiveTypes == assimp_primitive && "meshes must have the same primitive");
      assert(assimp_scene.mMaterials[assimp_mesh.mMaterialIndex] == assimp_material && "meshes must have the same material");
    }
  }
}
