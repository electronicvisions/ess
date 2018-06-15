// ******************************************************************************
// Exact simulation of integrate-and-fire models with
// exponential synaptic conductances
//
// Romain Brette
// brette@di.ens.fr
// Last updated: Feb 2007 - I thank Michiel D'Haene for pointing a couple of bugs
// ******************************************************************************
//
// Insert #define IFSC_PREC 1e-4 in file IFSC.h to use precalculated tables
// otherwise the original expressions are used
//
// All variables are normalized, i.e. V0=0, Vt=1, tau = 1
//
// N.B.: the code is not optimized

#include "IFSC.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

// ************************************************************
// CONSTANTS
// ************************************************************

// The neuron spikes when the membrane potential
// is within SPIKE_COMPUTATION_ERROR of the threshold
#ifdef IFSC_PREC
#define SPIKE_COMPUTATION_ERROR	1e-10
#else
#define SPIKE_COMPUTATION_ERROR	1e-6
#endif

// Relative error in calculating the incomplete Gamma integral
#define MAX_GAMMA_ERROR 1e-15

// Time constant of the synaptic conductances and its inverse
// (_taus is in units of the membrane time constant)
double _taus;
double inv_taus;

double rho(double g);

#ifdef IFSC_PREC
// ************************************************************
// LOOK-UP TABLES
// ************************************************************
// 
// The values are computed with the look-up tables 
// using linear interpolation as follows:
//
// cell number:     n=(int)(x/dx)    (dx is the precision of the table)
// remainder:       h=(x/dx)-n       (for the linear interpolation)
// value:           y=table[n]+h*(table[n+1]-table[n])
//
// ----------------------------------------------------------
//    Look-up table for the exponential (exp(-x))
// ----------------------------------------------------------
//
// Address
double *expLookupTable;
// Precision
double expLookup_dx;
// Inverse of precision
double inv_expLookup_dx;
// Size
int expLookup_nmax;
// 1./(_taus*expLookup_dx)
double inv_taus_X_inv_expLookup_dx;

// --------------------------------------------------
//    Look-up table for the (modified) rho function
//       rho(1-\tau_s,\tau_s g) * g * \tau_s
// --------------------------------------------------
// Address
double *rhoLookupTable;
// Precision
double rhoLookup_dg;
// Inverse of precision
double inv_rhoLookup_dg;
// Size
int rhoLookup_nmax;

// -----------------------------------------------
// Functions for the exponential look-up table
// -----------------------------------------------
//
// Build the look-up table for x in [0,xmax] with precision dx
void makeExpLookupTable(double dx,double xmax) {
	double x;
	int n;

	expLookup_dx=dx;
	inv_expLookup_dx=1./dx;
	inv_taus_X_inv_expLookup_dx=inv_taus*inv_expLookup_dx;
	expLookup_nmax=(int)(xmax*inv_expLookup_dx);

	expLookupTable=(double *)
		malloc(sizeof(double)*(expLookup_nmax));

	x=0.;
	for(n=0;n<expLookup_nmax;n++) {
		expLookupTable[n]=exp(-x);
		x+=dx;
	}

	expLookup_nmax--;
}

// Free the memory allocated for the table
void freeExpLookupTable() {
	free((void *)expLookupTable);
}

// tableExpM(x) returns exp(-x) from the look-up table
// (with linear interpolation)
double tableExpM(double x) {
	double a,b;
	double *table;
	int n=(int)(x=x*inv_expLookup_dx);

	if ((n>=expLookup_nmax) || (n<0)) {
		return (exp(-x*expLookup_dx));
	}

	table=expLookupTable+n;
	a=*(table++);
	b=*table;

	return ( a + (x-n)*(b - a) );
}

// -----------------------------------------------
// Functions for the rho look-up table
// -----------------------------------------------
//
// Build the look-up table for g in [0,gmax] with precision dg
void makeRhoLookupTable(double dg,double gmax) {
	double g;
	int n;

	rhoLookup_dg=dg;
	inv_rhoLookup_dg=1./dg;
	rhoLookup_nmax=(int)(gmax/dg);

	rhoLookupTable=(double *)
		malloc(sizeof(double)*(rhoLookup_nmax));

	g=0.;
	for(n=0;n<rhoLookup_nmax;n++) {
		rhoLookupTable[n]=rho(g)*g*_taus;
		g+=dg;
	}

	rhoLookup_nmax--;
}

// Free the memory allocated for the table
void freeRhoLookupTable() {
	free((void *)rhoLookupTable);
}

