#ifndef NDNBRITEHELPER_H
#define NDNBRITEHELPER_H

#include <string>
#include <vector>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

#include "ns3/random-variable-stream.h"
#include "ns3/ptr.h"
#include "ns3/Brite.h"
#include "ns3/Graph.h"

namespace ns3
{
namespace  ndn
{

/**
 * \defgroup brite BRITE Topology Generator
 *
 * Create topologies with BRITE.
 */

/**
 * \ingroup brite
 * \brief Interface with BRITE, the Boston university Representative Internet
 *        Topology gEnerator
 *
 * This helper class creates an interface with BRITE and allows the user to
 * easily create ns-3 topologies from BRITE generated graphs. This class
 * accepts a BRITE configuration and seed file, much like the stand-alone
 * BRITE software. Using these files, BRITE generates a graph which is
 * stored herein. ns-3 examples can then grab the BRITE generated nodes and
 * edges from this helper and create ns-3 specific topologies.
 *
 */

class NDNBriteHelper
{
public:
  /**
   * Construct a NDNBriteHelper
   *
   * \param confFile a BRITE configuration file
   * \param seedFile a BRITE seed file
   * \param newseedFile a BRITE seed file with newly generated values
   */
  NDNBriteHelper (std::string confFile,
                       std::string seedFile,
                       std::string newseedFile);

  /**
   * Construct a NDNBriteHelper using NS3 to generate seed values
   * need by BRITE
   *
   */
  NDNBriteHelper (std::string confFile, std::string queueName, uint32_t queueSize);

  ~NDNBriteHelper ();

  /**
   * Assigns stream number to UniformRandomVariable used to
   * generate brite seed file
   *
   * \param streamNumber the stream number to assign
   *
   */
  void AssignStreams (int64_t streamNumber);

  /**
   *  Create NS3 topology using information generated from BRITE.
   *
   *  \param stack Internet stack to assign to nodes in topology
   */
  void BuildBriteTopology ();

  /**
   * Create NS3 topology using information generated from BRITE and configure topology for MPI use.
   *
   * \param stack Internet stack to assign to nodes in topology.
   * \param systemCount The number of MPI instances to be used in the simulation.
   *
   */
  void BuildBriteTopology (const uint32_t systemCount);

  /**
   * Returns the number of router leaf nodes for a given AS
   *
   * \param asNum the AS number
   * \returns the number of leaf nodes in the specified AS
   *
   */
  uint32_t GetNLeafNodesForAs (uint32_t asNum);

  /**
   * Returns a given router leaf node from a given AS
   *
   * \param asNum the AS number
   * \param leafNum the leaf number
   * \returns the specified node
   */
  Ptr<Node> GetLeafNodeForAs (uint32_t asNum, uint32_t leafNum);

  /**
   * Returns the total number of nodes for a given AS
   *
   * \param asNum the AS number
   * \returns the total number of nodes in the given AS
   */
  uint32_t GetNNodesForAs (uint32_t asNum);

  /**
   * Returns a given router node for a given AS
   *
   * \param asNum the AS number
   * \return the specified node
   *
   */
  Ptr<Node> GetNodeForAs (uint32_t asNum, uint32_t nodeNum);

  /**
    * Returns the number of AS created in the topology
    *
    * \returns the number of AS created in the topology
    */
  uint32_t GetNAs (void) const;

  /**
    * Returns the system number for the MPI instance that this AS is assigned to.  Will always return 0 if MPI not used
    *
    * \returns The system number that the specified AS number belongs to
    *
    * param asNum The AS Number
    */
  uint32_t GetSystemNumberForAs (uint32_t asNum) const;

  /**
    * Returns the number of nodes created within
    * the topology
    *
    * \returns the total number of nodes within the brite topology
    */
  uint32_t GetNNodesTopology () const;

  /**
    * Returns the number of edges created within
    * the topology
    *
    * \returns the total number of edges within the brite topology
    */
  uint32_t GetNEdgesTopology () const;

private:
  //brite values are unitless however all examples provided use mbps to specify rate
  //this constant value is used to convert the mbps provided by brite to bps.
  static const int mbpsToBps = 1000000;
  static const int kbpsToBps = 1000;

  /**
   * \brief Node information from BRITE
   *
   * The BRITE code generates a graph and returns
   * information on the nodes generated. This is
   * stored here in a struct.
   */
  struct BriteNodeInfo
  {
    int nodeId;
    double xCoordinate;
    double yCoordinate;
    int inDegree;
    int outDegree;
    int asId;
    std::string type;
  };

  /**
   * \brief Edge information from BRITE
   *
   * The BRITE code generates a graph and returns
   * information on the edges generated. This is
   * stored here in a struct.
   */
  struct BriteEdgeInfo
  {
    int edgeId;
    int srcId;
    int destId;
    double length;
    double delay;
    double bandwidth;
    int asFrom;
    int asTo;
    std::string type;
  };

  //stores all of the nodes used in the BRITE generated topology
  NodeContainer m_nodes;

  /**
    * \internal
    */
  void BuildBriteNodeInfoList (void);

  /**
    * \internal
    */
  void BuildBriteEdgeInfoList (void);

  /**
    * \internal
    */
  void ConstructTopology (void);

  /**
    * \internal
    */
  void GenerateBriteTopology (void);

  /// brite configuration file to use
  std::string m_confFile;

  /// brite seed file to use
  std::string m_seedFile;

  /// brite seed file to generate for next run
  std::string m_newSeedFile;

  /// stores the number of AS in the BRITE generated topology
  uint32_t m_numAs;

  /// stores the netdevices created for each AS
  std::vector<NetDeviceContainer*> m_netDevices;

  /// stores the leaf router nodes for each AS
  std::vector<NodeContainer*> m_asLeafNodes;

  /// stores all of the nodes in the brite topology by AS number
  std::vector<NodeContainer*> m_nodesByAs;

  /// stores the MPI system number each AS assigned to.  All assigned to 0 if MPI not used.
  std::vector<int> m_systemForAs;

  /// the Brite topology
  brite::Topology* m_topology;

  /// stores the number of nodes created in the BRITE topology
  uint32_t m_numNodes;

  /// stores the number of edges created in the BRITE topology
  uint32_t m_numEdges;

  // name of queue implementation
  std::string m_queueName;

  uint32_t m_queueSize;

  /**
   * The BRITE code generates multiple nodes and edges. Each
   * node and edge is stored in a BriteNodeInfo or BriteEdgeInfo
   * struct, and each instance is stored in a vector.
   * @{
   */
  typedef std::vector<BriteNodeInfo> BriteNodeInfoList;
  typedef std::vector<BriteEdgeInfo> BriteEdgeInfoList;

  BriteNodeInfoList m_briteNodeInfoList;
  BriteEdgeInfoList m_briteEdgeInfoList;

  /// used to create the links within the topology
  PointToPointHelper m_britePointToPointHelper;

  /// random variable stream for brite seed file
  Ptr<UniformRandomVariable> m_uv;
};

} // ndn
} // namespace ns3

#endif
