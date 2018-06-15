//_____________________________________________
// Company      :	UNI Heidelberg	
// Author       :	Bernhard Vogginger
// E-Mail   	:	bernhard.vogginger@kip.uni-heidelberg.de
//_____________________________________________

#ifndef _HW_NEURON_IFSC_H_
#define _HW_NEURON_IFSC_H_	

#include "systemc.h"
#include "hw_neuron.h"

/** class hw_neuron_IFSC
 provides functionality a group of connected denmems.
  It is configured with hardware neuron parameters (hw_params).
  As we do not want to simulate the "analogue" hardware neuron, we can choose here between 2 types:
  	1.) a conductance-based-leaky I&F-Neuron by Brette(IFSC)
	As this is an event-based analytical Neuron-Model it fits well into the systemC environment.
	The Computation of the Neuron is very fast.
	But it has the restrictions that excitatory and inhibitory synaptic time constants have to be equal, which limits variability.
	Also, atm. the ratio between the leakage-time-constant and synaptic time constant is set globally(for whole FACETS_HARDWARE)
 */
class hw_neuron_IFSC : public hw_neuron {

	public:	
		double Ep ; 	///< IFSC: excitatory reversal Potential in Terms of Vt
		double t;	///< IFSC: current time(in IFSC - time unit)
		double tl;	///< IFSC: time of last incoming spike
		double Es;	///< IFSC: effective synaptic reversal potential
		double Em; 	///< IFSC: inhibitory reversal Potential
		double V;	///< IFSC: initial value of membrane potential(arbitrary betw. V0 and Vt)
		double Vt;	///< IFSC: threshold voltage(normalized)
		double g;	///< IFSC: initial value of conductance in terms of leak conductance
		double V0; 	///< IFSC: reset potential
		double spike_time;	///< time to next spike(SystemSimTime in Seconds) 

		/** Biological Neuron Parameters:
		 * PyNN like parameters in hardware voltage and time domain.
		 */
        ESS::BioParameter neuron_parameter;

		sc_event rec_v;	///< triggers record_v()

		FILE * voltage_file;	///< File to record voltage of this Neuron
		hw_neuron_IFSC();	///< standard constructor

		/** this function is called, when neuron spikes, V is reset after outgoing spike. */
		virtual void spike_out();	
		/** this function is called, when  a spike is released */
		virtual void release_spike();	
		/** this function is called, when refractory time is over*/
		virtual void end_of_refrac_period();	
		/** updates the time of next spike, and may trigger spike_out()*/
		virtual void update_spike_timing();
		/** handles incoming spike, updates all variables and next spike time, this may trigger spike_out()
		 * @param weight weight(in Siemens) that is added to corresponding synaptic conductance
		 * @param syn_type Type of incoming pulse(0=excitatory, 1=inhibitory)*/
		virtual void spike_in(double weight, char syn_type);	
		/** records membrane voltage: CAUTION this is very computation-cost-worthy */
		virtual void record_v();
		
		/** function to initialize hw_neuron with neuron parameters, 6-bit adr, and recording parameters
		 * @param _hw_params struct containing all hardware-specific neuron parameters
		 * @param _addr 6-bit adress on layer 1 bus
		 * @param rec flag whether voltage shall be recorded
		 * @param fn filename for voltage recording
		 * @param dt voltage recording sampling interval in ms 
		 * */
		virtual void init(ESS::BioParameter _neuron_parameter, unsigned int _addr, bool rec, std::string fn, double dt);
		
		/// SC_HAS_PROCESS needed for declaration of SC_METHOD if not using SC_CTOR
		SC_HAS_PROCESS(hw_neuron_IFSC);	
		
		/** Constructor
		 * @param log_neuron: logical_neuron on hicann(starting from 0..max. 511)
		 * @param adr: 6-bit adr on layer 1 bus
		 * @param wta: ID of WTA this Neuron is connected to
		 * @param a: pointer to anncore for outgoing spikes
		 */
		hw_neuron_IFSC(sc_module_name name, unsigned int log_neuron, unsigned int adr, unsigned int wta, sc_port<anncore_pulse_if> *a);

		~hw_neuron_IFSC();	///< standard destructor
};
#endif// _HW_NEURON_IFSC_H_	
