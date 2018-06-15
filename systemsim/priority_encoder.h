#ifndef _PRIORITY_ENCODER_H_
#define _PRIORITY_ENCODER_H_

// always needed
#include "systemc.h"


// pre-declarations
class merger_pulse_if;

/** The Priority Encoder class.
 * implements a 64:6 Priority Encoder
 * From 64 binary inputs, it takes the lowest ID being 1 and generates
 * a 6-bit pulse with the address stored in the register associated
 * with this ID. This pulse is forwared in an over-writing fashion
 * to the background merger. This means that (previous) pulses
 * can be lost here.
 * The processing takes 2 clock cycles. The maximum output rate of the
 * priority encoder is 1 pulse per 2 clock cycles.
 */
class priority_encoder : public sc_module
{
public:
	/// SC_HAS_PROCESS needed for declaration of SC_METHOD if not using SC_CTOR
	SC_HAS_PROCESS(priority_encoder);

	/** constructor.*/
	priority_encoder(
		sc_module_name name, //!< SystemC Module name
		merger_pulse_if* bg_merger,    //!< the (background) merge, to which this generator is connected to.
		uint8_t PLL_period_ns
        );

	/** destructor */
	virtual~priority_encoder();

	/** receive event from a denmem, that is connected to input of this priority encoder.
	 * returns true, if input channel is still free, which means that this event is further processed.
	 * returns false otherwise, which means that this event is definitely lost!
	 */
	bool rcv_event(
			short input //!< input channel connected to the denmem that has spiked!
			);

	/** sets the neuron address, which is transmitted to the spl1_merger tree,
	 * if a spike has been report for the denmem connection to input input_channel.*/
	void set_neuron_address(
			short input_channel, //!< input channel
			short neuron_address //!< 6-bit neuron address (0..63)
			);


private:
	sc_clock _clock; //!< systemc clock

	/** checks for events (spikes) to be processed.
	 * If there is at least one spike at the 64-bit input register,
	 * the active input with the lowest id is taken and forwarded to the backgound merger.
	 * called at every positive edge of the clock.
	 */
	void check_for_events();

	std::vector<bool> _input_channel; //!< vector containing flags for all inputs. flag = true, if neuron of input has spiked. (Size 64)
	std::vector<short> _addresses;  //!< vector containing the programmable 6-bit address (0..63) for each input. (Size 64)
	merger_pulse_if* _bg_merger; //!< interface to the bg_merger, to which this PE is connected
	unsigned int _num_inputs_to_process;  //!< stores how many input flags are set to true. So that checking of all input is not needed if no signal is active.

	/** buffer for 6-bit event to be sent to background merger in next clock
	 * cycle.
	 * Used to mimick that processing takes 2 clock cycles and max out-rate
	 * is 1 / 2 clock cycles.
	 */
	short _event_buffer;

	/** flag indicating that event buffer is occupied*/
	bool _event_buffer_occupied;
};

#endif // _PRIORITY_ENCODER_H_
