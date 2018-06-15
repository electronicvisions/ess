#ifndef _merger_pulse_if_h_
#define _merger_pulse_if_h_

/** pulse interface for a single merger in the spl1-merger tree.
 * offers various methods for pulse communication 
 * between mergers.
 * It especially offers the two methods nb_read_pulse() and nb_write_pulse(),
 * which have the same effect as nb_write() / nb_read() for an sc_fifo.
 * however, please note: write_pulse() differs from write() of sc_fifo.
 */
class merger_pulse_if {
	public:
		/** recv pulse with address on input input. 
		 * for pure c++ implementation only*/
		virtual void rcv_pulse(
				bool input, //!< input, either 0 or 1
				short nrn_adr //!< 6-bit neuron adr
				) const = 0;

		/**  writing of pulse event to merger.
		 * 	writes nrn_adr into input_register[input].
		 * 	If the register is already occupied, the old value 
		 * 	will be overwritten by nrn_adr!
		 * 	returns true, if register was free, false otherwise
		 */
		virtual bool write_pulse(
				bool input, //!< input, either 0 or 1
				short nrn_adr //!< 6-bit neuron address
				) = 0;

		/** non-blocking writing of pulse event to merger.
		 * 	writes nrn_adr into input_register[input] if this is empty.
		 *  returns true, if register was empty, otherwise returns false.
		 */
		virtual bool nb_write_pulse(
				bool input, //!< input, either 0 or 1
				short nrn_adr //!< 6-bit neuron address
				) = 0;

		/** non-blocking reading of pulse event from merger.
		 * 	reads nrn_adr from output_register, if it is full;
		 *  returns true, if register was full, otherwise returns false.
		 */
		virtual bool nb_read_pulse(
				short& nrn_adr //!< 6-bit neuron address
				) = 0;
};
#endif //_merger_pulse_if_h_
