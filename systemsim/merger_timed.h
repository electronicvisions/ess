#ifndef _merger_timed_h_
#define _merger_timed_h_
#include "systemc.h"
#include "merger.h"
#include <map>
#include <string>

/** class merger_timed.
 * the timed (clock-based) verson of a single merger in the spl1 merger tree.
 * works the following way:
 * Follows the following specification:
 * - maximum output rate is 250 MHz for slow=0 and 125 Hz for slow=1.
 * - 2 clk cycles are required to pass through one merger. (in slow=0 mode)
 * - input register may be overwritten by dnc_if, priority encoder or bg_merger.
 * - If merging is enabled, an event from one input register always precedes an event
 *   from the other input register. (modelled with _last_in)
 * - An event can move from one register to another register within one cycle
 *   if either the target register is already free, or if it is freed within this cycle.
 * - An event can maximally move one register per clock!
 * Implementation:
 * At every positive edge of the clock:
 * - all empty registers check if there is an event available at their preceding register.
 *   (the preceding merger are either the 2 input register for the output register; or the connected
 *   in-connected ouput-register of the preceding merger( if it exists).
 *   This check is triggered in method process_empty_registers().
 *   If there is an event at the preceding register, the event is moved to the next register.
 *   At the preceding register, the same check for its preceding register is triggered.
 *   This is done with methods:
 *   	- process_in2out()
 *   	- nb_read_pulse()
 *   	- process_input(bool input)
 *   To avoid the moving of events over more than 1 register per cycle, each method is called
 *   maximally once per cycle.
 * - Example: a merger chain, where each merger is connected to the selected input of the next merger.
 *   Imagine that all registers are occupied, only the output_register of the tip of the chain is free.
 *   At the positive clk edge, this free register triggers the moving of all event (entries) one step
 *   forward in the chain within one clk cycle, where each register, after it has been read, reads from 
 *   its preceding register, and so on.
 * At every negative edge of the clock:
 * - The flags, that the methods have been called, are reset. (We have to do this at a different point in time,
 *   otherwise problems may arise, as it is not specified, in which order systemc modules are processed)
 */
class merger_timed : public sc_module, public merger {
	public:
		// register this systemc module to have processes.
		SC_HAS_PROCESS(merger_timed);

		/** constructor.*/
		merger_timed(
				sc_module_name name, //!< systemc module name
				uint8_t PLL_period_ns
                );

		/** destructor.*/
		virtual ~merger_timed();

		void rcv_pulse(
				bool input, //!< input, either 0 or 1
				short nrn_adr //!< 6-bit neuron adr
				) const;

		/** defined in class merger_pulse_if */
		virtual bool write_pulse(
				bool input, //!< input, either 0 or 1
				short nrn_adr //!< 6-bit neuron address
				);

		virtual bool nb_write_pulse(
				bool input, //!< input, either 0 or 1
				short nrn_adr //!< 6-bit neuron address
				);
		virtual bool nb_read_pulse(
				short& nrn_adr //!< 6-bit neuron address
				);
		virtual void connect_output_to(
				merger* target_merger,
				bool input
				);
		virtual void connect_input_to(
				merger* source_merger,
				bool input
				);

	protected:
		/** processes the path from the input registers to the output register within this merger.
		 * For the case that the output register is free, it checks if there is a valid event at one of the input registers
		 * and moves it to the ouput register.
		 * If merging is not enabled, only the selected input register is checked.
		 * If merging is enabled, the input that has been used the last time for the output register, is checked 
		 * after the other input register. This ensures that both inputs are served equally.
		 * If _slow is enabled, the moving of events from the input to output is blocked for one cycle, so that the output rate
		 * is halfened.
		 * is called maximally once per clock cycle.
		 * is called from process_empty_registers() in the case that out_register is empty or from nb_read_pulse, when a pulse was read from the OR.
		 */
		virtual void process_in2out();

		/** processes empty registers (in and out).
		 * registers that are empty, can be filled with valid entries from their predecessor.
		 * calls process_in2out if out_register is empty.
		 * calls process_input(n) if in_register[n] is empty, but only if this input is active via _select or _enable.
		 * is called at every positive edge of the clock.
		 */
		virtual void process_empty_registers();

		/** resets the flags that methods have been called in this cycle.
		 * is called at every negative edge of the clock.
		 */
		virtual void reset_function_called_flags();

		/** processes the input register[input].
		 * checks if there is a new signal at the output of the connected source merger 
		 * and updates itself with this value.
		 * is called from either process_empty_registers() or process_in2out() , but only if
		 * this in_register is not occupied or if it has been read recently.
		 */
		void process_input(bool input);

		sc_clock _clock; //!< systemc clock

		merger_pulse_if* _in_source_merger[2]; //!< interface to the sources mergers of the input registers
		short _in_register[2]; //!< stores the current event (neuron addr) in the input registers
		short _out_register;  //!< stores the current event (neuron addr) in the output register
		bool _in_occupied[2]; //!< stores whether input register[i] is occupied: true->occupied
		bool _out_occupied; //!< stores whether output register is occupied: true->occupied
		bool _last_in; //!< stores which input register has been read last
		bool _slow_lock; //!< locks the enabling of an out register for 1 cycle (for _slow = true)
		bool _process_input_called[2]; //!< flag indicating, whether this register has changed during this cycle.
		bool _process_in2out_called; //!< flag indicating, whether this register has changed during this cycle.
};
#endif //_merger_timed_h_
