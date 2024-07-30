#include "btBulletDynamicsCommon.h"

#include <ludo/api.h>
#include <ludo/bullet/math.h>
#include <ludo/opengl/util.h>

int main()
{
  // SETUP

  auto inst = ludo::instance();
  ludo::allocate<ludo::script>(inst, 3);

  auto window = ludo::window { .title = "physicy!", .width = 1920, .height = 1080, .v_sync = false };
  ludo::init(window);

  auto rendering_context = ludo::rendering_context();
  ludo::init(rendering_context, 1);

  auto box_counts = ludo::box_counts(ludo::vertex_format_p);

  auto render_commands = ludo::allocate_heap_vram(sizeof(ludo::render_command));
  auto indices = ludo::allocate_heap_vram((10000 + 2 * box_counts.first) * sizeof(uint32_t));
  auto vertices = ludo::allocate_heap_vram((10000 + 2 * box_counts.second) * ludo::vertex_format_pc.size);

  // RENDER PROGRAMS

  auto render_programs = ludo::allocate_array<ludo::render_program>(1);
  auto render_program = ludo::add(render_programs, { .primitive = ludo::mesh_primitive::LINE_LIST });
  ludo::init(*render_program, ludo::vertex_format_pc, render_commands, 1);

  // MESHES

  auto floor_mesh = ludo::mesh();
  ludo::init(floor_mesh, indices, vertices, box_counts.first, box_counts.second, ludo::vertex_format_p.size);
  ludo::box(floor_mesh, ludo::vertex_format_p, 0, 0, { .dimensions = ludo::vec3(25.0f, 1.0f, 25.0f) });

  auto box_mesh = ludo::mesh();
  ludo::init(box_mesh, indices, vertices, box_counts.first, box_counts.second, ludo::vertex_format_p.size);
  ludo::box(box_mesh, ludo::vertex_format_p, 0, 0);

  auto box_positions = std::vector<ludo::vec3>(8);
  std::memcpy(box_positions.data(), box_mesh.vertex_buffer.data, box_mesh.vertex_buffer.size);

  // PHYSICS

  auto physics_context = ludo::physics_context();
  ludo::init(physics_context);

  auto floor = ludo::static_body { .transform = { .position = ludo::vec3(0.0f, -2.0f, 0.0f) } };
  ludo::init(floor, physics_context);
  ludo::connect(floor, physics_context, floor_mesh, ludo::vertex_format_p);

  auto box_shape = ludo::dynamic_body_shape { .convex_hulls = { box_positions } };
  ludo::init(box_shape);

  auto box = ludo::dynamic_body { .transform = { .position = ludo::vec3(0.0f, 2.0f, -10.0f) }, .mass = 1.0f };
  ludo::init(box, physics_context);
  ludo::connect(box, physics_context, { box_shape });

  auto kinematic_box = ludo::kinematic_body { .transform = { .position = ludo::vec3(-5.0f, 2.0f, -10.0f) } };
  ludo::init(kinematic_box, physics_context);
  ludo::connect(kinematic_box, physics_context, { box_shape });

  auto ghost_box = ludo::ghost_body { .transform = { .position = ludo::vec3(5.0f, 2.0f, -10.0f) } };
  ludo::init(ghost_box, physics_context);
  ludo::connect(ghost_box, physics_context, { box_shape });

  auto physics_mesh = ludo::mesh();
  ludo::init(physics_mesh, indices, vertices, 10000, 10000, ludo::vertex_format_pc.size);

  auto physics_render_mesh = ludo::render_mesh();
  ludo::init(physics_render_mesh, *render_program, physics_mesh, indices, vertices, 1);

  // SCRIPTS

  ludo::add<ludo::script>(inst, [&](ludo::instance& inst)
  {
    ludo::receive_input(window, inst);

    if (window.active_window_frame_button_states[ludo::window_frame_button::CLOSE] == ludo::button_state::UP)
    {
      ludo::stop(inst);
    }
  });

  ludo::add<ludo::script>(inst, [&](ludo::instance& inst)
  {
    if (window.active_mouse_button_states[ludo::mouse_button::LEFT] == ludo::button_state::HOLD)
    {
      kinematic_box.transform.position[0] += float(window.mouse_movement[0]) / 100.0f;
      kinematic_box.transform.position[1] += float(window.mouse_movement[1]) / -100.0f;
      ludo::commit(kinematic_box);
    }

    if (window.active_mouse_button_states[ludo::mouse_button::RIGHT] == ludo::button_state::HOLD)
    {
      ghost_box.transform.position[0] += float(window.mouse_movement[0]) / 100.0f;
      ghost_box.transform.position[1] += float(window.mouse_movement[1]) / -100.0f;
      ludo::commit(ghost_box);
    }

    ludo::simulate(physics_context, inst.delta_time);
  });

  ludo::add<ludo::script>(inst, [&](ludo::instance& inst)
  {
    ludo::start_render_transaction(rendering_context, render_programs);
    ludo::swap_buffers(window);

    ludo::use_and_clear(ludo::frame_buffer { .width = 1920, .height = 1080 });

    ludo::visualize(physics_context, physics_mesh);
    ludo::add_render_command(*render_program, physics_render_mesh);

    ludo::commit_render_commands(rendering_context, render_programs, render_commands, indices, vertices);
    ludo::commit_render_transaction(rendering_context);
  });

  // PLAY

  ludo::play(inst);
}
