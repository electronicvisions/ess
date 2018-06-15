#ifndef _merger_h_
#define _merger_h_
#include <map>
#include <string>
#include "merger_pulse_if.h"
#include "merger_config_if.h"

/** the merger base class.
 * the class implementing as single merger of spl1 events.
 * It has 2 inputs and one output.
 * The output may be connected to several targets.
 * implements the merger_pulse_if in a non-timed way (transaction-based)
 * i.e. this is a fast version. no spikes can be lost here.
 */
class merger : public merger_pulse_if, public merger_config_if{
	public:
		/** named constructor.*/
		merger(
			std::string name //!< name of this merger (for debugging)
            );

		/** destructor.*/
		virtual ~merger();

		/** defined in class merger_pulse_if */
		virtual void rcv_pulse(
				bool input, //!< input, either 0 or 1
				short nrn_adr //!< 6-bit neuron adr
				) const;

		/** defined in class merger_pulse_if */
		virtual bool write_pulse(
				bool input, //!< input, either 0 or 1
				short nrn_adr //!< 6-bit neuron address
				);

		/** defined in class merger_pulse_if */
		virtual bool nb_write_pulse(
				bool input, //!< input, either 0 or 1
				short nrn_adr //!< 6-bit neuron address
				);

		/** defined in class merger_pulse_if */
		virtual bool nb_read_pulse(
				short& nrn_adr //!< 6-bit neuron address
				);

		/** defined in class merger_config_if */
		virtual void connect_output_to(
				merger* target_merger,
				bool input
				);

		/** defined in class merger_config_if */
		virtual void connect_input_to(
				merger* source_merger,
				bool input
				);

		/** defined in class merger_config_if */
		void set_select(bool select){ _select=select;}

		/** defined in class merger_config_if */
		void set_enable(bool enable){ _enable=enable;}

		/** defined in class merger_config_if */
		void set_slow(bool slow){_slow=slow;}

		bool get_enable() const
		{return _enable;}

		bool get_select() const
		{return _select;}
	protected:
		merger(); //!< default constructor, shouldnt be used.
		bool _select; //!< if the input, if _enable=0
		bool _enable; //!< activates merger function
		bool _slow; //!< sets output rate to slow (clk/2)
		std::string _name; //!< name of this merger

	private:
		/** list of target mergers and corresponding inputs.
		 *  i.e. the mergers to which the output of this merger is connected
		 *  and the input at the target merger.
		 */
		std::map< merger_pulse_if*, bool > target_mergers;
};
#endif //_merger_h_
