#include "hw_neuron_ADEX.h"
#include "ADEX.h"
#include "sim_def.h"
#include <log4cxx/logger.h>
#include <limits>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("ESS.HICANN.Neuron");

/** Constructor
 * param log_neuron: logical_neuron
 * param adr: 6-bit adr
 * param wta: ID of WTA this Neuron is connected to
 * param a: pointer to anncore for outgoing spikes
 */
hw_neuron_ADEX::hw_neuron_ADEX(sc_module_name name, unsigned int log_neuron, unsigned int adr, unsigned int wta, sc_port<anncore_pulse_if> *a):
	hw_neuron(name, log_neuron, adr, wta, a)
	, t(0.)
	, tl(0.)
	, refrac(false)
{
    LOG4CXX_DEBUG(logger, "HardwareNeuron " << logical_neuron << " built!" );
	myADEX = new ADEX(-70.6e-3,0.);
}

hw_neuron_ADEX::~hw_neuron_ADEX(){
	if ( !sc_end_of_simulation_invoked() ) {
		// If recording, run recording up to the end of simulation
		if(rec_voltage)
		{
			t = sc_time_stamp().to_seconds();
			double dt = t-tl;
			myADEX->run(dt, refrac);
		}

		delete myADEX;
	}
    LOG4CXX_DEBUG(logger, "hw_neuron_ADEX::destructed" );
}

void hw_neuron_ADEX::spike_out()	
{
	myADEX->OutgoingSpike();
	refrac = true;
	t = sc_time_stamp().to_seconds();
	
	tl=t;
    LOG4CXX_TRACE(logger, name() << ": spike_out(ADEX): t = " << sc_time_stamp() <<  "\tnext spike : NOW" );
	if (!rel_spike_lock ) {
		rel_spike.notify(L1_DELAY_REP_TO_DENMEM, SC_NS);
		rel_spike_lock = true;
        LOG4CXX_TRACE(logger, "spike_out(ADEX): rel_spike notify" );
	}
	else {
        LOG4CXX_TRACE(logger, "spike_out(ADEX): rel_spike lock" );
		sc_time absolute_rel_time = sc_time_stamp() + sc_time(L1_DELAY_REP_TO_DENMEM, SC_NS);
		release_spike_buffer.push(absolute_rel_time);
	}
	end_of_refrac.notify(tau_refrac, SC_NS);
}

void hw_neuron_ADEX::release_spike()	
{
    LOG4CXX_TRACE(logger, name() << ": release_spike():" << sc_time_stamp() <<"\t Neuron_ID : " << logical_neuron <<  " with addr " << addr );
	(*anc)->handle_spike(logical_neuron, addr, wta);
	if ( release_spike_buffer.empty()) {
		rel_spike_lock = false;
	} else {
		sc_time delta_rel_time = release_spike_buffer.front() - sc_time_stamp();
		release_spike_buffer.pop();
		rel_spike.notify( delta_rel_time );
	}
}

void hw_neuron_ADEX::end_of_refrac_period()
{
    LOG4CXX_TRACE(logger, name() << ": end_of_refrac_period():" << sc_time_stamp());
	t = sc_time_stamp().to_seconds();
	double dt =t-tl;
	myADEX->run(dt,/* is_refractory */ refrac);
	tl=t;
	refrac=false;
	spk_time_up.notify(SC_ZERO_TIME);
}

void hw_neuron_ADEX::spike_in(double weight, char syn_type)
{
    LOG4CXX_TRACE(logger, name() << ".spike_in(" << weight << ") type=" << (unsigned int) syn_type << "(exc=0, inh=1) at: "<< sc_time_stamp() << " in Refractory Time: " << refrac );
	if(refrac==false){
        LOG4CXX_TRACE(logger, name() << " not refractory");
        t = sc_time_stamp().to_seconds();
		double dt = t-tl;

		myADEX->run(dt, refrac);
		if(syn_type==0){
			myADEX->add_exc_w(weight);
		}
		else if(syn_type==1){
			myADEX->add_inh_w(weight);
		}
		else {
            LOG4CXX_WARN(logger, "hw_neuron_ADEX::spike_in: wrong synapse type = " << (unsigned int) syn_type );
		}
		// get time of next spike
		spike_time = myADEX->time_to_next_spike();
		// if spike_time is > 0 -> there will be a next spike
		if(spike_time>=0){
            LOG4CXX_TRACE(logger, name() << " there will be a spike @ " << spike_time);
			next_spike.cancel();
			next_spike.notify(spike_time, SC_SEC);
		}
		else {
            LOG4CXX_TRACE(logger, name() << " no spike triggered");
			next_spike.cancel();
		}
		
		tl = t;
	}
	else
	{
		// during refractory period:
		// if there is a spike in:
		// just update g_e and g_i and not calculate timing of next spike
		// dont call myADEX->time_to_next_spike
		t = sc_time_stamp().to_seconds();
		double dt = t-tl;

		myADEX->run(dt,/* is_refractory */ refrac);
		if(syn_type==0){
			myADEX->add_exc_w(weight);
		}
		else if(syn_type==1){
			myADEX->add_inh_w(weight);
		}
		else {
            LOG4CXX_WARN(logger, "hw_neuron_ADEX::spike_in: wrong synapse type = " << (unsigned int) syn_type );
		}
		tl = t;
	}
}

