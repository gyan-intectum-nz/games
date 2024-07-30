#include <ludo/api.h>
#include <ludo/opengl/util.h>

int main()
{
  // SETUP

  auto inst = ludo::instance();
  ludo::allocate<ludo::script>(inst, 3);

  auto window = ludo::window { .title = "spinny!", .width = 1920, .height = 1080, .v_sync = false };
  ludo::init(window);

  auto rendering_context = ludo::rendering_context();
  ludo::init(rendering_context, 1);

  auto instance_count = 3;
  auto box_counts = ludo::box_counts(ludo::vertex_format_pt);

  auto render_commands = ludo::allocate_heap_vram(instance_count * sizeof(ludo::render_command));
  auto indices = ludo::allocate_heap_vram(instance_count * box_counts.first * sizeof(uint32_t));
  auto vertices = ludo::allocate_heap_vram(instance_count * box_counts.second * ludo::vertex_format_pc.size);

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

  auto render_programs = ludo::allocate_array<ludo::render_program>(3);
  auto render_program_p = ludo::add(render_programs, {});
  ludo::init(*render_program_p, ludo::vertex_format_p, render_commands, 1);
  auto render_program_pc = ludo::add(render_programs, {});
  ludo::init(*render_program_pc, ludo::vertex_format_pc, render_commands, 1);
  auto render_program_pt = ludo::add(render_programs, {});
  ludo::init(*render_program_pt, ludo::vertex_format_pt, render_commands, 1);

  // TEXTURES

  auto texture = ludo::load(ludo::asset_folder + "/models/minifig-diffuse.png");

  // CUBY

  auto cuby_mesh = ludo::mesh();
  ludo::init(cuby_mesh, indices, vertices, box_counts.first, box_counts.second, render_program_p->format.size);
  ludo::box(cuby_mesh, render_program_p->format, 0, 0);

  auto cuby_render_mesh = ludo::render_mesh();
  ludo::init(cuby_render_mesh, *render_program_p, cuby_mesh, indices, vertices, 1);

  // RUBY

  auto ruby_mesh = ludo::mesh();
  ludo::init(ruby_mesh, indices, vertices, box_counts.first, box_counts.second, render_program_pc->format.size);
  ludo::box(ruby_mesh, render_program_pc->format, 0, 0, { .color = ludo::vec4(1.0f, 0.0f, 0.0f, 1.0f) });

  auto ruby_render_mesh = ludo::render_mesh();
  ludo::init(ruby_render_mesh, *render_program_pc, ruby_mesh, indices, vertices, 1);

  // TUBY

  auto tuby_mesh = ludo::mesh { .texture_id = texture.id };
  ludo::init(tuby_mesh, indices, vertices, box_counts.first, box_counts.second, render_program_pt->format.size);
  ludo::box(tuby_mesh, render_program_pt->format, 0, 0);

  auto tuby_render_mesh = ludo::render_mesh();
  ludo::init(tuby_render_mesh, *render_program_pt, tuby_mesh, indices, vertices, 1);

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
    ludo::instance_transform(cuby_render_mesh) = ludo::mat4(ludo::vec3(-2.5f, 0.0f, -4.0f), ludo::mat3(ludo::vec3_unit_y, inst.total_time));
    ludo::instance_transform(ruby_render_mesh) = ludo::mat4(ludo::vec3(0.0f, 0.0f, -4.0f), ludo::mat3(ludo::vec3_unit_y, inst.total_time));
    ludo::instance_transform(tuby_render_mesh) = ludo::mat4(ludo::vec3(2.5f, 0.0f, -4.0f), ludo::mat3(ludo::vec3_unit_y, inst.total_time));
  });

  ludo::add<ludo::script>(inst, [&](ludo::instance& inst)
  {
    ludo::start_render_transaction(rendering_context, render_programs);
    ludo::swap_buffers(window);

    ludo::use_and_clear(ludo::frame_buffer { .width = 1920, .height = 1080 });

    ludo::add_render_command(*render_program_p, cuby_render_mesh);
    ludo::add_render_command(*render_program_pc, ruby_render_mesh);
    ludo::add_render_command(*render_program_pt, tuby_render_mesh);

    ludo::commit_render_commands(rendering_context, render_programs, render_commands, indices, vertices);
    ludo::commit_render_transaction(rendering_context);
  });

  // PLAY

  ludo::play(inst);
}
