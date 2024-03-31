#ifndef ASTRUM_POST_PROCESSING_HDR_RESOLVE_H
#define ASTRUM_POST_PROCESSING_HDR_RESOLVE_H

#include <ludo/api.h>

namespace astrum
{
  void add_hdr_resolve(ludo::instance& inst, uint64_t vertex_shader_id, uint64_t mesh_instance_id);
}

#endif // ASTRUM_POST_PROCESSING_HDR_RESOLVE_H
