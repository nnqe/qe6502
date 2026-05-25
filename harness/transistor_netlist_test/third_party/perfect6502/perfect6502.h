#ifndef INCLUDED_FROM_NETLIST_SIM_C
#define state_t void
#endif

typedef struct perfect6502_snapshot perfect6502_snapshot_t;

extern state_t *initAndResetChip(void);
extern void destroyChip(state_t *state);
extern void step(state_t *state);
extern void chipStatus(state_t *state);
extern unsigned short readPC(state_t *state);
extern unsigned char readA(state_t *state);
extern unsigned char readX(state_t *state);
extern unsigned char readY(state_t *state);
extern unsigned char readSP(state_t *state);
extern unsigned char readP(state_t *state);
extern unsigned int readRW(state_t *state);
extern unsigned short readAddressBus(state_t *state);
extern void writeDataBus(state_t *state, unsigned char);
extern unsigned char readDataBus(state_t *state);
extern unsigned char readIR(state_t *state);

extern perfect6502_snapshot_t *perfect6502_snapshot_create(state_t *state);
extern int perfect6502_snapshot_restore(state_t *state, const perfect6502_snapshot_t *snapshot);
extern void perfect6502_snapshot_destroy(perfect6502_snapshot_t *snapshot);

extern unsigned char memory[65536];
extern unsigned long cycle;
//extern unsigned int transistors;