void hw_neuron_ADEX::update_current(double const current)
{
	if(refrac==false)
    {
        LOG4CXX_TRACE(logger, name() <<"::update_current current offset set to " << current);
		t = sc_time_stamp().to_seconds();
		double dt = t-tl;

		myADEX->run(dt, refrac);
        myADEX->set_current_offset(current);
		
        // get time of next spike
		spike_time = myADEX->time_to_next_spike();
		// if spike_time is > 0 -> there will be a next spike
		if(spike_time>=0)
        {
            LOG4CXX_TRACE(logger, name() <<"::update_current next spike @ " << spike_time);
			next_spike.cancel();
			next_spike.notify(spike_time, SC_SEC);
		}
		else 
        {
            LOG4CXX_TRACE(logger, name() <<"::update_current bo next spike");
			next_spike.cancel();
		}
		
		tl = t;
	}
    else
    {
        LOG4CXX_TRACE(logger, name() <<"::update_current current offset set to " << current << ", while neuron is refractory");
		t = sc_time_stamp().to_seconds();
		double dt = t-tl;

		myADEX->run(dt,/* is_refractory */ refrac);
        myADEX->set_current_offset(current);
		
        tl = t;
	}
}

void hw_neuron_ADEX::update_spike_timing()
{
	// Get Time of next spike(out) of this neuron and add event into queue(by next_spike.notify(spike_time; SC_TIME_STEP);
	spike_time = myADEX->time_to_next_spike();
	// if spike_time is > 0 -> there will be a next spike
	if(spike_time>=0){
		next_spike.cancel();
		next_spike.notify(spike_time, SC_SEC);
	}
	else {
		next_spike.cancel();
	}
	//log(Logger::DEBUG2) << "update_spike_timing(ADEX): t = " << sc_time_stamp() <<"\t Neuron_ID : " << logical_neuron <<  "\tnext spike : "<< spike_time << endl;
}

