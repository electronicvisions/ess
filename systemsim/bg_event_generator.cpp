#include "bg_event_generator.h"
#include "merger_pulse_if.h"
#include "sim_def.h"
#include <bitset>

bg_event_generator::bg_event_generator(
        sc_module_name name,
        merger_pulse_if* bg_merger,
        uint8_t PLL_period_ns):
	sc_module(name)
	,_bg_merger(bg_merger)
	,_random(false)
	,_can_fire(false)
	,_addr(0)
	,_lfsr(1u)
	,_period(1u)
    ,_PLL_period_ns(PLL_period_ns)
{
	SC_METHOD(release_spike);
	dont_initialize();
	sensitive << next_spike;
}

void
bg_event_generator::reset_n(
	bool enable,   //!< enable/disable bg event generator
	uint16_t seed  //!< initialize LFSR to this seed, if enable=1
	) {
	// cancel next scheduled event
	next_spike.cancel();
	if (enable) {
		// seed mustn't be all 0!
		if (seed > 0)
			_lfsr=seed;
		else
			_lfsr=1u;
		generate_spike();
	}
}

void
bg_event_generator::release_spike() {
	// note that, that an existing event in the input[1] of _bg_merger is overwritten here.
	_bg_merger->write_pulse(1,_addr); // bg-generator is connected to input 1 of bg-merger
	generate_spike();
}

void
bg_event_generator::generate_spike() {
	unsigned cycles = 0;
	if ( _random ) {

		std::bitset<16> lfsr(_lfsr);

		bool fire_now = false;
		do {
			bool bit_0 = ((unsigned ) lfsr[15] + lfsr[13] + lfsr[12] + lfsr[10])%2;
			std::bitset<16> cp_lfsr = lfsr;
			for (size_t i = 1; i < 16; ++i)
				lfsr[i] = cp_lfsr[i-1];
			lfsr[0] = cp_lfsr[15] ^ cp_lfsr[13] ^ cp_lfsr[12] ^ cp_lfsr[10];
			_lfsr = lfsr.to_ulong();

			// do exactly what the verilog code does.
			// If LFSR bigger than period, we fire, if we can fire.
			// and always disable firing in the next cycle
			if (_lfsr > _period ) {
				fire_now = _can_fire;
				_can_fire = false;
			}
			else {
				_can_fire = true;
			}
			++cycles;
		} while(!fire_now);
	} else {
		cycles = _period+1; // in periodic mode, we fire one cycle after the period is equal to number of cycles.
	}
	next_spike.notify(cycles*_PLL_period_ns, SC_NS); // sys_clock_period = 5 ns
}

