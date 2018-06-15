#include "DenmemParams.h"
#include "HAL2ESSContainer.h"

const double DenmemParams::V_factor = 0.001; //from mV to V
const double DenmemParams::t_factor = 0.001; //from ms to s
const double DenmemParams::a_factor = 1.e-9; //from nA to A
const double DenmemParams::c_factor = 1.e-9; //from nF to F
const double DenmemParams::g_factor = 1.e-9; //from nS to S

DenmemParams::DenmemParams(ESS::BioParameter const & params):
		cm(c_factor*params.cm)
		,g_l(g_factor*params.g_l)
		,v_rest(V_factor*params.v_rest)
		,v_spike(V_factor*params.v_spike)
		,v_reset(V_factor*params.v_reset)
		,tau_refrac(t_factor*params.tau_refrac)
		,e_rev{  {V_factor*params.e_rev_E,   V_factor*params.e_rev_I  }}
		,tau_syn{{t_factor*params.tau_syn_E, t_factor*params.tau_syn_I}}
		,a(g_factor*params.a)
		,delta_T(V_factor*params.delta_T)
		,v_thresh(V_factor*params.v_thresh)
		,b(a_factor*params.b)
		,tau_w(t_factor*params.tau_w)
{
}

	/// converts denmem params to bio parameters
ESS::BioParameter DenmemParams::toBioParameter() const
{
	ESS::BioParameter rv;

	rv.a           = a/g_factor;
	rv.b           = b/a_factor;
	rv.cm          = cm/c_factor;
	rv.delta_T     = delta_T/V_factor;
	rv.e_rev_E     = e_rev[0]/t_factor;
	rv.e_rev_I     = e_rev[1]/t_factor;
	rv.tau_refrac  = tau_refrac/t_factor;
	rv.tau_syn_E   = tau_syn[0]/t_factor;
	rv.tau_syn_I   = tau_syn[1]/t_factor;
	rv.tau_w       = tau_w/t_factor;
	rv.v_reset     = v_reset/V_factor;
	rv.v_rest      = v_rest/V_factor;
	rv.v_spike     = v_spike/V_factor;
	rv.v_thresh    = v_thresh/V_factor;
	rv.g_l         = g_l/g_factor;

    // calculate tau_m from g_l and cm
	rv.tau_m       = cm/g_l/t_factor;

	return rv;
}
