//_____________________________________________
// Company      :	UNI Heidelberg	
// Author       :	Bernhard Vogginger
// E-Mail   	:	bernhard.vogginger@kip.uni-heidelberg.de
//_____________________________________________

#include "ADEX.h"
#include <math.h>
#include <iostream>	//for debugging
#include <sstream>
#include <assert.h>
#include <vector>
#include <algorithm>

#include <log4cxx/logger.h>
static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("ESS.HICANN.Neuron");

const double ADEX::rel_float_tolerance = 1.e-6;

ADEX::ADEX(double _V, double _w):
	C(281.e-12)
	,g_l(30.e-9)
	,E_l(-70.6e-3)
	,E_reset(-70.6e-3)
	,E_e(0.0)
	,E_i(-0.08)
	,tau_syn_e(0.005)
	,tau_syn_i(0.005)
	,V_t(-50.4e-3)
	,D_t(2e-3)
	,tau_w(0.144)
	,a(4e-9)
	,b(0.0805e-9)
	,V_spike(0.0)
	,denmemId(-1)
	,V(_V) // all state variables are intialized to _V, _w or 0 for conductances
	,w(_w)
	,g_e(0.)
	,g_i(0.)
    ,I(0.)
    ,V_last(_V) // the same for the last states of the changable variables.
	,w_last(_w)
	,t_last(0)
	,t_sim(0)
	,g_e_last(0.)
	,g_i_last(0.)
	,dt(0.00001) // 0.01 ms
	,total_run_time(0)
	,extra_time(0)
	,rec_voltage(false)
{
	g_e_exp_fac=exp(-dt/tau_syn_e);
	g_i_exp_fac=exp(-dt/tau_syn_i);

	// see init() for details about this formulaes
	assert(V_spike > E_reset); // threshold must be higher than reset
	double voltages[6] = {V_spike, E_l, E_i, E_e, V_t, E_reset};
	double min_voltage = *std::min_element(voltages, voltages+6);
	max_dvdt = 2*(V_spike - min_voltage)/dt; // is always > 0
	min_dvdt = (V_spike - E_reset)/(100*C/g_l); // is always > 0
	max_w_influence_on_dvdt = -min_dvdt*C;
	I_min = g_l * (fmin(V_spike,V_t) - E_l);
    t_comp_epsilon = dt*rel_float_tolerance;
}

ADEX::ADEX(){}

void ADEX::set_V(double _V){
	V=_V;
	V_last = _V;
}

void ADEX::set_w(double _w){
	w=_w;
	w_last = _w;
}

void ADEX::add_exc_w(double weight)
{
	g_e += weight;
	g_e_last = g_e;
}
void ADEX::add_inh_w(double weight)
{
	g_i += weight;
	g_i_last = g_i;
}
double ADEX::time_to_next_spike()
{
    LOG4CXX_TRACE(logger, "ADEX::time_to_next_spike() called" );
	// set t_sim back to zero and set g_*_last to current values.
	t_sim=0.;
	g_e_last=g_e;
	g_i_last=g_i;

	double dvdt;
	double dwdt;
	HEUN_method(dvdt,dwdt);

	// Integrate as long as V is increasing and has not yet reached spike threshold
	// comment: V has to be bigger than than the inhibitory reversal potential
	// otherwise this loop can get infinite:
	// If V < E_i and a inhibitory spike is received, the membrane potential keeps monoton increasing and so never stop
	// I > I_min to account for broad after spike potential: Voltage decreases first, but then increases and the neuron spikes
    while( ( (I > I_min) || (dvdt > min_dvdt) || (w < max_w_influence_on_dvdt) ) && (V<V_spike)){
		advance_one_step(dvdt,dwdt);
		HEUN_method(dvdt,dwdt);
        //LOG4CXX_TRACE(logger, "ADEX::time_to_next_spike() loooooooping: V = " << V << " w = " << w );
	}

	// then if spike has reached threshold: return timing of next spike
	if(V>=V_spike){
		return t_sim;
	}
	// there is no spike
	else{
		return -1;
	}

}

double ADEX::dV_dt(double _V, double _w, double _g_e, double _g_i) const
{
	//double tmp = (-g_l*(_V-E_l) + g_l*D_t*exp((_V-V_t)/D_t) - g_e*g_e_exp_fac*(_V-E_e) - g_i*g_i_exp_fac*(_V-E_i) - _w)/C;
	double tmp = (-g_l*(_V-E_l) + g_l*D_t*exp((_V-V_t)/D_t) - _g_e*(_V-E_e) - _g_i*(_V-E_i) - _w + I)/C;
	if(tmp > max_dvdt)
		tmp = max_dvdt;
	return tmp;
}

double ADEX::dw_dt(double _V, double _w) const
{
	// this check makes sure, that the returned value is not too big
	// otherwise there could be a "w" 1000 times to big -> that would destroy the correctness of further computation
	if(_V<V_spike)
		return (a*(_V-E_l) - _w)/tau_w;
	else
		return (a*(V_spike-E_l) - _w)/tau_w;
}

