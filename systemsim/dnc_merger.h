#ifndef _dnc_merger_h_
#define _dnc_merger_h_
#include <map>
#include "merger.h"

class spl1_merger;

/** the dnc merger class.
 * merges input from background or level mergers with input from the DNC Interface.
 * as there is no next merger but an output register, there is no map of target mergers
 * but an interface for sending to the output register.
 */
class dnc_merger: public merger {
	public:
		/** constructor.*/
		dnc_merger(
			std::string name, //!< name of this merger (for debugging)
			spl1_merger* output_register_if, //!< pointer to spl1_merger
			unsigned int output_register_id //!< output register id of spl1_merger, to which this merger connects
			);

		/** destructor.*/
		virtual ~dnc_merger();
		/** defined in class merger_pulse_if */
		virtual void rcv_pulse(
				bool input, //!< input, either 0 or 1
				short nrn_adr //!< 6-bit neuron adr
				) const;
		/** defined in class merger_config_if */
		virtual void connect_output_to(
				merger* target_merger,
				bool input
				);

	private:
		dnc_merger(); //!< default constructor, mustn't be used.
		spl1_merger* _output_register_if; //!< pointer to (parent) spl1_merger for sending spikes to output_registers(L2+L1)
		unsigned int _output_register_id; //!< the ID of the outputregister, to which this merger is connected (0..7)
};
#endif //_dnc_merger_h_
