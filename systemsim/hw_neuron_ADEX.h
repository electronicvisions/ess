//_____________________________________________
// Company      :	UNI Heidelberg	
// Author       :	Bernhard Vogginger
// E-Mail   	:	bernhard.vogginger@kip.uni-heidelberg.de
//_____________________________________________

#ifndef _HW_NEURON_ADEX_H_
#define _HW_NEURON_ADEX_H_	

#include "systemc.h"
#include "hw_neuron.h"

// pre-declaration
class ADEX;

/** class hw_neuron_ADEX
 provides functionality a group of connected denmems.
  It is configured with hardware neuron parameters (hw_params).
  This neuron implements the following model:
  Adaptive-Exponential I&F Neuron with conductance based synapses(AdEx)
  As this model has to be computed numerically, its use is very computation-cost-worthy!
 */ 
class hw_neuron_ADEX: public hw_neuron
{
	private:
		double t;	///< ADEX: current time
		double tl;	///< ADEX: time of last incoming spike or change of current stimulus
		bool refrac;	///< flag for refractory period
		ADEX * myADEX;	///< Adex neuron class instance

		/** Biological Neuron Parameters:
		 * PyNN like parameters in hardware voltage and time domain.
		 */
        ESS::BioParameter neuron_parameter;

        //constants for parameter scaling
        const double V_factor = 0.001; //from mV to V
	    const double t_factor = 0.001; //from ms to s
	    const double a_factor = 1.e-9; //from nA to A
	    const double c_factor = 1.e-9; //from nF to F
	    const double g_factor = 1.e-9; //from nS to S

		/** this function is called, when neuron spikes, V is reset after outgoing spike. */
		virtual void spike_out();	
		/** this function is called, when  a spike is released */
		virtual void release_spike();	
		/** this function is called, when refractory time is over*/
		virtual void end_of_refrac_period();	
		/** updates the time of next spike, and may trigger spike_out()*/
		virtual void update_spike_timing();

	public:
		/** handles incoming spike, updates all variables and next spike time, this may trigger spike_out()
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
		 * @param dt timestep in ms used for numerical integration and as voltage recording sampling interval
		 * */
		virtual void init(ESS::BioParameter _neuron_parameter, unsigned int _addr, bool rec, std::string fn, double dt);

		/** Constructor
		 * @param log_neuron: logical_neuron on hicann(starting from 0..max. 511)
		 * @param adr: 6-bit adr on layer 1 bus
		 * @param wta: ID of WTA this Neuron is connected to
		 * @param a: pointer to anncore for outgoing spikes
		 */
		hw_neuron_ADEX(sc_module_name name, unsigned int log_neuron, unsigned int adr, unsigned int wta, sc_port<anncore_pulse_if> *a);

		~hw_neuron_ADEX();	/// standard destructor

		/** get biological neuron parameters.
		 * returns PyNN like parameters in hardware voltage and time domain.*/
        ESS::BioParameter get_neuron_parameter() const;
        
        void end_of_simulation();  /// call back function from sc_module, is called when sc_stop() is called. We use this for finalizing the voltage files and destroying myAdex.
};

#endif //_HW_NEURON_ADEX_H_	
