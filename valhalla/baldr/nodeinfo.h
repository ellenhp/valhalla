#ifndef VALHALLA_BALDR_NODEINFO_H_
#define VALHALLA_BALDR_NODEINFO_H_

#include <valhalla/midgard/pointll.h>
#include <valhalla/midgard/util.h>
#include <valhalla/baldr/graphid.h>
#include <valhalla/baldr/graphconstants.h>

using namespace valhalla::midgard;

namespace valhalla {
namespace baldr {

constexpr uint32_t kMaxTileEdgeCount = 4194303;   // 2^22 directed edges
constexpr uint32_t kMaxEdgesPerNode  = 127;       // Maximum edges per node

/**
 * Information held for each node within the graph. The graph uses a forward
 * star structure: nodes point to the first outbound directed edge and each
 * directed edge points to the other end node of the edge.
 */
class NodeInfo {
 public:
  /**
   * Constructor
   */
  NodeInfo();

  /**
   * Destructor.
   */
  ~NodeInfo();

  /**
   * Get the latitude, longitude of the node.
   * @return  Returns the latitude and longitude of the node.
   */
  const PointLL& latlng() const;

  /**
   * Get the index of the first outbound edge from this node. Since
   * all outbound edges are in the same tile/level as the node we
   * only need an index within the tile.
   * @return  Returns the GraphId of the first outbound edge.
   */
  uint32_t edge_index() const;

  /**
   * Get the number of outbound directed edges.
   * @return  Returns the number of outbound directed edges.
   */
  uint32_t edge_count() const;

  /**
   * Get the best road class of the outbound directed edges.
   * @return   Returns road class.
   */
  RoadClass bestrc() const;

  /**
   * Get the computed version of NodeInfo attributes.
   * @return   Returns internal version.
   */
  static const uint64_t internal_version();

 protected:
  // Latitude, longitude position of the node.
  std::pair<float, float> latlng_;

  // Node attributes.
  struct NodeAttributes {
    uint32_t edge_index_  : 22; // Index within the node's tile of its first
                                // outbound directed edge
    uint32_t edge_count_   : 7; // Number of outbound edges
    uint32_t bestrc_       : 3; // Best directed edge road class
  };
  NodeAttributes attributes_;

  // Node access (see graphconstants.h)
  Access access_;

  // Intersection type (TODO)
  uint8_t intersection_;

  // Administrative information
  struct NodeAdmin {
    uint16_t admin_index  : 6; // Index into this tile's list of admin data
    uint16_t timezone     : 6; // Time zone
    uint16_t dst          : 1; // Is Daylight Saving Time used?
    uint16_t spare        : 3;
  };
  NodeAdmin nodeadmin_;

  // Node type
  // TODO - number of edges on local level?
  // TODO - can we define street intersection types for use in
  // transition costing?
  struct NodeType {
    uint32_t density      : 4; // Density (population? edges?)
    uint32_t type         : 4; // Node type
    uint32_t false_       : 1; // Node connects to only 2 edges (but
                               // attributes of the edges change). TODO - new name!
    uint32_t end          : 1; // End node (only connects to 1 edge)
    uint32_t parent       : 1; // Is this a parent node
    uint32_t child        : 1; // Is this a child node
    uint32_t mode_change  : 1; // Mode change allowed?
    uint32_t spare        : 19;
  };
  NodeType nodetype_;

  // Transit stop Id
  uint32_t stop_id;
};

}
}

#endif  // VALHALLA_BALDR_NODEINFO_H_
