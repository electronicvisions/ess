#ifndef _TEST_HW_NEURON_H_
#define _TEST_HW_NEURON_H_	

#include "systemc.h"
#include "anncore_pulse_if.h"
#include "neuron_param.h"

// pre-declarations
class hw_neuron;
class Logger;

/** class test_hw_neuron
 * serves as a testbench for hw_neurons and classes derived from it.
 */
class test_hw_neuron: public anncore_pulse_if, public sc_module {
	
	private:
		FILE * pulse_file_rx; ///< File with input pulses
		hw_neuron * hw_neuron_i; ///< hw_neuron instance
		Logger& log; //!< reference to logger
		neuron_param params;	///< hw neuron parameters struct	
	public:
		sc_port<anncore_pulse_if> out_port; ///< out_port for recording of spikes
		void handle_spike(const int& ,const sc_uint<9>&, const short& ,  int wta_id=0 ); ///< for doc see anncore_pulse_if
		void play_tx_event();	///< function to read input spikes from a file an inject them into the neuron

		SC_HAS_PROCESS(test_hw_neuron);
		/** Constructor
		 */
		test_hw_neuron(sc_module_name name);
		virtual ~test_hw_neuron();
};
#endif// _TEST_HW_NEURON_H_	
