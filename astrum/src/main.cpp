#include <iomanip>
#include <iostream>

#include <ludo/api.h>
#include <ludo/opengl/util.h>

#include "constants.h"
#include "post-processing/atmosphere.h"
#include "post-processing/bloom.h"
#include "post-processing/tone_mapping.h"
#include "post-processing/pass.h"
#include "post-processing/util.h"
#include "solar_system.h"
#include "terrain/terrain.h"
#include "util.h"

int main()
{
  auto timer = ludo::timer();

  auto inst = ludo::instance();

  auto max_terrain_bodies = 25;
  auto post_processing_rectangle_counts = ludo::rectangle_counts(ludo::vertex_format_pt);
  auto sol_mesh_counts = astrum::terrain_counts(astrum::sol_lods);
  auto terra_mesh_counts = astrum::terrain_counts(astrum::terra_lods);
  auto luna_mesh_counts = astrum::terrain_counts(astrum::luna_lods);
  auto tree_counts = std::array<std::pair<uint32_t, uint32_t>, astrum::tree_type_count>
  {
    astrum::import_assets ? ludo::import_counts(ludo::asset_folder + "/models/fruit-tree.dae") : ludo::mesh_counts(ludo::asset_folder + "/meshes/fruit-tree-0.lmesh"),
    astrum::import_assets ? ludo::import_counts(ludo::asset_folder + "/models/oak-tree.dae") : ludo::mesh_counts(ludo::asset_folder + "/meshes/oak-tree-0.lmesh"),
    astrum::import_assets ? ludo::import_counts(ludo::asset_folder + "/models/palm-tree.dae") : ludo::mesh_counts(ludo::asset_folder + "/meshes/palm-tree-0.lmesh"),
    astrum::import_assets ? ludo::import_counts(ludo::asset_folder + "/models/pine-tree.dae") : ludo::mesh_counts(ludo::asset_folder + "/meshes/pine-tree-0.lmesh")
  };
  auto person_mesh_counts = ludo::import_counts(ludo::asset_folder + "/models/minifig.dae");
  auto spaceship_mesh_counts = ludo::import_counts(ludo::asset_folder + "/models/spaceship.dae");
  auto bullet_debug_counts = std::pair<uint32_t, uint32_t> { max_terrain_bodies * 48 * 2, max_terrain_bodies * 48 * 2 };

  auto max_rendered_instances =
    14 + // post-processing
    3 * (5120 * 2) + // terrains (doubled to account for re-allocations - overkill)
    5120 * 200 + // trees
    1 + // person
    1; // spaceship
  auto max_indices =
    post_processing_rectangle_counts.first +
    sol_mesh_counts.first +
    terra_mesh_counts.first +
    luna_mesh_counts.first +
    tree_counts[0].first +
    tree_counts[1].first +
    tree_counts[2].first +
    tree_counts[3].first +
    person_mesh_counts.first +
    spaceship_mesh_counts.first;
  auto max_vertices =
    post_processing_rectangle_counts.second +
    sol_mesh_counts.second +
    terra_mesh_counts.second +
    luna_mesh_counts.second +
    tree_counts[0].second +
    tree_counts[1].second +
    tree_counts[2].second +
    tree_counts[3].second +
    person_mesh_counts.second +
    spaceship_mesh_counts.second;

  if (astrum::show_paths)
  {
    max_rendered_instances += 5;
    max_indices += astrum::path_steps * 5;
    max_vertices += astrum::path_steps * 5;
  }

  if (astrum::visualize_physics)
  {
    max_rendered_instances++;
    max_indices += bullet_debug_counts.first;
    max_vertices += bullet_debug_counts.second;
  }

  ludo::allocate<ludo::physics_context>(inst, 1);
  ludo::allocate<ludo::rendering_context>(inst, 1);

  ludo::allocate<ludo::animation>(inst, 1);
  ludo::allocate<ludo::armature>(inst, 1);
  ludo::allocate<ludo::compute_program>(inst, 5);
  ludo::allocate<ludo::dynamic_body>(inst, 0);
  ludo::allocate<ludo::dynamic_body_shape>(inst, 3);
  ludo::allocate<ludo::frame_buffer>(inst, 16);
  ludo::allocate<ludo::ghost_body>(inst, 1);
  ludo::allocate<ludo::grid3>(inst, 5);
  ludo::allocate<ludo::kinematic_body>(inst, 2);
  ludo::allocate<ludo::mesh>(inst, max_rendered_instances);
  ludo::allocate<ludo::render_mesh>(inst, max_rendered_instances);
  ludo::allocate<ludo::render_program>(inst, 12);
  ludo::allocate<ludo::script>(inst, 36);
  ludo::allocate<ludo::static_body>(inst, max_terrain_bodies);
  ludo::allocate<ludo::texture>(inst, 21);
  ludo::allocate<ludo::window>(inst, 1);

  ludo::allocate<astrum::celestial_body>(inst, 3);
  ludo::allocate<astrum::game_controls>(inst, 1);
  ludo::allocate<astrum::map_controls>(inst, 1);
  ludo::allocate<astrum::person>(inst, 1);
  ludo::allocate<astrum::person_controls>(inst, 1);
  ludo::allocate<astrum::point_mass>(inst, 5);
  ludo::allocate<astrum::solar_system>(inst, 1);
  ludo::allocate<astrum::spaceship_controls>(inst, 1);
  ludo::allocate<astrum::terrain>(inst, 3);

  auto window = ludo::add(inst, ludo::window { .title = "astrum", .width = 1920, .height = 1080, .v_sync = false });
  ludo::init(*window);
  //ludo::capture_mouse(*window);

  auto rendering_context = ludo::add(inst, ludo::rendering_context());
  ludo::init(*rendering_context, 1);

  auto& render_commands = ludo::allocate_heap_vram(inst, "ludo::vram_render_commands", max_rendered_instances * sizeof(ludo::render_command));
  auto& indices = ludo::allocate_heap_vram(inst, "ludo::vram_indices", max_indices * sizeof(uint32_t));
  auto& vertices = ludo::allocate_heap_vram(inst, "ludo::vram_vertices", max_vertices * ludo::vertex_format_pnc.size);

  auto default_grid = ludo::add(
    inst,
    ludo::grid3
      {
        .bounds =
        {
          .min = { -2.0f * astrum::astronomical_unit, -2.0f * astrum::astronomical_unit, -2.0f * astrum::astronomical_unit },
          .max = { 2.0f * astrum::astronomical_unit, 2.0f * astrum::astronomical_unit, 2.0f * astrum::astronomical_unit }
        },
        .cell_count_1d = 16
      }
  );
  default_grid->compute_program_id = ludo::add(inst, ludo::build_compute_program(*default_grid))->id;
  ludo::init(*default_grid);

  auto msaa_color_texture = ludo::add(inst, ludo::texture { .datatype = ludo::pixel_datatype::FLOAT16, .width = window->width, .height = window->height });
  ludo::init(*msaa_color_texture, { .samples = astrum::msaa_samples });
  auto msaa_depth_texture = ludo::add(inst, ludo::texture { .components = ludo::pixel_components::DEPTH, .datatype = ludo::pixel_datatype::FLOAT32, .width = window->width, .height = window->height });
  ludo::init(*msaa_depth_texture, { .samples = astrum::msaa_samples });
  auto msaa_frame_buffer = ludo::add(inst, ludo::frame_buffer { .width = window->width, .height = window->height, .color_texture_ids = { msaa_color_texture->id }, .depth_texture_id = msaa_depth_texture->id });
  ludo::init(*msaa_frame_buffer);

  auto physics_context = ludo::add(inst, ludo::physics_context { .gravity = ludo::vec3_zero });
  ludo::init(*physics_context);

  ludo::thread_pool_start();

  std::cout << std::fixed << std::setprecision(4) << "main load time: " << ludo::elapsed(timer) << "s" << std::endl;
  ludo::reset(timer);

  astrum::add_solar_system(inst);

  std::cout << std::fixed << std::setprecision(4) << "solar system load time: " << ludo::elapsed(timer) << "s" << std::endl;
  ludo::reset(timer);

  ludo::commit(*default_grid);

  ludo::add<ludo::script>(inst, [](ludo::instance& inst)
  {
    auto window = ludo::first<ludo::window>(inst);

    ludo::receive_input(*window, inst);

    if (window->active_window_frame_button_states[ludo::window_frame_button::CLOSE] == ludo::button_state::UP)
    {
      ludo::stop(inst);
    }
  });

  ludo::add<ludo::script>(inst, [](ludo::instance& inst)
  {
    auto msaa_frame_buffer = ludo::first<ludo::frame_buffer>(inst);
    auto rendering_context = ludo::first<ludo::rendering_context>(inst);
    auto& render_programs = ludo::data<ludo::render_program>(inst);
    auto window = ludo::first<ludo::window>(inst);

    ludo::start_render_transaction(*rendering_context, render_programs);
    ludo::swap_buffers(*window);

    ludo::use_and_clear(*msaa_frame_buffer);
  });

  ludo::add<ludo::script>(inst, [](ludo::instance& inst)
  {
    auto& grids = ludo::data<ludo::grid3>(inst);
    auto rendering_context = ludo::first<ludo::rendering_context>(inst);
    auto& compute_programs = ludo::data<ludo::compute_program>(inst);
    auto& render_programs = ludo::data<ludo::render_program>(inst);

    auto& render_commands = ludo::data_heap(inst, "ludo::vram_render_commands");

    ludo::add_render_commands(grids, compute_programs, render_programs, render_commands, ludo::get_camera(*rendering_context));
  });

  if (astrum::visualize_physics)
  {
    auto bullet_debug_render_program = ludo::add(inst, ludo::render_program { .primitive = ludo::mesh_primitive::LINE_LIST }, "physics");
    ludo::init(*bullet_debug_render_program, ludo::vertex_format_pc, render_commands, 1);

    auto bullet_debug_mesh = ludo::add(inst, ludo::mesh(), "physics");
    ludo::init(*bullet_debug_mesh, indices, vertices, bullet_debug_counts.first, bullet_debug_counts.second, bullet_debug_render_program->format.size);

    auto bullet_debug_render_mesh = ludo::add(inst, ludo::render_mesh(), "physics");
    ludo::init(*bullet_debug_render_mesh, *bullet_debug_render_program, *bullet_debug_mesh, indices, vertices, 1);

    ludo::add<ludo::script>(inst, [](ludo::instance& inst)
    {
      auto mesh = ludo::first<ludo::mesh>(inst, "physics");
      auto physics_context = ludo::first<ludo::physics_context>(inst);
      auto render_mesh = ludo::first<ludo::render_mesh>(inst, "physics");
      auto render_program = ludo::first<ludo::render_program>(inst, "physics");

      ludo::visualize(*physics_context, *mesh);
      ludo::add_render_command(*render_program, *render_mesh);
    });
  }

  ludo::add<ludo::script>(inst, [](ludo::instance& inst)
  {
    auto rendering_context = ludo::first<ludo::rendering_context>(inst);
    auto& render_programs = ludo::data<ludo::render_program>(inst);

    auto& render_commands = ludo::data_heap(inst, "ludo::vram_render_commands");
    auto& indices = ludo::data_heap(inst, "ludo::vram_indices");
    auto& vertices = ludo::data_heap(inst, "ludo::vram_vertices");

    ludo::commit_render_commands(*rendering_context, render_programs, render_commands, indices, vertices);
  });

  // Post-processing
  auto post_processing_render_mesh = astrum::add_post_processing_render_mesh(inst);
  astrum::add_pass(inst); // Implicitly converts MSAA textures to regular textures
  //astrum::write_atmosphere_textures(astrum::terra_atmosphere_scale);
  astrum::add_atmosphere(inst, *post_processing_render_mesh, 1, astrum::terra_radius, astrum::terra_radius * astrum::terra_atmosphere_scale);
  astrum::add_bloom(inst, *post_processing_render_mesh, 5, 0.1f);
  astrum::add_tone_mapping(inst, *post_processing_render_mesh);
  astrum::add_pass(inst, true);

  ludo::add<ludo::script>(inst, [](ludo::instance& inst)
  {
    ludo::commit_render_transaction(*ludo::first<ludo::rendering_context>(inst));
  });

  ludo::add<ludo::script>(inst, astrum::print_timings);

  std::cout << std::fixed << std::setprecision(4) << "remaining load time: " << ludo::elapsed(timer) << "s" << std::endl;

  ludo::play(inst);
}
