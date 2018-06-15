#include "hw_neuron_IFSC.h"
#include "IFSC.h"
#include "neuron_def.h"
#include <log4cxx/logger.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("ESS.HICANN.Neuron");

/** Constructor
 * param log_neuron: logical_neuron
 * param adr: 6-bit adr
 * param wta: ID of WTA this Neuron is connected to
 * param a: pointer to anncore for outgoing spikes
 */
hw_neuron_IFSC::hw_neuron_IFSC(sc_module_name name, unsigned int log_neuron, unsigned int adr, unsigned int wta, sc_port<anncore_pulse_if> *a):
	hw_neuron(name, log_neuron, adr, wta, a)
	, Ep(E_PLUS)
	, tl(0)
	, Es(E_S_INIT)
	, Em(E_MINUS)
	, V(V_INIT)
	, Vt(V_THRESH)
	, g(0.00000001)
	, V0(V_REST)
{
	
	if(rec_voltage){
		char filename[1024];
		snprintf(filename,sizeof(filename),"voltage.dat");
		voltage_file = fopen(filename,"w");
		    //fprintf(voltage_file,"// bio_V(in mV), V(normalized), bio_g(in uS), g(normalized), Es\n");
		    //fflush(voltage_file);

		SC_METHOD(record_v);
		sensitive << rec_v;
	}

    LOG4CXX_DEBUG(logger, "HardwareNeuron " << logical_neuron << " built!" );
}

hw_neuron_IFSC::~hw_neuron_IFSC()
{
    LOG4CXX_DEBUG(logger, "hw_neuron_IFSC::destructor: " );
	if(rec_voltage){
        LOG4CXX_DEBUG(logger, "hw_neuron_IFSC::destructor: closing voltage file" );
		fclose(voltage_file);
	}
}

void hw_neuron_IFSC::spike_out()	
{
	// IFSC stuff: needs getting input after sc_start();
	t = (sc_time_stamp().to_seconds()/TAU_UNIT);
	double dt = t-tl;
	
	g = IFSC_OutgoingSpike(dt, g);
	V = V0;
	tl=t;
    LOG4CXX_TRACE(logger, "spike_out: t = " << sc_time_stamp() <<"\t Neuron_ID : " << logical_neuron <<  "\t V: " << V << "\t g: " << g << "\t Es: " << Es << "\tnext spike : NOW" );
	spk_time_up.notify(SC_ZERO_TIME);
	rel_spike.notify(10, SC_NS);
}
void hw_neuron_IFSC::release_spike()	
{
    LOG4CXX_TRACE(logger, "hw_neuron_ADEX::release_spike():" << sc_time_stamp() <<"\t Neuron_ID : " << logical_neuron );
	(*anc)->handle_spike(logical_neuron, addr, wta);
}

void hw_neuron_IFSC::end_of_refrac_period()
{
    LOG4CXX_TRACE(logger, "hw_neuron_IFSC::end_of_refrac_period():" << sc_time_stamp() <<"\t Neuron_ID : " << logical_neuron );
	/*
	t = sc_time_stamp().to_seconds();
	double dt =t-tl;
	myADEX->run_refrac(dt);		
	tl=t;
	refrac=false;
	spk_time_up.notify(SC_ZERO_TIME);
	*/
}

void hw_neuron_IFSC::spike_in(double weight, char type)
{
    LOG4CXX_TRACE(logger, name() << " spike_in(" << (double) weight << ") at: "<< sc_time_stamp() );

	// IFSC stuff: needs getting input after sc_start();
	// factor 50 because 
	t = (sc_time_stamp().to_seconds()/TAU_UNIT);
	
	double dt = t-tl;
	//log(Logger::DEBUG2) << "dt = " << dt << endl;
	// Update V, g and Es up to time of incoming spike
	// weight*20 test
	double weight_IFSC = weight/(neuron_parameter.g_l*1e-9);
	IFSC_IncomingSpike(dt, weight_IFSC, &V, &g, &Es, Ep, Em);
	// Get Time of next spike(out) of this neuron and add event into queue(by next_spike.notify(spike_time; SC_TIME_STEP);
	spike_time = (IFSC_SpikeTiming(V,g,Es) * TAU_UNIT);
	next_spike.cancel();
	next_spike.notify(spike_time, SC_SEC);
	
    LOG4CXX_TRACE(logger, name() << " spike_in:  t = " << sc_time_stamp() <<"\t Neuron_ID : " << logical_neuron << "\t V: " << V << "\t g: " << g << "\t Es: " << Es << "\tnext spike : "<< spike_time );
	tl = t;
}	

void hw_neuron_IFSC::update_spike_timing()
{
	// Get Time of next spike(out) of this neuron and add event into queue(by next_spike.notify(spike_time; SC_TIME_STEP);
	spike_time = (IFSC_SpikeTiming(V,g,Es) * TAU_UNIT);
	next_spike.cancel();
	next_spike.notify(spike_time, SC_SEC);
	
    LOG4CXX_TRACE(logger, name() << "update_spike_timing: t = " << sc_time_stamp() <<"\t Neuron_ID : " << logical_neuron << "\t V: " << V << "\t g: " << g << "\t Es: " << Es << "\tnext spike : "<< spike_time );
}

void hw_neuron_IFSC::record_v()
{	
	double loc_V = V;
	double loc_g = g;	
	double loc_Es = Es;
	double	tt = (sc_time_stamp().to_seconds()/TAU_UNIT);
	double spike_time_in_s = sc_time_stamp().to_seconds();
	
	double dt = tt-tl;
	IFSC_IncomingSpike(dt, 0, &loc_V, &loc_g, &loc_Es, Ep, Em);
	
	double bio_V = (-65 + 15*loc_V); 
	double bio_g = loc_g*0.05; // bio_g in uS
	if (voltage_file) {	
	    fprintf(voltage_file,"%.6f %.3f\t%.3f\t%.6f\t%.3f\t%.3f\n",spike_time_in_s, bio_V, loc_V, bio_g, loc_g, loc_Es);
	    fflush(voltage_file);
	} else {
        LOG4CXX_WARN(logger, name() << "hw_neuron_IFSC::record_v(): No open pulse record file found in pulse_rx_neuron" );
	}
	// set time for next record
	rec_v.notify(10,SC_US);
}
void hw_neuron_IFSC::init(ESS::BioParameter _neuron_parameter, unsigned int _addr, bool rec, std::string fn, double dt)
{
	this->addr=_addr;
	this->neuron_parameter = _neuron_parameter;
	//////////////////////////////////////////////////
	//	Compute Translation from hw_params to IFSC
	//////////////////////////////////////////////////
	//	V_exp -> Vt (= V_threshold)
	//	V_reset -> V0
	//	E_syn_e -> Ep
	//	E_syn_i -> Em
	//		
	this->Vt=1.0;
	this->V0=0.0;
	this->Ep=((neuron_parameter.e_rev_E - neuron_parameter.v_rest)/(neuron_parameter.v_spike - neuron_parameter.v_rest));
	this->Em=((neuron_parameter.e_rev_I - neuron_parameter.v_rest)/(neuron_parameter.v_spike- neuron_parameter.v_rest));

	this->Es= (this->Ep + this->Em);

    LOG4CXX_INFO(logger, name() << "hw_neuron_IFSC::init() successfully called!" );
}
