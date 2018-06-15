#include "CompoundNeuronModule.h"
#include <log4cxx/logger.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("ESS.HICANN.Neuron");

CompoundNeuronModule::CompoundNeuronModule(sc_module_name name, size_t N, unsigned int log_neuron, unsigned int wta, sc_port<anncore_pulse_if> *a):
	sc_module(name)
	,mLastTime(0.)
	,mClock("mClock", 5,SC_NS)
	,recording_interval(10.e-9)
	,mLastRecordTime(-1.) // any values < 0 to allow recording at t=0.
	,mCompoundNeuron(N)
	,logical_neuron(log_neuron)
	,addr(0)
	,wta(wta)
	,rec_voltage(false)
	,anc(a)
	,rel_spike_lock(false)
{
	SC_HAS_PROCESS(CompoundNeuronModule);

	// TODO: call tick one delta cycle later!
	SC_METHOD(tick);
	dont_initialize();
	sensitive << mClock.posedge_event();

	SC_METHOD(release_spike);
	dont_initialize();
	sensitive << rel_spike;

	SC_METHOD(record);
	dont_initialize();
	sensitive << trigger_record;
}

CompoundNeuronModule::~CompoundNeuronModule() {
	if ( !sc_end_of_simulation_invoked() ) {
		// If recording, run recording up to the end of simulation
		if(rec_voltage){
			record();
		}
	}
	if(rec_voltage){
		voltage_file.close();
	}
    LOG4CXX_DEBUG(logger, "CompoundNeuronModule::destructed" );
}

void CompoundNeuronModule::tick() {
	double CurrentTime = sc_time_stamp().to_seconds();
	if (CurrentTime > mLastTime) {
		bool has_fired = mCompoundNeuron.update(mLastTime, CurrentTime);
		mLastTime=CurrentTime;
		if ( has_fired )
			spike_out();
	}
}

void CompoundNeuronModule::init(unsigned int _addr, bool rec, std::string fn, double dt) {
    addr  =_addr;
	rec_voltage = rec;

	mCompoundNeuron.initialize();

	if(rec_voltage)
	{
		voltage_file.open(fn);
		voltage_file.setf(std::ios::scientific,std::ios::floatfield); // scientific values for floating point number, default precision of 6 is ok.
		// print values at time 0
		voltage_file << "#t[Seconds]" << "\t" << "V[Volt]" << "\t" << "w[Ampere]" << "\t" << "g_e[Siemens]" << "\t" << "g_i[Siemens]" << "\n";
		CompoundNeuronState const & state =  mCompoundNeuron.mState;
		voltage_file << mLastTime << "\t" << state.V << "\t" << state.denmems[0].w
			<< "\t" << state.denmems[0].g_syn[0] << "\t" << state.denmems[0].g_syn[1] << "\n";
	}

	static const double t_factor = 0.001;
	recording_interval = dt*t_factor;
	trigger_record.notify(recording_interval, SC_SEC);

    LOG4CXX_DEBUG(logger, "CompoundNeuronModule::init() successfully called!" );
}

void CompoundNeuronModule::setFiringDenmem(size_t id) {
	mCompoundNeuron.setFiringDenmem(id);
}

void CompoundNeuronModule::spike_out()
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

void CompoundNeuronModule::release_spike()
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

void CompoundNeuronModule::record()
{
	const double CurrentTime = sc_time_stamp().to_seconds();
	if (rec_voltage &&  CurrentTime > mLastRecordTime) {
		tick();
		CompoundNeuronState const & state =  mCompoundNeuron.mState;
		voltage_file << mLastTime << "\t" << state.V << "\t" << state.denmems[0].w
			<< "\t" << state.denmems[0].g_syn[0] << "\t" << state.denmems[0].g_syn[1] << "\n" << std::flush;
		trigger_record.notify(recording_interval, SC_SEC);
		mLastRecordTime = CurrentTime;
	}
}

void CompoundNeuronModule::end_of_simulation()
{
    LOG4CXX_DEBUG(logger, "CompoundNeuronModule::end_of_simulation_called()!" );
	// If recording, run recording up to the end of simulation
	if(rec_voltage){
		record();
	}
}

unsigned int CompoundNeuronModule::get_wta_id() const {
	return wta;
}

unsigned int CompoundNeuronModule::get_6_bit_address() const {
	return addr;
}
