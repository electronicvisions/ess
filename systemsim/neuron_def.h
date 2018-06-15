#ifndef _NEURON_DEF_H_
#define _NEURON_DEF_H_
// biological Parameter
#define B_TAU_MEM 20 // in ms
#define B_TAU_SYN 5 // in ms
#define B_E_REV_EXC 0.0 // in mV
#define B_E_REV_INH -70.0 // in mV
#define B_V_REST -65.0 // in mV
#define B_V_RESET -65.0 // in mV
#define B_V_INIT -65.0 // in mV
#define B_V_THRESH -50.0 // in mV	
#define B_C_M 1.0 // in nF	

// normalized Parameters for IFSC
#define TAU_MEM 1.0 // 20 ms
#define V_REST 0.0  //
#define V_THRESH 1.0 
#define TAU_SYN (TAU_MEM*B_TAU_SYN/B_TAU_MEM)
#define E_PLUS ((B_E_REV_EXC - B_V_REST)/(B_V_THRESH - B_V_REST))
#define E_MINUS ((B_E_REV_INH - B_V_REST)/(B_V_THRESH - B_V_REST))
#define V_INIT 0.0
#define E_S_INIT (E_PLUS + E_MINUS)
#define TAU_UNIT 0.02

// random_gen Parameters:
#define RAND_TAU 1
#define RAND_UNIT SC_MS
#define RAND_FREQ_IN_HZ 6
#endif //_NEURON_DEF_H_