void ADEX::run(double time_to_run, bool is_refractory)
{
	// if voltage is recorded, set all variables back to last secure timestamp(t_sim = 0)
	// that makes sure that voltage is recorded for every timestep 
	if(rec_voltage){
		// do set all variables back to t_sim=0
		// and do simulation again up to time "time_to_run"
		extra_time+=t_sim;
		reset_to_last_secure_state();
	}
	// if time_to_run comes before the time, up to which we already simulated, we have to go back in time.
	else if(time_to_run < t_sim){
		// do set all variables back to t_sim=0
		// and do simulation again up to time "time_to_run"
		extra_time+=(t_sim-time_to_run);
		reset_to_last_secure_state();
	}

	//while(t_sim < time_to_run){
	// we don not comapare (t_sim < time_to_run) anymore, as it might happen,
	// although that both are virtually equal, they are not equal because of machine errors that add up.
	// Hence we compare for something greate than zero, here 1.e-6*dt.
	// but still big enough to filter out the machine error
	if(rec_voltage) //FIXME is this if correct here ???
    LOG4CXX_TRACE(logger, "ADEX::run called with time_to_run = " << time_to_run << " , t_sim =" << t_sim );
	while( (time_to_run - t_sim) > t_comp_epsilon )
	{
        //LOG4CXX_TRACE(logger, "ADEX::run looooooooooping: t_sim = " << t_sim << " time_to_run = " << time_to_run );
		//integrate
		double dvdt;
		double dwdt;
		HEUN_method(dvdt,dwdt, is_refractory);
		advance_one_step(dvdt,dwdt);
		if (rec_voltage)
		{
			voltage_file << t_sim + total_run_time << "\t" << V << "\t" << w << "\t" << g_e << "\t" << g_i << "\n";
		}
	}
	if (rec_voltage)
	{
		voltage_file.flush();
	}

	total_run_time+=time_to_run;
	set_secure_state();
}

void ADEX::OutgoingSpike()
{
	run(t_sim);
	// do reset and spike-triggered adaption
	V=E_reset;
	w+=b;
	set_secure_state();
}

void ADEX::set_current_offset(double const current)
{
    I = current;
}

void ADEX::init(double delta_t, bool rec, std::string fn, int id){
	assert(delta_t > 0);
	rec_voltage = rec;
	denmemId = id;
	dt=delta_t;
	g_e_exp_fac=exp(-dt/tau_syn_e);
	g_i_exp_fac=exp(-dt/tau_syn_i);

    assert (g_e_exp_fac < 1.);
	assert (g_i_exp_fac < 1.);
	assert (g_e_exp_fac >= 0.);
	assert (g_i_exp_fac >= 0.);

	// set initial variables.
	set_V(E_l);
	set_w(0.);

	if(rec_voltage)
	{
		std::stringstream stream;
		stream << fn << std::flush;
		voltage_file.open(stream.str().c_str() );
		voltage_file.setf(std::ios::scientific,std::ios::floatfield); // scientific values for floating point number, default precision of 6 is ok.
		// print values at time 0
		voltage_file << "#t[Seconds]" << "\t" << "V[Volt]" << "\t" << "w[Ampere]" << "\t" << "g_e[Siemens]" << "\t" << "g_i[Siemens]" << "\n";
		voltage_file << t_sim << "\t" << V << "\t" << w << "\t" << g_e << "\t" << g_i << "\n";
	}

	assert(V_spike >= E_reset); // threshold must be higher than reset

	// During one integration step, v cannot increase mor than max_dvdt.
	// avoids infinite values of v.
	// calculated such that threshold is reached for sure, even if increment is smaller than it would be from the mathematics.
	double voltages[6] = {V_spike, E_l, E_i, E_e, V_t, E_reset};
	double min_voltage = *std::min_element(voltages, voltages+6);
	
	max_dvdt = 2*(V_spike - min_voltage)/dt; // is always > 0

	// minimum increase of v during integration, to continue integration.
	// (needs to be independent from chosen dt. so we take the membrane time constant as reference)
	// formulae:
	//            V_spike - E_reset
	// min_dvdt = ------------
	//            100 * tau_m
	// In words: If the current voltage change rate is not high enough to reach the spike threshold within 100 times the membrane
	// time constant, the neuron won't fire!
	min_dvdt = (V_spike - E_reset)/(100*C/g_l); // is always > 0

	// see doc for details on this.
	max_w_influence_on_dvdt = -min_dvdt*C;
	I_min = g_l * (fmin(V_spike,V_t) - E_l);
	t_comp_epsilon = dt*rel_float_tolerance;
}

ADEX::~ADEX(){
	if(rec_voltage){
		voltage_file.close();
	}
}
void ADEX::HEUN_method(double & dvdt, double & dwdt, bool clamp_V) const
{
	if (!clamp_V){
		double dvdt_0 = dV_dt(V,w,g_e, g_i);
		double dwdt_0 = dw_dt(V,w);
		double V_1 = V + dvdt_0*dt;
		double w_1 = w + dwdt_0*dt;
		double dvdt_1 = dV_dt(V_1,w_1, g_e*g_e_exp_fac,g_i*g_i_exp_fac ); // in the 2nd step, we also have to advance g_e and g_i
		double dwdt_1 = dw_dt(V_1,w_1);
		dvdt = .5*(dvdt_0+dvdt_1);
		dwdt = .5*(dwdt_0+dwdt_1);
	}
	else {
		// if we are in refractory period, we only update w and clamp V, i.e. use the same V for the 2nd step
		double dwdt_0 = dw_dt(V,w);
		double w_1 = w + dwdt_0*dt;
		double dwdt_1 = dw_dt(V,w_1);
		dvdt = 0.;
		dwdt = .5*(dwdt_0+dwdt_1);
	}
}

void ADEX::advance_one_step( double dvdt, double dwdt ) {
	w+=dwdt*dt;
	V+=dvdt*dt;
	g_e*=g_e_exp_fac;
	g_i*=g_i_exp_fac;
	t_sim+=dt;
}

void ADEX::reset_to_last_secure_state() {
	V=V_last;
	w=w_last;
	g_e=g_e_last;
	g_i=g_i_last;
	t_sim=0.;
}
void ADEX::set_secure_state() {
	V_last=V;
	w_last=w;
	g_e_last = g_e;
	g_i_last = g_i;
    t_sim = 0.;
}