// tableRho(g) returns rho(g) from the look-up table
// (with linear interpolation)
double tableRho(double g) {
	double a,b;
	double *table;
	int n=(int)(g=g*inv_rhoLookup_dg);

	if ((n>=rhoLookup_nmax) || (n<0)) {
		return (rho(g*expLookup_dg));
	}

	table=rhoLookupTable+n;
	a=*(table++);
	b=*table;

	return ( a + (g-n)*(b - a) );
}

#endif

// **************************
// CONSTRUCTION & DESTRUCTION
// **************************

// Initialize (create the tables) and
//   set the constants:
//     taus = synaptic time constants (in units of the membrane time constant)
void IFSC_Init(double taus){
	_taus=taus;
	inv_taus=1./_taus;

	#ifdef IFSC_PREC
	makeExpLookupTable(IFSC_PREC,1.);
	makeRhoLookupTable(IFSC_PREC,1.);
	#endif
}

// Delete the tables
void IFSC_Done() {
	#ifdef IFSC_PREC
	freeRhoLookupTable();
	freeExpLookupTable();
	#endif
}

// ************************************************************
// RHO FUNCTION (based on incomplete gamma integral - see text)
// ************************************************************
// rho(g) = \rho(1-\tau_s,\tau_s * g)
// (see text)
//
// We use the power series expansion of the incomplete gamma integral
double rho(double g) {
    double sum, del, ap;
	double x=_taus*g;

	// Note: all numbers are always positive
	ap = 1.-_taus;
    del = sum = 1.0 / (1.-_taus);
	do {
		++ap;
        del *= x / ap;
        sum += del;
    } while (del >= sum * MAX_GAMMA_ERROR);

	return sum;
}

// ****************************************
// The three functions for exact simulation
// ****************************************

// 1) Update the variables V, g and Es
//      after an incoming spike at time t
//      (relative to the time of the last update tl)
void IFSC_IncomingSpike(double t,double w, double *V, double *g, double *Es, double Ep, double Em) {
	double loc_Es = *Es;
	double gt=IFSC_OutgoingSpike(t,*g);

	#ifdef IFSC_PREC
	*V=-_taus*loc_Es*gt*rho(gt)+
		tableExpM(t-_taus*(gt-*g))*(*V+_taus*loc_Es**g*tableRho(*g));
	#else
	*V=-_taus*loc_Es*gt*rho(gt)+
		exp(-t+_taus*(gt-*g))*(*V+_taus*loc_Es**g*rho(*g));
	#endif

	if (w>0)
		*Es=(gt*(loc_Es)+w*Ep)/(gt+w);
	else
		*Es=(gt*(loc_Es)-w*Em)/(gt-w);
	*g=gt+fabs(w);
}

// 2) Returns the updated value of variable g
//      after an outgoing spike at time t
//      (relative to the time of the last update tl)
//    The membrane potential V is not reset here,
//      therefore one must add the line V=Vr after calling
//      this function
double IFSC_OutgoingSpike(double t, double g) {
	#ifdef IFSC_PREC
	return(g*tableExpM(t*inv_taus));
	#else
	return(g*exp(-t*inv_taus));
	#endif
}

// 3) Calculate the time of next spike
double IFSC_SpikeTiming(double V, double g, double Es) {
	if (IFSC_SpikeTest(V,g,Es))
	{
		// Newton-Raphson method
		double T=0.;
		double loc_V = V;
		double loc_g = g;
		double loc_Es = Es;

		while(1.-loc_V>SPIKE_COMPUTATION_ERROR) {
			T+=(1.-loc_V)/(-loc_V+loc_g*(loc_Es-loc_V));
			loc_V = V;
			loc_g = g;
			loc_Es = Es;
			IFSC_IncomingSpike(T,0., &loc_V, &loc_g, &loc_Es, 0., 0.);
		}

		return T;
	} else {
		return HUGE_VAL;
	}
}

// The spike test (mostly for internal use)
//   positive => there is a spike
//   zero     => there is no spike
int IFSC_SpikeTest(double V, double g, double Es) {
	double gstar;

	// Quick test
	if (Es<=1.)
		return 0;
	
	gstar=1./(Es-1.);
	if (g<=gstar)
		return 0;

	// Full test
	//     Calculate V(gstar)
	//		(N.B.: we should use a table for the log as well)
	IFSC_IncomingSpike(-_taus*log(gstar/g),0., &V, &g, &Es, 0., 0.);
	if (V<=1.)
		return 0;
	else
		return 1;
}