void hw_neuron_ADEX::init(ESS::BioParameter _neuron_parameter, unsigned int _addr, bool rec, std::string fn, double dt)
{
    this->addr  =_addr;
	this->neuron_parameter = _neuron_parameter;
	this->rec_voltage = rec;

	std::ostringstream oss;
	oss << fn << "_" << _addr;

	myADEX->C = c_factor * neuron_parameter.cm;
	myADEX->g_l = g_factor * neuron_parameter.g_l;
	myADEX->E_l = V_factor * neuron_parameter.v_rest;
	myADEX->E_reset = V_factor * neuron_parameter.v_reset;
	myADEX->E_e = V_factor * neuron_parameter.e_rev_E;
	myADEX->E_i = V_factor * neuron_parameter.e_rev_I;
	myADEX->tau_syn_e = t_factor * neuron_parameter.tau_syn_E;
	myADEX->tau_syn_i = t_factor * neuron_parameter.tau_syn_I;
	myADEX->V_t = V_factor*neuron_parameter.v_thresh;
	myADEX->D_t = V_factor*neuron_parameter.delta_T;
	// make sure that D_T is not 0.
	if ( myADEX->D_t < std::numeric_limits<double>::epsilon() ) {
		myADEX->D_t = std::numeric_limits<double>::epsilon();
        LOG4CXX_DEBUG(logger, "hw_neuron_adex::init() setting D_t to " << myADEX->D_t << " to avoid division by zero" );
	}
	// make sure that tau_w is not 0. to avoid division by zero
	myADEX->tau_w = t_factor * neuron_parameter.tau_w;
	if (myADEX->tau_w == 0. ) {
		myADEX->tau_w = std::numeric_limits<double>::epsilon();
		LOG4CXX_DEBUG(logger, "hw_neuron_adex::init() setting tau_w to " << myADEX->tau_w << " to avoid division by zero" );
	}
	myADEX->a = g_factor * neuron_parameter.a;
	myADEX->b = a_factor * neuron_parameter.b;
	myADEX->V_spike = V_factor * neuron_parameter.v_spike;
	tau_refrac = neuron_parameter.tau_refrac*t_factor*1.e9;	//TODO correct factors for the refractory period
	//Make sure, that tau_refrac has a minimum value of 10(NS)
	if(tau_refrac < 10.)
		tau_refrac=10.;
	// make sure that time_step is at <= 0.1 ms
	double timestep =0.0001;
	if(dt <= 0.1)
		timestep = dt*t_factor;

	// set integration timestep in seconds, and if voltage is to be recorded
	myADEX->init(timestep, rec, fn);

	// print params
	LOG4CXX_DEBUG(logger, name() << "ADEX params");
	LOG4CXX_DEBUG(logger, "C            = " << myADEX->C        << " F" );
	LOG4CXX_DEBUG(logger, "g_l          = " << myADEX->g_l      << " S" );
	LOG4CXX_DEBUG(logger, "E_l          = " << myADEX->E_l      << " V" );
	LOG4CXX_DEBUG(logger, "E_reset      = " << myADEX->E_reset  << " V" );
	LOG4CXX_DEBUG(logger, "E_e          = " << myADEX->E_e      << " V" );
	LOG4CXX_DEBUG(logger, "E_i          = " << myADEX->E_i      << " V" );
	LOG4CXX_DEBUG(logger, "tau_syn_e    = " << myADEX->tau_syn_e<< " s" );
	LOG4CXX_DEBUG(logger, "tau_syn_i    = " << myADEX->tau_syn_i<< " s" );
	LOG4CXX_DEBUG(logger, "V_t          = " << myADEX->V_t      << " V" );
	LOG4CXX_DEBUG(logger, "D_t          = " << myADEX->D_t      << " V" );
	LOG4CXX_DEBUG(logger, "tau_w        = " << myADEX->tau_w    << " s" );
	LOG4CXX_DEBUG(logger, "a            = " << myADEX->a        << " S" );
	LOG4CXX_DEBUG(logger, "b            = " << myADEX->b        << " A" );
	LOG4CXX_DEBUG(logger, "V_spike      = " << myADEX->V_spike  << " V" );
	LOG4CXX_DEBUG(logger, "tau_refrac   = " << tau_refrac       << " ns");
	LOG4CXX_DEBUG(logger, "timestep     = " << timestep         << " s" );
        
    LOG4CXX_DEBUG(logger, "hw_neuron::init() successfully called!" );
}


void hw_neuron_ADEX::end_of_simulation()
{
    LOG4CXX_DEBUG(logger, "hw_neuron_adex::end_of_simulation_called()!" );
	// If recording, run recording up to the end of simulation
	if(rec_voltage)
	{
		t = sc_time_stamp().to_seconds();
		double dt = t-tl;
		myADEX->run(dt,refrac);
	}
	delete myADEX;
}

//return neuron parameter, all values in the following units (nF,mV,nA,nS,ms)
ESS::BioParameter hw_neuron_ADEX::get_neuron_parameter() const
{
    ESS::BioParameter returnval;
	returnval.cm        = myADEX->C / c_factor;
	returnval.g_l       = myADEX->g_l / g_factor;
	returnval.v_rest    = myADEX->E_l / V_factor; 
	returnval.v_reset   = myADEX->E_reset / V_factor; 
	returnval.e_rev_E   = myADEX->E_e / V_factor; 
	returnval.e_rev_I   = myADEX->E_i / V_factor; 
	returnval.tau_syn_E = myADEX->tau_syn_e / t_factor; 
	returnval.tau_syn_I = myADEX->tau_syn_i / t_factor; 
	returnval.v_thresh  = myADEX->V_t / V_factor;
	returnval.delta_T   = myADEX->D_t / V_factor;
	returnval.tau_w     = myADEX->tau_w / t_factor; 
	returnval.a         = myADEX->a / g_factor; 
	returnval.b         = myADEX->b / a_factor; 
	returnval.v_spike   = myADEX->V_spike / V_factor; 
	returnval.tau_refrac = tau_refrac / (t_factor*1.e9);	//TODO correct factors for the refractory period
    return returnval;
}
