#ifndef _bg_event_generator_h_
#define _bg_event_generator_h_
// always needed
#include "systemc.h"

#include <stdint.h>


// pre-declaration
class merger_pulse_if;

/** The Background Event Generator class.
 * Models a Background event generators.
 * It can be set to either create random or regular spike-trains
 * Uses a 16-bit Galois Linear Feedback Shift Register(LFSR) to generate random times.
 */
class bg_event_generator: public sc_module
{
public:
	/** returns random flag */
	bool get_random() const { return _random;}
	/** returns 6-bit addr */
	short get_addr() const { return _addr;}
	
	/** return event period */
	uint16_t get_period() const {return _period;}

	//!< sets random if true, poisson event interval distribution
	void set_random(bool random) {_random = random;}
	void set_addr(short addr) {_addr=addr;}; //!< sets 6-bit l1-address
	void set_period(uint16_t period) { if (period>0) _period = period; else _period = 1u;} //!< sets event period in sysclock cycles (4ns)

	/** resets or disables event generator.
	 * if enable = 0, the event generator is disabled.
	 * if enable = 1, the lfsr will be initialized to the given seed and event generation is triggered.*/
	void reset_n(
			bool enable,   //!< enable/disable bg event generator
			uint16_t seed  //!< initialize LFSR to this seed, if enable=1
			);

	/** constructor.
	 */
	bg_event_generator(
		sc_module_name name, //!< SystemC Module name
		merger_pulse_if* bg_merger,    //!< the (background) merge, to which this generator is connected to.
		uint8_t PLL_period_ns
        );

	/** destructor. */
	virtual ~bg_event_generator(){}

	/// SC_HAS_PROCESS needed for declaration of SC_METHOD if not using SC_CTOR
	SC_HAS_PROCESS(bg_event_generator);

private:
	/** releases a scheduled spike.
	 * triggered by sc_event next_spike.
	 * call generate_spike() to schedule next spike */
	void release_spike();

	/** generates a spike in the future.
	 * depending on the config(random or not, period) calculates the time up to the next spike
	 * and schedules this event(next_spike.notify) */
	void generate_spike();
	merger_pulse_if*  _bg_merger;  //!< the background merger
	bool _random; //!< if true, poisson event interval distribution
	bool _can_fire; //!< background event generator can fire in next clock cycle. Corresponds to 'evout' in verilog code
	short _addr; //!< 6-bit l1-address
	uint16_t _lfsr; //!< state of the linear feedback shift register
	uint16_t _period; //!< event period in sysclock cycles (4ns) (not 0)
	sc_event next_spike; ///< triggers spike_out()
    uint8_t _PLL_period_ns;
};
#endif // _bg_event_generator_h_
