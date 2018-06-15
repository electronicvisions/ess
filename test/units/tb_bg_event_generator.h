#ifndef _TB_BG_EVENT_GENERATOR_h__
#define _TB_BG_EVENT_GENERATOR_h__

#include "systemc.h"
#include "merger_pulse_if.h"
#include <vector>

class Logger;
class bg_event_generator;

/** class tb_bg_event_generator
 * serves as a testbench of a bg_event_generator
 */ 
class tb_bg_event_generator: public merger_pulse_if {

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

		/** Constructor.*/
		tb_bg_event_generator();

		~tb_bg_event_generator();
		std::vector< double >& spiketimes();
		bg_event_generator * bg_event_generator_i; ///< bg_event_generator instance
	protected:
		Logger& _log;
		std::vector< double > _spiketimes;
};
#endif // _TB_BG_EVENT_GENERATOR_h__
