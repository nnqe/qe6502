#ifndef INCLUDED_FROM_NETLIST_SIM_C
#define state_t void
#endif

typedef struct netlist_sim_snapshot netlist_sim_snapshot_t;

state_t *setupNodesAndTransistors(netlist_transdefs *transdefs, BOOL *node_is_pullup, nodenum_t nodes, nodenum_t transistors, nodenum_t vss, nodenum_t vcc);
void destroyNodesAndTransistors(state_t *state);
void setNode(state_t *state, nodenum_t nn, BOOL s);
BOOL isNodeHigh(state_t *state, nodenum_t nn);
unsigned int readNodes(state_t *state, int count, nodenum_t *nodelist);
void writeNodes(state_t *state, int count, nodenum_t *nodelist, int v);

void recalcNodeList(state_t *state);
void stabilizeChip(state_t *state);

netlist_sim_snapshot_t *createNetlistSnapshot(state_t *state);
int restoreNetlistSnapshot(state_t *state, const netlist_sim_snapshot_t *snapshot);
void destroyNetlistSnapshot(netlist_sim_snapshot_t *snapshot);
