#ifndef _dnc_merger_timed_h_
#define _dnc_merger_timed_h_
#include "merger_timed.h"
class spl1_merger;

/** class for the timed(clock-based) dnc merger.
 * works like the merger_timed, but:
 * the dnc_merger is not connected to another merger
 * but to the OUTPUT Register of the SPL1 Merger tree.
 * Hence, in method process_in2out() events
 * are directly forwarded to the SPL1 Output Register.
 */
class dnc_merger_timed: public merger_timed {
	public:
		/** constructor.*/
		dnc_merger_timed(
			sc_module_name name, //!< systemc name of this merger
			spl1_merger* output_register_if, //!< pointer to spl1_merger
			unsigned int output_register_id, //!< output register id of spl1_merger, to which this merger connects
			uint8_t PLL_period_ns
            );

		/** destructor.*/
		virtual ~dnc_merger_timed();
	private:
		/** processes the path from the input registers to the output register within this merger.
		 * it differs a little from the implementation in merger_timed:
		 * For the case that the output register is free, it checks if there is a valid event at one of the input registers
		 * and transmits this event to the OUTPUT Register of the SPL1 Merger tree.
		 * If merging is not enabled, only the selected input register is checked.
		 * If merging is enabled, the input that has been used the last time for the output register, is checked 
		 * after the other input register. This ensures that both inputs are served equally.
		 * If _slow is enabled, the checking of events at the input to output is blocked for one cycle, so that the output rate
		 * is halfened.
		 * is called maximally once per clock cycle.
		 * is called from process_empty_registers() in the case that out_register is empty.
		 */
		void process_in2out();

		spl1_merger* _output_register_if; //!< pointer to (parent) spl1_merger for sending spikes to output_registers(L2+L1)
		unsigned int _output_register_id; //!< the ID of the outputregister, to which this merger is connected (0..7)
};
#endif //_dnc_merger_timed_h_
