#include "dnc_merger_timed.h"
#include "spl1_merger.h"
#include <log4cxx/logger.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("ESS.HICANN.Layer1");

dnc_merger_timed::dnc_merger_timed(
		sc_module_name name,
		spl1_merger* output_register_if,
		unsigned int output_register_id,
        uint8_t PLL_period_ns
		): merger_timed(name, PLL_period_ns)
		,_output_register_if(output_register_if)
		,_output_register_id(output_register_id)
{
}

dnc_merger_timed::~dnc_merger_timed(){}

void dnc_merger_timed::process_in2out(){
	if(!_process_in2out_called) {
		if(!_slow_lock) {
			// in 2 out register (within one merger)
			if (!_out_occupied){
				bool found_suitable_in_register = false;
				bool in_register_id;
				if (_enable) { // merge: check for both inputs
					// check for the input that was NOT used before.
					if( _in_occupied[ !_last_in ]) {
						found_suitable_in_register = true;
						in_register_id = !_last_in;
					}
					if (!found_suitable_in_register && _in_occupied[_last_in] ) {
						found_suitable_in_register = true;
						in_register_id = _last_in;
					}
				} 
                else { // no merge: only check for one input
					if( _in_occupied[_select]) {
						found_suitable_in_register = true;
						in_register_id = _select;
					}
				}
				// if we found a suitable entry in the input register, pass it to the output register and trigger processing of free'd input register.
				if( found_suitable_in_register ) {
					_out_register = _in_register[ in_register_id];
                    LOG4CXX_TRACE(logger, _name << "::process_in2out(): at " << sc_time_stamp() << "  moving in[" << in_register_id << "] to out. Addr =" << _out_register);
					_output_register_if->send_pulse_to_output_register( _out_register, _output_register_id);
					//_out_occupied = true;
					_in_occupied[ in_register_id ] = false;
					_last_in = in_register_id; // for merger
					process_input(in_register_id);
					if (_slow)
						_slow_lock=true;
				}
			}
		} 
        else {
            LOG4CXX_TRACE(logger, _name << "::process_in2out()" << sc_time_stamp() << " resetting _slow_lock");
			_slow_lock=false;
		}
		_process_in2out_called=true;
	}
}

