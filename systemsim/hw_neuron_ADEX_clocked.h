#ifndef _HW_NEURON_ADEX_clocked_H_
#define _HW_NEURON_ADEX_clocked_H_

#include "systemc.h"
#include "hw_neuron.h"
#include "CompoundNeuron.h"

/** class hw_neuron_ADEX_clocked
 Implementation of a single ADEX neuron that is simulated time-driven.
 In contrast to hw_neuron_ADEX, which is an event-based implementation,
 this version is clock-based and is updated in constant intervals.
 Internally, a `CompoundNeuron` with a single denmem is used.
 */
class hw_neuron_ADEX_clocked: public hw_neuron
{
	private:
		double mLastTime; ///< time of last update
		sc_clock mClock; //!< systemc clock

		sc_event trigger_record; //!< triggers function `record()`
		double recording_interval; //!< recording interval in seconds.
		double mLastRecordTime; ///< time of last voltage recording

		CompoundNeuron mCompoundNeuron; ///< compound neuron, holds and simulates the actual neuron dynamics
		const size_t one_and_only_denmem = 0; ///< ID of one and only denmem in compound neuron

		std::ofstream voltage_file; ///< outstream to voltage recording file

		/** updates neuron dynamics at every posedge of mClock from mLastTime to now. */
		void tick();

		/** records voltage, adaptation current and synaptic conductances.
		 * Is triggered by `trigger_record` every `recording_interval`*/
		void record();

		/** this function is called, when neuron spikes, V is reset after outgoing spike. */
		virtual void spike_out();

		/** this function is called, when  a spike is released */
		virtual void release_spike();

		/** derived from class `hw_neuron`, has empty functionality in clock-based version.
		 * Thus, this should not be called.
		 * @throws runtime_error when called.
		 * */
		virtual void end_of_refrac_period();

	public:
		/** handles incoming spike and increase synaptic conductances by `weight`.
		 * @param weight weight(in Siemens) that is added to corresponding synaptic conductance
		 * @param syn_type Type of incoming pulse(0=excitatory, 1=inhibitory)*/
		virtual void spike_in(double weight, char syn_type);

		/** update amplitude of external current injected into neuron.
		 * @params current current amplitude in Ampere
		 * */
        virtual void update_current(double const current);

		/** function to initialize hw_neuron with neuron parameters, 6-bit adr, and recording parameters
		 * @param _hw_params struct containing all hardware-specific neuron parameters
		 * @param _addr 6-bit adress on layer 1 bus
		 * @param rec flag whether voltage shall be recorded
		 * @param fn filename for voltage recording
		 * @param dt voltage recording sampling interval in ms
		 * */
		virtual void init(ESS::BioParameter _neuron_parameter, unsigned int _addr, bool rec, std::string fn, double dt);

		/** Constructor
		 * @param log_neuron: logical_neuron on hicann(starting from 0..max. 511)
		 * @param adr: 6-bit adr on layer 1 bus
		 * @param wta: ID of WTA this Neuron is connected to
		 * @param a: pointer to anncore for outgoing spikes
		 */
		hw_neuron_ADEX_clocked(sc_module_name name, unsigned int log_neuron, unsigned int adr, unsigned int wta, sc_port<anncore_pulse_if> *a);

		virtual ~hw_neuron_ADEX_clocked();	///< standard destructor

		/** get biological neuron parameters.
		 * returns PyNN like parameters in hardware voltage and time domain.*/
        ESS::BioParameter get_neuron_parameter() const;

        void end_of_simulation();  ///< call back function from sc_module, is called when sc_stop() is called. We use this for finalizing the voltage files.
};

#endif //_HW_NEURON_ADEX_clocked_H_
