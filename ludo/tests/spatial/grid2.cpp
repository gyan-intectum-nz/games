/*
 * This file is part of ludo. See the LICENSE file for the full license governing this code.
 */

#include <ludo/spatial/grid2.h>
#include <ludo/testing.h>

#include "grid2.h"

namespace ludo
{
  std::vector<uint64_t> cell_render_mesh_ids(const grid2& grid, uint32_t cell_index);

  void test_spatial_grid2()
  {
    test_group("grid2");

    auto bounds_1 = aabb2 { .min = { -1.0f, -1.0f }, .max = { 1.0f, 1.0f } };
    auto bounds_2 = aabb2 { .min = { -0.5f, -0.5f, }, .max = { 0.5f, 0.5f } };
    auto bounds_3 = aabb2 { .min = { 0.25f, 0.25f }, .max = { 0.75f, 0.75f } };

    auto grid_1 = grid2 { .bounds = bounds_1, .cell_count_1d = 2 };
    init(grid_1);

    auto position_1 = vec2 { -0.25f, -0.25f };
    auto position_1_cell_index = 0;

    auto render_mesh_1 = render_mesh { .id = 1 };
    add(grid_1, render_mesh_1, position_1);
    for (auto cell_index = 0; cell_index < 4; cell_index++)
    {
      auto ids = cell_render_mesh_ids(grid_1, cell_index);
      if (cell_index == position_1_cell_index)
      {
        test_equal("grid2: add (cell render mesh count)", ids.size(), std::size_t(1));
      }
      else
      {
        test_equal("grid2: add (cell render mesh count)", ids.size(), std::size_t(0));
      }
    }

    auto render_mesh_2 = render_mesh { .id = 2 };
    add(grid_1, render_mesh_2, position_1);

    remove(grid_1, render_mesh_2, position_1);
    for (auto cell_index = 0; cell_index < 4; cell_index++)
    {
      auto ids = cell_render_mesh_ids(grid_1, cell_index);
      if (cell_index == position_1_cell_index)
      {
        test_equal("grid2: remove (cell render mesh count)", ids.size(), std::size_t(1));
      }
      else
      {
        test_equal("grid2: remove (cell render mesh count)", ids.size(), std::size_t(0));
      }
    }

    auto meshes_3 = find(grid_1, [&](const aabb2& bounds)
    {
      return intersect(bounds_2, bounds) ? 0 : -1;
    });
    test_equal("grid2: find", meshes_3.size(), std::size_t(1));

    auto meshes_4 = find(grid_1, [&](const aabb2& bounds)
    {
      return intersect(bounds_3, bounds) ? 0 : -1;
    });
    test_equal("grid2: find 2", meshes_4.size(), std::size_t(0));
  }
}
