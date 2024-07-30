#include <future>

#include <ludo/api.h>

#include "constants.h"
#include "controllers/game.h"
#include "entities/luna.h"
#include "entities/people.h"
#include "entities/sol.h"
#include "entities/spaceships.h"
#include "entities/terra.h"
#include "entities/trees.h"
#include "meshes/lods.h"
#include "paths.h"
#include "physics/centering.h"
#include "physics/gravity.h"
#include "physics/point_masses.h"
#include "physics/relativity.h"
#include "physics/util.h"
#include "solar_system.h"
#include "terrain/terrain.h"

namespace astrum
{
  void add_solar_system(ludo::instance& inst)
  {
    auto& indices = ludo::data_heap(inst, "ludo::vram_indices");
    auto& vertices = ludo::data_heap(inst, "ludo::vram_vertices");

    auto tree_collapse_iterations = std::vector<uint32_t> { 200, 50, 12 };
    auto tree_lod_meshes = std::array<std::vector<ludo::mesh>, tree_type_count>();
    if (import_assets)
    {
      for (auto tree_type_index = uint32_t(0); tree_type_index < tree_types.size(); tree_type_index++)
      {
        auto tree_import = ludo::import(ludo::asset_folder + "/models/" + tree_types[tree_type_index] + "-tree.dae", indices, vertices, { .merge_meshes = true });
        tree_lod_meshes[tree_type_index] = build_lod_meshes(tree_import.meshes[0], ludo::vertex_format_pnc, indices, vertices, tree_collapse_iterations);
        std::reverse(tree_lod_meshes[tree_type_index].begin(), tree_lod_meshes[tree_type_index].end());
        for (auto lod_index = uint32_t(0); lod_index < tree_collapse_iterations.size(); lod_index++)
        {
          ludo::save(tree_lod_meshes[tree_type_index][lod_index], ludo::asset_folder + "/meshes/" + tree_types[tree_type_index] + "-tree-" + std::to_string(lod_index) + ".lmesh");
        }
      }
    }
    else
    {
      for (auto tree_type_index = uint32_t(0); tree_type_index < tree_types.size(); tree_type_index++)
      {
        for (auto lod_index = uint32_t(0); lod_index < tree_collapse_iterations.size(); lod_index++)
        {
          tree_lod_meshes[tree_type_index].emplace_back(ludo::load(ludo::asset_folder + "/meshes/" + tree_types[tree_type_index] + "-tree-" + std::to_string(lod_index) + ".lmesh", indices, vertices));
        }
      }
    }

    for (auto tree_type_index = uint32_t(0); tree_type_index < tree_types.size(); tree_type_index++)
    {
      for (auto lod_index = uint32_t(0); lod_index < tree_collapse_iterations.size(); lod_index++)
      {
        ludo::add(inst, tree_lod_meshes[tree_type_index][lod_index], tree_types[tree_type_index] + "-trees");
      }
    }

    auto minifig = ludo::import(ludo::asset_folder + "/models/minifig.dae", indices, vertices);
    ludo::add(inst, minifig.animations[0], "people");
    ludo::add(inst, minifig.armatures[0], "people");
    ludo::add(inst, minifig.dynamic_body_shapes[0], "people");
    ludo::add(inst, minifig.meshes[0], "people");
    ludo::add(inst, minifig.textures[0], "people");

    auto spaceship = ludo::import(ludo::asset_folder + "/models/spaceship.dae", indices, vertices);
    ludo::add(inst, spaceship.meshes[0], "spaceships");
    ludo::add(inst, spaceship.dynamic_body_shapes[0], "spaceships");
    ludo::add(inst, spaceship.dynamic_body_shapes[1], "spaceships");

    ludo::add(inst, solar_system());

    const auto terra_initial_position = ludo::vec3 { -1.0f * astronomical_unit, 0.0f, 0.0f };
    const auto terra_initial_velocity = ludo::vec3 { 0.0f, orbital_speed(ludo::length(terra_initial_position), sol_mass), 0.0f };

    const auto luna_initial_position = ludo::vec3 { -1.0f * astronomical_unit + luna_orbit_distance, 0.0f, 0.0f };
    const auto luna_initial_velocity = terra_initial_velocity + ludo::vec3 { 0.0f, orbital_speed(luna_orbit_distance, terra_mass), 0.0f };

    auto person_surface_position = ludo::vec3 { 4138.0f, -3054.0f, 3800.0f };
    const auto person_initial_position = terra_initial_position + person_surface_position;
    const auto person_initial_velocity = ludo::vec3 { 0.0f, orbital_speed(ludo::length(person_initial_position), sol_mass), 0.0f };

    const auto spaceship_initial_position = terra_initial_position + ludo::vec3 { 4154.0f, -3046.0f, 3785.0f };
    const auto spaceship_initial_velocity = ludo::vec3 { 0.0f, orbital_speed(ludo::length(spaceship_initial_position), sol_mass), 0.0f };

    // Initialize camera to roughly correct position to ensure the correct terrain LODs are pre-loaded
    auto& rendering_context = *ludo::first<ludo::rendering_context>(inst);
    ludo::set_camera(
      rendering_context,
      {
        .near_clipping_distance = 0.1f,
        .far_clipping_distance = 2.0f * astronomical_unit,
        .view = ludo::mat4(person_initial_position, ludo::mat3_identity),
        .projection = ludo::perspective(60.0f, 16.0f / 9.0f, 0.1f, 2.0f * astronomical_unit)
      }
    );

    add_sol(inst, {}, ludo::vec3_zero);
    add_terra(inst, { .position = terra_initial_position }, terra_initial_velocity);
    add_luna(inst, { .position = luna_initial_position }, luna_initial_velocity);

    add_trees(inst, 1);

    add_person(inst, { .position = person_initial_position }, person_initial_velocity);

    auto spaceship_up = spaceship_initial_position - terra_initial_position;
    ludo::normalize(spaceship_up);
    add_spaceship(inst, { .position = spaceship_initial_position, .rotation = ludo::quat(ludo::vec3_unit_y, spaceship_up) * ludo::quat(ludo::vec3_unit_y, ludo::pi * 1.5f) }, spaceship_initial_velocity);

    ludo::add(inst, game_controls());
    ludo::add(inst, map_controls { .target_radius = sol_radius });

    if (show_paths)
    {
      add_prediction_paths(
        inst,
        {
          { 1.0f, 1.0f, 0.0f, 1.0f },
          { 0.0f, 0.5f, 1.0f, 1.0f },
          { 1.0f, 1.0f, 1.0f, 1.0f },
          { 1.0f, 0.0f, 0.0f, 1.0f },
          { 0.0f, 1.0f, 0.0f, 1.0f }
        }
      );
    }

    ludo::add<ludo::script>(inst, center_universe);
    ludo::add<ludo::script>(inst, relativize_universe);

    ludo::add<ludo::script>(inst, simulate_gravity);
    ludo::add<ludo::script>(inst, [](ludo::instance& inst)
    {
      auto physics_context = ludo::first<ludo::physics_context>(inst);

      ludo::simulate(*physics_context, inst.delta_time);
    });
    ludo::add<ludo::script, std::vector<std::string>>(inst, simulate_point_mass_physics, { "people", "spaceships" });

    ludo::add<ludo::script>(inst, stream_terrain);
    ludo::add<ludo::script, uint32_t>(inst, stream_trees, 1);

    ludo::add<ludo::script>(inst, sync_light_with_sol);

    ludo::add<ludo::script>(inst, simulate_people);
    ludo::add<ludo::script>(inst, simulate_spaceships);

    ludo::add<ludo::script>(inst, control_game);

    ludo::add<ludo::script, std::vector<std::string>>(inst, sync_render_meshes_with_point_masses, { "people", "spaceships" });

    if (show_paths)
    {
      ludo::add<ludo::script>(inst, update_prediction_paths);
    }
  }
}
