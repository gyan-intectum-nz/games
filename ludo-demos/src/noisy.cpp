#include <random>

#include <ludo/api.h>
#include <ludo/opengl/util.h>

int main()
{
  // SETUP

  auto inst = ludo::instance();
  ludo::allocate<ludo::script>(inst, 3);

  auto window = ludo::window { .title = "noisy!", .width = 1920, .height = 1080, .v_sync = false };
  ludo::init(window);

  auto rendering_context = ludo::rendering_context();
  ludo::init(rendering_context, 0);
  set_camera(rendering_context, ludo::camera
  {
    .view = ludo::mat4(ludo::vec3(0.0, 0.0, 300.0f), ludo::mat3_identity),
    .projection = ludo::perspective(60.0f, 16.0f / 9.0f, 0.1f, 1000.0f)
  });

  auto max_instance_count = uint32_t(1920 * 1080);
  auto circle_counts = ludo::circle_counts(ludo::vertex_format_pc, { .divisions = 20 });

  auto render_commands = ludo::allocate_heap_vram(2 * max_instance_count * sizeof(ludo::render_command));
  auto indices = ludo::allocate_heap_vram(2 * circle_counts.first * sizeof(uint32_t));
  auto vertices = ludo::allocate_heap_vram(2 * circle_counts.second * ludo::vertex_format_pc.size);

  // RENDER PROGRAMS

  auto render_programs = ludo::allocate_array<ludo::render_program>(1);
  auto render_program = ludo::add(render_programs, {});
  ludo::init(*render_program, ludo::vertex_format_pc, render_commands, 2 * max_instance_count);

  // MESHES

  auto exclusion_zone_mesh = ludo::mesh();
  ludo::init(exclusion_zone_mesh, indices, vertices, circle_counts.first, circle_counts.second, render_program->format.size);

  auto exclusion_zone_render_mesh = ludo::render_mesh { .instances = { 0, 0 } };
  ludo::init(exclusion_zone_render_mesh, *render_program, exclusion_zone_mesh, indices, vertices, max_instance_count);

  auto sample_mesh = ludo::mesh();
  ludo::init(sample_mesh, indices, vertices, circle_counts.first, circle_counts.second, render_program->format.size);

  auto sample_render_mesh = ludo::render_mesh { .instances = { max_instance_count, 0 } };
  ludo::init(sample_render_mesh, *render_program, sample_mesh, indices, vertices, max_instance_count);

  // POISSON DISC
  // Based on https://www.cs.ubc.ca/~rbridson/docs/bridson-siggraph07-poissondisk.pdf
  auto random = std::mt19937(123456);
  std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
  std::uniform_real_distribution<float> distribution2(-1.0f, 1.0f);
  auto minimum_distance = 10.0f / 180.0f * ludo::pi;
  auto maximum_attempts = 30;

  ludo::circle(sample_mesh, render_program->format, 0, 0, { .dimensions = { minimum_distance * 0.1f, 0.0f, 0.0f }, .divisions = 20, .color = { 0.0f, 0.0f, 0.5f, 1.0f } });
  ludo::circle(exclusion_zone_mesh, render_program->format, 0, 0, { .dimensions = { minimum_distance, 0.0f, 0.0f }, .divisions = 20, .color = { 0.0f, 0.0f, 1.0f, 1.0f } });

  auto samples = std::vector<ludo::vec3> { { 0.0f, 1.0f, 0.0f } };
  auto active_samples = std::vector<ludo::vec3> { { 0.0f, 1.0f, 0.0f } }; // TODO make this a random point

  while (!active_samples.empty())
  {
    auto active_index = uint32_t(distribution(random) * float(active_samples.size()));
    auto& active_sample = active_samples[active_index];

    auto sample_added = false;
    for (auto attempt = 0; attempt < maximum_attempts; attempt++)
    {
      auto random_unit_vector = ludo::vec3 { distribution2(random), distribution2(random), distribution2(random) };
      ludo::normalize(random_unit_vector);
      auto axis = ludo::cross(active_sample, random_unit_vector);
      ludo::normalize(axis);
      auto angle = minimum_distance + distribution(random) * minimum_distance;

      auto sample = active_sample;
      ludo::rotate(sample, axis, angle);

      bool rejected = false;
      for (auto& existing_sample : samples)
      {
        if (ludo::angle_between(sample, existing_sample) < minimum_distance)
        {
          rejected = true;
          break;
        }
      }

      if (!rejected)
      {
        samples.push_back(sample);
        active_samples.push_back(sample);
        sample_added = true;
        break;
      }
    }

    if (!sample_added)
    {
      active_samples.erase(active_samples.begin() + active_index);
    }
  }

  for (auto& sample : samples)
  {
    auto rotation = ludo::mat3(ludo::quat(ludo::vec3_unit_z, sample));

    ludo::instance_transform(exclusion_zone_render_mesh, exclusion_zone_render_mesh.instances.count++) = ludo::mat4(sample, rotation);
    ludo::instance_transform(sample_render_mesh, sample_render_mesh.instances.count++) = ludo::mat4(sample * 1.0001f, rotation);
  }

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

    auto camera = ludo::get_camera(rendering_context);
    camera.view = ludo::mat4(ludo::vec3_zero, ludo::mat3(ludo::quat(float((*mouse_movement_accumulator)[0]) / 500.0f, float((*mouse_movement_accumulator)[1]) / 500.0f, 0.0f)));
    ludo::translate(camera.view, { 0.0f, 0.0f, 3.0f });
    ludo::set_camera(rendering_context, camera);
  });

  ludo::add<ludo::script>(inst, [&](ludo::instance& inst)
  {
    ludo::start_render_transaction(rendering_context, render_programs);
    ludo::swap_buffers(window);

    ludo::use_and_clear(ludo::frame_buffer { .width = 1920, .height = 1080 });

    ludo::add_render_command(*render_program, exclusion_zone_render_mesh);
    ludo::add_render_command(*render_program, sample_render_mesh);

    ludo::commit_render_commands(rendering_context, render_programs, render_commands, indices, vertices);
    ludo::commit_render_transaction(rendering_context);
  });

  // PLAY

  ludo::play(inst);
}
