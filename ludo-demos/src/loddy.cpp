#include <iomanip>
#include <iostream>

#include <ludo/api.h>
#include <ludo/opengl/util.h>

#include "../constants.h"
#include "../meshes/lod_shaders.h"

int main()
{
  // SETUP

  auto timer = ludo::timer();

  auto inst = ludo::instance();
  ludo::allocate<ludo::script>(inst, 3);

  auto window = ludo::window { .title = "loddy!", .width = 1920, .height = 1080, .v_sync = false };
  ludo::init(window);

  auto rendering_context = ludo::rendering_context();
  ludo::init(rendering_context, 1);

  auto tree_counts = ludo::import_counts("assets/models/oak-tree.dae");

  auto render_commands = ludo::allocate_heap_vram(2 * sizeof(ludo::render_command));
  auto indices = ludo::allocate_heap_vram(3 * tree_counts.first * sizeof(uint32_t));
  auto vertices = ludo::allocate_heap_vram(3 * tree_counts.second * 80);

  // LIGHTS

  ludo::set_light(rendering_context, ludo::light
  {
    .ambient = { 0.01f, 0.01f, 0.01f, 1.0f },
    .diffuse = { 0.7f, 0.7f, 0.7f, 1.0f },
    .specular = { 0.01f, 0.01f, 0.01f, 1.0f },
    .position = ludo::vec3(0.0f, 4.0f, 0.0f),
    .attenuation = { 1.0f, 0.0f, 0.0f },
    .strength = 1.0f,
    .range = 100.0f
  }, 0);

  // RENDER PROGRAMS

  auto lod_format = ludo::vertex_format_pnc;
  lod_format.components.insert(lod_format.components.end(), ludo::vertex_format_pnc.components.begin(), ludo::vertex_format_pnc.components.end());
  lod_format.size *= 2;

  auto render_programs = ludo::allocate_array<ludo::render_program>(1);
  auto render_program = ludo::add(
    render_programs,
    {
      .format = lod_format,
      .shader_buffer = ludo::allocate_dual(2 * 2 * sizeof(float)),
      .instance_size = sizeof(ludo::mat4) + sizeof(uint32_t) + 12 // align 16
    }
  );
  auto vertex_shader_code = astrum::lod_vertex_shader_code(ludo::vertex_format_pnc, false);
  auto fragment_shader_code = astrum::lod_fragment_shader_code(ludo::vertex_format_pnc, false);
  ludo::init(*render_program, vertex_shader_code, fragment_shader_code, render_commands, 2);

  // TREE

  auto tree_import = ludo::import("assets/models/oak-tree.dae", indices, vertices, { .merge_meshes = true });
  auto lod_meshes = astrum::build_lod_meshes(tree_import.meshes[0], ludo::vertex_format_pnc, indices, vertices, { 200, 50 });

  auto lod_render_mesh_0 = ludo::render_mesh();
  ludo::init(lod_render_mesh_0, *render_program, lod_meshes[0], indices, vertices, 1);
  ludo::cast<uint32_t>(lod_render_mesh_0.instance_buffer, sizeof(ludo::mat4)) = uint32_t(0);

  auto lod_render_mesh_1 = ludo::render_mesh { .instances = { 1, 1 } };
  ludo::init(lod_render_mesh_1, *render_program, lod_meshes[1], indices, vertices, 1);
  ludo::cast<uint32_t>(lod_render_mesh_1.instance_buffer, sizeof(ludo::mat4)) = uint32_t(1);

  // SCRIPTS

  ludo::add<ludo::script>(inst, [&](ludo::instance& inst)
  {
    ludo::receive_input(window, inst);

    if (window.active_window_frame_button_states[ludo::window_frame_button::CLOSE] == ludo::button_state::UP)
    {
      ludo::stop(inst);
    }
  });

  auto mouse_movement_accumulator = new std::array<int32_t, 2>();
  ludo::add<ludo::script>(inst, [&](ludo::instance& inst)
  {
    if (window.active_mouse_button_states[ludo::mouse_button::LEFT] == ludo::button_state::HOLD)
    {
      (*mouse_movement_accumulator)[0] += window.mouse_movement[0];
      (*mouse_movement_accumulator)[1] += window.mouse_movement[1];
    }

    auto rotation = ludo::quat(float((*mouse_movement_accumulator)[1]) / 250.0f, float((*mouse_movement_accumulator)[0]) / 250.0f, 0.0f);

    ludo::instance_transform(lod_render_mesh_0) = ludo::mat4(ludo::vec3(-6.0f, -4.0f, -16.0f), ludo::mat3(rotation));
    ludo::instance_transform(lod_render_mesh_1) = ludo::mat4(ludo::vec3(6.0f, -4.0f, -16.0f), ludo::mat3(rotation));

    auto time = std::sin(inst.total_time / 2.0f) * 0.5f + 0.5f;
    auto stream = ludo::stream(render_program->shader_buffer.back);
    ludo::write(stream, 16.0f + 32.0f * time);
    ludo::write(stream, -16.0f + 32.0f * time);
    ludo::write(stream, 16.0f + 32.0f * time);
    ludo::write(stream, -16.0f + 32.0f * time);
  });

  ludo::add<ludo::script>(inst, [&](ludo::instance& inst)
  {
    ludo::start_render_transaction(rendering_context, render_programs);
    ludo::swap_buffers(window);

    ludo::use_and_clear(ludo::frame_buffer { .width = 1920, .height = 1080 });

    ludo::add_render_command(*render_program, lod_render_mesh_0);
    ludo::add_render_command(*render_program, lod_render_mesh_1);

    ludo::commit_render_commands(rendering_context, render_programs, render_commands, indices, vertices);
    ludo::commit_render_transaction(rendering_context);
  });

  std::cout << std::fixed << std::setprecision(4) << "load time (seconds): " << ludo::elapsed(timer) << std::endl;

  // PLAY

  ludo::play(inst);
}
