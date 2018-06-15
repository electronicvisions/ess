//_____________________________________________
// Company      :	UNI Heidelberg	
// Author       :	Bernhard Vogginger
// E-Mail   	:	bernhard.vogginger@kip.uni-heidelberg.de
//_____________________________________________
// TODO:
// * rm random stuff

#ifndef _HW_NEURON_H_
#define _HW_NEURON_H_

#include "systemc.h"
#include "anncore_pulse_if.h"
#include <queue>
#include "HAL2ESSContainer.h"
//#include "sim_def.h"

class ADEX;
/** class hw_neuron
 provides functionality a group of connected denmems.
  It is configured with hardware neuron parameters (hw_params).
 */
class hw_neuron: public sc_module
{
	protected:
		unsigned int logical_neuron;///< logical neuron id: 0..NRN_ANN
		unsigned int addr;	        ///< 6-bit addr of this nrn: this is forwarded to l1-bus and dnc
		unsigned int wta;	        ///< ID of Priority Encoder(WTA) this nrn is connected to: 0..7
		bool rec_voltage;	        ///< Flag for voltage recording: True = record membrane voltage to file
		double tau_refrac;	        ///< Refractory time in nanoSeconds
		unsigned int counter;
		unsigned int counter_thresh;

		double spike_time;	///< time to next spike(SystemSimTime in Seconds) 

		sc_event next_spike; ///< triggers spike_out()
		sc_event rel_spike; ///< triggers release_spike()
		sc_event end_of_refrac; ///< triggers end_of_refrac_period()
		sc_event spk_time_up;	///< triggers update_spike_timing()
		sc_event next_rand_spike; ///< triggers start_random_spike()

		bool rel_spike_lock;
		std::queue< sc_time > release_spike_buffer; // buffer to store events that have to be release after L1_DELAY_REP_TO_DENMEMbut there is already another event scheduled before.

		/** this is the target where the anncore delivers its output spikes.*/
		sc_port<anncore_pulse_if> *anc;


		/** this function is called, when neuron spikes, V is reset after outgoing spike. */
		virtual void spike_out();
		/** this function is called, when  a spike is released */
		virtual void release_spike();
		/** this function is called, when refractory time is over*/
		virtual void end_of_refrac_period();
		/** updates the time of next spike, and may trigger spike_out()*/
		virtual void update_spike_timing();
        /** function to generate random spikes with a given frequency for simulation-time-estimation*/
		void gen_random_spike();
		/** function to start random spikes with a given frequency for simulation-time-estimation*/
		void start_random_spike();

		friend class test_hw_neuron;

	public:

		hw_neuron();	///< standard constructor

		/** Constructor
		 * @param log_neuron: logical_neuron on hicann(starting from 0..max. 511)
		 * @param adr: 6-bit adr on layer 1 bus
		 * @param wta: ID of WTA this Neuron is connected to
		 * @param a: pointer to anncore for outgoing spikes
		 */
		hw_neuron(sc_module_name name, unsigned int log_neuron, unsigned int adr, unsigned int wta, sc_port<anncore_pulse_if> *a);

		virtual ~hw_neuron();	/// standard destructor


		/// SC_HAS_PROCESS needed for declaration of SC_METHOD if not using SC_CTOR
		SC_HAS_PROCESS(hw_neuron);


		/** handles incoming spike, updates all variables and next spike time, this may trigger spike_out()
		 * @param weight weight(in Siemens) that is added to corresponding synaptic conductance
		 * @param syn_type Type of incoming pulse(0=excitatory, 1=inhibitory)*/
		virtual void spike_in(double weight, char syn_type);

		/** function to initialize hw_neuron with neuron parameters, 6-bit adr, and recording parameters
		 * @param _hw_params struct containing all hardware-specific neuron parameters
		 * @param _addr 6-bit adress on layer 1 bus
		 * @param rec flag whether voltage shall be recorded
		 * @param fn filename for voltage recording
		 * @param dt timestep in ms used for numerical integration and as voltage recording sampling interval
		 * */
		virtual void init(ESS::BioParameter _neuron_parameter, unsigned int _addr, bool rec, std::string fn, double dt);

		/** update amplitude of external current injected into neuron.
		 * @params current current amplitude in Ampere
		 * */
        virtual void update_current(double const current) = 0;

		/** get biological neuron parameters.
		 * returns PyNN like parameters in hardware voltage and time domain.*/
        virtual ESS::BioParameter get_neuron_parameter() const = 0;

        void set_logical_neuron_id(unsigned int log);
		void set_biological_neuron_id(unsigned int bio);
		void set_6_bit_address(unsigned int addr);
		void set_wta_id(unsigned int wta);

		unsigned int get_logical_neuron_id();
		unsigned int get_wta_id();
		unsigned int get_biological_neuron_id();
		unsigned int get_6_bit_address();
        double get_refrac() const
        { return tau_refrac;}
};
#endif // _HW_NEURON_H_	
