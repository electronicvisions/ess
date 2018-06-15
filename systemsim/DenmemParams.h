#pragma once
#include <array>

// pre declaration
namespace ESS {
class BioParameter;
}//end namespace ess

/// Analog Parameters of a Denmem
/// parameter names according to PyNN.EIF_multicond_exp_isfa_ista
/// but with g_l instead of tau_m
/// All parameters in SI units.
struct DenmemParams {
	double cm;
	double g_l;
	double v_rest;
	double v_spike;
	double v_reset;
	double tau_refrac;
	std::array<double,2> e_rev;
	std::array<double,2> tau_syn;
	double a;
	double delta_T;
	double v_thresh;
	double b;
	double tau_w;

	DenmemParams () :
		cm(2.e-9)
		,g_l(30.e-9)
		,v_rest(-0.065)
		,v_spike(-0.05)
		,v_reset(-0.065)
		,tau_refrac(0.001)
		,e_rev{{0.,-0.1}}
		,tau_syn{{0.005, 0.005}}
		,a(4.0e-9)
		,delta_T(0.002)
		,v_thresh(-0.05)
		,b(0.0805e-9)
		,tau_w(0.144)
	{}

	DenmemParams(ESS::BioParameter const & params);

	/// converts denmem params to bio parameters
	ESS::BioParameter toBioParameter() const;

	//constants for parameter scaling
	static const double V_factor; // from mV to V = 0.001;
	static const double t_factor; // from ms to s = 0.001;
	static const double a_factor; // from nA to A = 1.e-9;
	static const double c_factor; // from nF to F = 1.e-9;
	static const double g_factor; // from nS to S = 1.e-9;

};
