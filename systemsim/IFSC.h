// *****************************************************************
// Exact simulation of integrate-and-fire models with
// exponential synaptic conductances
//
// Romain Brette
// brette@di.ens.fr
// Last updated: Jul 2005
// *****************************************************************
//
// Insert #define IFSC_PREC 1e-4 to use precalculated tables
// otherwise the original expressions are used
//
// All variables are normalized, i.e. V0=0, Vt=1, tau = 1

#ifndef __IFSC_H
#define __IFSC_H

extern double _taus;

// Initialize (create the tables) and
//   set the constants:
//     taus = synaptic time constants (in units of the membrane time constant)
void IFSC_Init(double taus);

// Delete the tables
void IFSC_Done();

// ****************************************
// The three functions for exact simulation
// ****************************************

// 1) Update the variables V, g and Es
//      after an incoming spike at time t
//      (relative to the time of the last update tl)
//      (w is the signed weight of the synapse)
//     Ep, Em = synaptic reversal potentials
//     (with normalized potential s.t. V0 = 0, Vt = 1)
void IFSC_IncomingSpike(double t, double w, double *V, double *g, double *Es, double Ep, double Em);

// 2) Returns the updated value of variable g
//      after an outgoing spike at time t
//      (relative to the time of the last update tl)
//    The membrane potential V is not reset here,
//      therefore one must add the line V=Vr after calling
//      this function
double IFSC_OutgoingSpike(double t, double g);

// 3) Calculate the time of next spike
double IFSC_SpikeTiming(double V, double g, double Es);

// The spike test (mostly for internal use)
//   positive => there is a spike
//   zero     => there is no spike
int IFSC_SpikeTest(double V, double g, double Es);

#endif
