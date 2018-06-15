#pragma once

#include "systemc.h"
#include "anncore_pulse_if.h"
#include <queue>
#include "CompoundNeuron.h"

/** module for the simulation of a compound neuron.
 * simulates a compound neuron, takes care about voltage recording and
 * propagates spikes to the attached priority encoder.
 * The neuron dynamics are updated every 1 nano second.
 */
class CompoundNeuronModule : public sc_module
{
public:
	CompoundNeuronModule(sc_module_name name, size_t N, unsigned int log_neuron, unsigned int wta, sc_port<anncore_pulse_if> *a);
	virtual ~CompoundNeuronModule();
	void tick();

	/** function to initialize compound neuron with 6-bit adr and recording parameters
	 * @param _addr 6-bit adress on layer 1 bus
	 * @param rec flag whether voltage shall be recorded
	 * @param fn filename for voltage recording
	 * @param dt voltage recording sampling interval in ms
	 * */
	void init(unsigned int _addr, bool rec, std::string fn, double dt);

	/// set firing denmem.
	/// Note: this is the id within this compound neuron, not the absolut denmem id
	void setFiringDenmem(size_t id);

	unsigned int get_wta_id() const;
	unsigned int get_6_bit_address() const;

private:
	double mLastTime; ///< time of last update
	sc_clock mClock; //!< systemc clock

	sc_event trigger_record; //!< triggers function `record()`
	double recording_interval; //!< recording interval in seconds.
	double mLastRecordTime; ///< time of last voltage recording

public:
	CompoundNeuron mCompoundNeuron; ///< compound neuron, holds and simulates the actual neuron dynamics

private:

	unsigned int logical_neuron;///< id of the denmem that fires. This is different from the "local" firing denmem set via setFiringDenmem()
	unsigned int addr;	        ///< 6-bit addr of this nrn: this is forwarded to l1-bus and dnc
	unsigned int wta;	        ///< ID of Priority Encoder(WTA) this nrn is connected to: 0..7
	std::ofstream voltage_file; ///< outstream to voltage recording file
	bool rec_voltage;	        ///< Flag for voltage recording: True = record membrane voltage to file

	/** this is the target where the anncore delivers its output spikes.*/
	sc_port<anncore_pulse_if> *anc;

	sc_event rel_spike; ///< triggers release_spike()
	bool rel_spike_lock;
	std::queue< sc_time > release_spike_buffer; // buffer to store events that have to be release after L1_DELAY_REP_TO_DENMEMbut there is already another event scheduled before.

	/** this function is called, when neuron spikes, V is reset after outgoing spike. */
	void spike_out();

	/** this function is called, when  a spike is released */
	void release_spike();

	/** records voltage, adaptation current and synaptic conductances.
	 * Is triggered by `trigger_record` every `recording_interval`*/
	void record();

	void end_of_simulation();  ///< call back function from sc_module, is called when sc_stop() is called. We use this for finalizing the voltage files.
};
