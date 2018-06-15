#ifndef _TB_PRIORITY_ENCODER_H__
#define _TB_PRIORITY_ENCODER_H__

#include "systemc.h"
#include "merger_pulse_if.h"

class Logger;
class priority_encoder;

/** class tb_priority_encoder
 * serves as a testbench of a priority_encoder
 */ 
class tb_priority_encoder: public sc_module , public merger_pulse_if {

	public:
		void rcv_pulse(
				bool input, //!< input, either 0 or 1
				short nrn_adr //!< 6-bit neuron adr
				) const {}

		/** functin defined in merger_pulse_if.h
		 * 	writes nrn_adr into input_register[input] if this is empty.
		 *  returns true, if register was empty, otherwise returns false.
		 */
		bool nb_write_pulse(
				bool input, //!< input, either 0 or 1
				short nrn_adr //!< 6-bit neuron address
				);

		bool write_pulse(
				bool input, //!< input, either 0 or 1
				short nrn_adr //!< 6-bit neuron address
				);

		bool nb_read_pulse(
				short& nrn_adr //!< 6-bit neuron address
				){ return false;}
		void play_tx_event();	///< function to read input spikes from a file an inject them into the neuron

		SC_HAS_PROCESS(tb_priority_encoder);
		/** Constructor.*/
		tb_priority_encoder(sc_module_name name);

		~tb_priority_encoder();
	protected:
		priority_encoder * priority_encoder_i; ///< priority_encoder instance
		FILE * pulse_file_rx; ///<
		Logger& _log;
};
#endif// _TB_PRIORITY_ENCODER_H__
