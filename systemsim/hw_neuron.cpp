#include "hw_neuron.h"
#include "neuron_def.h"
#include <log4cxx/logger.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("ESS.HICANN.Neuron");

/** Constructor
 * param log_neuron: logical_neuron
 * param adr: 6-bit adr
 * param wta: ID of WTA this Neuron is connected to
 * param a: pointer to anncore for outgoing spikes
 */
hw_neuron::hw_neuron(sc_module_name name, unsigned int log_neuron, unsigned int adr, unsigned int wta, sc_port<anncore_pulse_if> *a):
	sc_module(name)
	, logical_neuron(log_neuron)
	, addr(adr)
	, wta(wta)
	, rec_voltage(false)	// DEFAULT:FALSE
    , tau_refrac(10.)
	, counter(0)
	, counter_thresh(100)
	, rel_spike_lock(false)
	, anc(a)
{
    LOG4CXX_DEBUG(logger, "HardwareNeuron " << logical_neuron << " built!" );
	
	SC_METHOD(spike_out);
	dont_initialize();
	sensitive << next_spike;

	SC_METHOD(release_spike);
	dont_initialize();
	sensitive << rel_spike;

	SC_METHOD(end_of_refrac_period);
	dont_initialize();
	sensitive << end_of_refrac;

	SC_METHOD(update_spike_timing);
//	dont_initialize();
	sensitive << spk_time_up;

	SC_METHOD(start_random_spike);
	dont_initialize();
	sensitive << next_rand_spike;
		
	//srand( (unsigned) time(NULL));
}

hw_neuron::~hw_neuron(){
    LOG4CXX_DEBUG(logger, "hw_neuron::destructor " );
}

void hw_neuron::spike_out()	
{
}

void hw_neuron::release_spike()	
{
    (*anc)->handle_spike(logical_neuron, addr, wta);
}

void hw_neuron::end_of_refrac_period()
{
    LOG4CXX_TRACE(logger, "hw_neuron::end_of_refrac_period():" << sc_time_stamp() <<"\t Neuron_ID : " << logical_neuron );

}

void hw_neuron::spike_in(double /*weight*/, char /*type*/)
{
	counter++;
	if(counter>=counter_thresh){
		release_spike();
		counter=0;
	}
}	

void hw_neuron::update_spike_timing()
{
}

/** function to generate random spikes with a given frequency for simulation-time-estimation*/
void hw_neuron::gen_random_spike(){
	double delta_time_in_NS = 1e9*((double) 2*rand()/RAND_MAX)/(RAND_FREQ_IN_HZ);
	next_rand_spike.notify(delta_time_in_NS, SC_NS);

}
/** function to start random spikes with a given frequency for simulation-time-estimation*/
void hw_neuron::start_random_spike(){
	// Send spike to wta
    LOG4CXX_TRACE(logger, name() << "::random_spike at: " << sc_time_stamp() );
	(*anc)->handle_spike(logical_neuron, addr, wta);
	// call gen_random_spike for next spike
	gen_random_spike();
}

void hw_neuron::init(ESS::BioParameter /*_neuron_parameter*/, unsigned int _addr, bool /*rec*/, std::string /*fn*/, double /*dt*/)
{
	this->addr=_addr;
    LOG4CXX_DEBUG(logger, "hw_neuron::init() successfully called!" );
}


void hw_neuron::set_logical_neuron_id(unsigned int log )
{
	hw_neuron::logical_neuron = log;
}

void hw_neuron::set_6_bit_address(unsigned int addr)
{
	hw_neuron::addr = addr;
}

void hw_neuron::set_wta_id(unsigned int wta)
{
	hw_neuron::wta = wta;
}

unsigned int hw_neuron::get_logical_neuron_id()
{
	return hw_neuron::logical_neuron;
}

unsigned int hw_neuron::get_wta_id()
{
	return hw_neuron::wta;
}

unsigned int hw_neuron::get_6_bit_address()
{
	return addr;
}
