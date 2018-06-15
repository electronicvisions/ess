#include "hw_neuron_ADEX_clocked.h"
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
hw_neuron_ADEX_clocked::hw_neuron_ADEX_clocked(sc_module_name name, unsigned int log_neuron, unsigned int adr, unsigned int wta, sc_port<anncore_pulse_if> *a):
	hw_neuron(name, log_neuron, adr, wta, a)
	,mLastTime(0.)
	,mClock("mClock", 1,SC_NS)
	,recording_interval(10.e-9)
	,mLastRecordTime(-1.) // any values < 0 to allow recording at t=0.
	,mCompoundNeuron(/*size*/1)
{

	SC_HAS_PROCESS(hw_neuron_ADEX_clocked);

	SC_METHOD(tick);
	dont_initialize();
	sensitive << mClock.posedge_event();

	SC_METHOD(record);
	dont_initialize();
	sensitive << trigger_record;

    LOG4CXX_DEBUG(logger, "HardwareNeuron " << logical_neuron << " built!" );
}

hw_neuron_ADEX_clocked::~hw_neuron_ADEX_clocked(){
	if ( !sc_end_of_simulation_invoked() ) {
		// If recording, run recording up to the end of simulation
		if(rec_voltage){
			record();
		}
	}
	if(rec_voltage){
		voltage_file.close();
	}
    LOG4CXX_DEBUG(logger, "hw_neuron_ADEX_clocked::destructed" );
}

void hw_neuron_ADEX_clocked::tick() {
	double CurrentTime = sc_time_stamp().to_seconds();
	if (CurrentTime > mLastTime ) {
		bool has_fired = mCompoundNeuron.update(mLastTime, CurrentTime);
		mLastTime=CurrentTime;
		if ( has_fired )
			spike_out();
	}
}

void hw_neuron_ADEX_clocked::spike_out()
{
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
}

void hw_neuron_ADEX_clocked::release_spike()
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

void hw_neuron_ADEX_clocked::end_of_refrac_period()
{
	throw std::runtime_error("hw_neuron_ADEX_clocked::end_of_refrac_period() must not be called");
}

void hw_neuron_ADEX_clocked::spike_in(double weight, char syn_type)
{
    LOG4CXX_TRACE(logger, name() << ".spike_in(" << weight << ") type=" << (unsigned int) syn_type << "(exc=0, inh=1) at: "<< sc_time_stamp() );
	bool input = false;
	if(syn_type==0){
		input = false;
	}
	else if(syn_type==1){
		input = true;
	}
	else {
		LOG4CXX_ERROR(logger, "hw_neuron_ADEX_clocked::spike_in: wrong synapse type = " << (unsigned int) syn_type );
		throw std::runtime_error("hw_neuron_ADEX_clocked::spike_in: wrong synapse type");
	}
	mCompoundNeuron.inputSpike(one_and_only_denmem,input,weight);
}

void hw_neuron_ADEX_clocked::update_current(double const current)
{
	tick();
	mCompoundNeuron.inputCurrent(one_and_only_denmem,current);
}

void hw_neuron_ADEX_clocked::init(ESS::BioParameter _neuron_parameter, unsigned int _addr, bool rec, std::string fn, double dt)
{
    this->addr  =_addr;
	this->rec_voltage = rec;

	DenmemParams denmem_params(_neuron_parameter);

	if (denmem_params.delta_T < std::numeric_limits<double>::epsilon() ) {
		denmem_params.delta_T = std::numeric_limits<double>::epsilon();
        LOG4CXX_DEBUG(logger, "hw_neuron_ADEX_clocked::init() setting delta_T to " << denmem_params.delta_T << " to avoid division by zero" );
	}

	if (denmem_params.tau_w < std::numeric_limits<double>::epsilon() ) {
		denmem_params.tau_w = std::numeric_limits<double>::epsilon();
        LOG4CXX_DEBUG(logger, "hw_neuron_ADEX_clocked::init() setting tau_w to " << denmem_params.tau_w << " to avoid division by zero" );
	}

	mCompoundNeuron.setDenmemParams(one_and_only_denmem,denmem_params);
	mCompoundNeuron.setFiringDenmem(one_and_only_denmem);
	mCompoundNeuron.initialize();

	if(rec_voltage)
	{
		voltage_file.open(fn);
		voltage_file.setf(std::ios::scientific,std::ios::floatfield); // scientific values for floating point number, default precision of 6 is ok.
		// print values at time 0
		voltage_file << "#t[Seconds]" << "\t" << "V[Volt]" << "\t" << "w[Ampere]" << "\t" << "g_e[Siemens]" << "\t" << "g_i[Siemens]" << "\n";
		CompoundNeuronState const & state =  mCompoundNeuron.mState;
		voltage_file << mLastTime << "\t" << state.V << "\t" << state.denmems[one_and_only_denmem].w
			<< "\t" << state.denmems[one_and_only_denmem].g_syn[0] << "\t" << state.denmems[one_and_only_denmem].g_syn[1] << "\n";
	}

	static const double t_factor = 0.001;
	recording_interval = dt*t_factor;
	trigger_record.notify(recording_interval, SC_SEC);

    LOG4CXX_DEBUG(logger, "hw_neuron_ADEX_clocked::init() successfully called!" );
}


void hw_neuron_ADEX_clocked::end_of_simulation()
{
    LOG4CXX_DEBUG(logger, "hw_neuron_adex_clocked::end_of_simulation_called()!" );
	// If recording, run recording up to the end of simulation
	if(rec_voltage){
		record();
	}
}

//return neuron parameter, all values in the following units (nF,mV,nA,nS,ms)
ESS::BioParameter hw_neuron_ADEX_clocked::get_neuron_parameter() const
{
    return mCompoundNeuron.getDenmemParams(one_and_only_denmem).toBioParameter();
}

void hw_neuron_ADEX_clocked::record()
{
	const double CurrentTime = sc_time_stamp().to_seconds();
	if (rec_voltage &&  CurrentTime > mLastRecordTime) {
		tick();
		CompoundNeuronState const & state =  mCompoundNeuron.mState;
		voltage_file << mLastTime << "\t" << state.V << "\t" << state.denmems[one_and_only_denmem].w
			<< "\t" << state.denmems[one_and_only_denmem].g_syn[0] << "\t" << state.denmems[one_and_only_denmem].g_syn[1] << "\n" << std::flush;
		trigger_record.notify(recording_interval, SC_SEC);
		mLastRecordTime = CurrentTime;
	}
}
