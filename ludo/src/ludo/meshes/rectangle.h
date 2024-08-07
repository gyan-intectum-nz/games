/*
 * This file is part of ludo. See the LICENSE file for the full license governing this code.
 */

#pragma once

#include "../meshes.h"

namespace ludo
{
  void rectangle(mesh& mesh, const vertex_format& format, uint32_t& index_index, uint32_t& vertex_index, const vec3& position_bottom_left, const vec3& position_delta_right, const vec3& position_delta_top, const vec2& tex_coord_min, const vec2& tex_coord_delta, bool unique_only, bool no_normal_check, const vec4& color, uint32_t divisions);

  void rectangle(mesh& mesh, const vertex_format& format, uint32_t& index_index, uint32_t& vertex_index, const vec3& position_bottom_left, const vec3& position_delta_right, const vec3& position_delta_top, const vec2& tex_coord_min, const vec2& tex_coord_delta, bool unique_only, bool no_normal_check, const vec4& color);
}
