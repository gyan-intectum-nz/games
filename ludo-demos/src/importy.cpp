#include <ludo/api.h>
#include <ludo/opengl/util.h>

int main()
{
  // SETUP

  auto inst = ludo::instance();
  ludo::allocate<ludo::script>(inst, 3);

  auto window = ludo::window { .title = "importy!", .width = 1920, .height = 1080, .v_sync = false };
  ludo::init(window);

  auto rendering_context = ludo::rendering_context();
  ludo::init(rendering_context, 1);

  auto minifig_counts = ludo::import_counts(ludo::asset_folder + "/models/minifig.dae");

  auto render_commands = ludo::allocate_heap_vram(sizeof(ludo::render_command));
  auto indices = ludo::allocate_heap_vram( minifig_counts.first * sizeof(uint32_t));
  auto vertices = ludo::allocate_heap_vram(minifig_counts.second * 80); // 80 is a rough guess

  // LIGHTS

  ludo::set_light(rendering_context, ludo::light
  {
    .ambient = { 0.01f, 0.01f, 0.01f, 1.0f },
    .diffuse = { 0.7f, 0.7f, 0.7f, 1.0f },
    .specular = { 0.01f, 0.01f, 0.01f, 1.0f },
    .position = ludo::vec3(0.0f, 4.0f, 0.0f),
    .attenuation = { 1.0f, 0.0f, 0.0f },
    .strength = 1.0f,
    .range = 10.0f
  }, 0);

  // RENDER PROGRAMS

  auto render_programs = ludo::allocate_array<ludo::render_program>(1);
  auto render_program = ludo::add(render_programs, {});
  ludo::init(*render_program, ludo::format(true, true, true, true), render_commands, 1);

  // MINIFIG

  auto minifig = ludo::import(ludo::asset_folder + "/models/minifig.dae", indices, vertices);

  auto render_mesh = ludo::render_mesh();
  ludo::init(render_mesh, *render_program, minifig.meshes[0], indices, vertices, 1);

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
    ludo::instance_transform(render_mesh) = ludo::mat4(ludo::vec3(0.0f, 0.0f, -3.0f), ludo::mat3(ludo::vec3_unit_y, inst.total_time));
    ludo::interpolate(minifig.animations[0], minifig.armatures[0], inst.total_time, ludo::instance_bone_transforms(render_mesh));
  });

  ludo::add<ludo::script>(inst, [&](ludo::instance& inst)
  {
    ludo::start_render_transaction(rendering_context, render_programs);
    ludo::swap_buffers(window);

    ludo::use_and_clear(ludo::frame_buffer { .width = 1920, .height = 1080 });

    ludo::add_render_command(*render_program, render_mesh);

    ludo::commit_render_commands(rendering_context, render_programs, render_commands, indices, vertices);
    ludo::commit_render_transaction(rendering_context);
  });

  // PLAY

  ludo::play(inst);
}
