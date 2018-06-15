#include "merger_timed.h"
#include "sim_def.h"
#include <log4cxx/logger.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("ESS.HICANN.Layer1");

merger_timed::merger_timed(
		sc_module_name name,
	    uint8_t PLL_period_ns
		): sc_module(name)
		, merger((const char *) name)
		, _clock("_clock", PLL_period_ns, SC_NS)
		,_out_register(-1)
		,_out_occupied(0)
		,_last_in(1)
		,_slow_lock(0)

{
	_in_source_merger[0]=NULL;
	_in_source_merger[1]=NULL;
	_in_register[0] = -1;
	_in_register[1] = -1;
	_in_occupied[0] = 0;
	_in_occupied[1] = 0;
	_process_input_called[0] = false;
	_process_input_called[1] = false;
	_process_in2out_called = false;

	SC_METHOD(process_empty_registers);
	dont_initialize();
	sensitive << _clock.posedge_event();

	SC_METHOD(reset_function_called_flags);
	dont_initialize();
	sensitive << _clock.negedge_event();
}

merger_timed::~merger_timed(){}

void merger_timed::process_in2out(){
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
				} else { // no merge: only check for one input
					if( _in_occupied[_select]) {
						found_suitable_in_register = true;
						in_register_id = _select;
					}
				}
				// if we found a suitable entry in the input register, pass it to the output register and trigger processing of free'd input register.
				if( found_suitable_in_register ) {
					_out_register = _in_register[ in_register_id];
                    LOG4CXX_TRACE(logger, _name << "::process_in2out(): at " << sc_time_stamp() << "  moving in[" << in_register_id << "] to out. Addr =" << _out_register);
					_out_occupied = true;
					_in_occupied[ in_register_id ] = false;
					_last_in = in_register_id; // for merger
					process_input(in_register_id);
					if (_slow)
						_slow_lock=true;
				}
			}
		} else {
            LOG4CXX_TRACE(logger, _name << "::process_in2out()" << sc_time_stamp() << " resetting _slow_lock");
			_slow_lock=false;
		}
		_process_in2out_called=true;
	}
}

void merger_timed::connect_input_to(
		merger* source_merger,
		bool input
		) {
	_in_source_merger[input] = source_merger;
}

void merger_timed::connect_output_to(
		merger* target_merger,
		bool input
		)
{
	target_merger->connect_input_to(this,input);
}

bool merger_timed::nb_read_pulse(
		short& nrn_adr //!< 6-bit neuron address
		){
	if (_out_occupied && !_process_in2out_called) { // the second condition make sure, that we not read the pulse that was written in this cycle.
		nrn_adr = _out_register;
		_out_register = -1;
		_out_occupied = false;
        LOG4CXX_TRACE(logger, _name << "::nb_read_pulse(" << nrn_adr << ") " << sc_time_stamp() );
		process_in2out();
		return true;
	}
	else {
		return false;
	}
}

bool merger_timed::write_pulse(
		bool input, //!< input, either 0 or 1
		short nrn_adr //!< 6-bit neuron address
		) {
	// write only possible is merging enabled or we are on the chose input
	// This is not really necessary, as it can be handled in process_in2out
	if(_enable || (input==_select)) {
		if (_in_occupied[input]) {
            LOG4CXX_TRACE(logger, _name << "::write_pulse(" << (int) input << ", " << nrn_adr << ") at:"
				<< sc_time_stamp() << "\t overwriting input register: addr " << _in_register[input] << " lost" );
			_in_register[input] = nrn_adr;
			return false;
		}
		else {
            _in_register[input] = nrn_adr;
			_in_occupied[input] = true;
			return true;
		}
	}
	return false;
}

bool merger_timed::nb_write_pulse(
		bool input, //!< input, either 0 or 1
		short nrn_adr //!< 6-bit neuron address
		) {
	// write only possible is merging enabled or we are on the chose input
	// This is not really necessary, as it can be handled in process_in2out
	if(_enable || (input==_select)) {
		if (_in_occupied[input]) {
			return false;
		} 
        else {
            LOG4CXX_TRACE(logger, _name <<  "::nb_write_pulse: writing pulse to neuron: " << nrn_adr);
            _in_occupied[input] = true;
			_in_register[input] = nrn_adr;
			return true;
		}
	} 
    else {
		return false;
	}
}

void merger_timed::rcv_pulse(
	bool input,
	short nrn_adr //!< 6-bit neuron adr
	) const
{
    LOG4CXX_WARN(logger, _name << "merger_timed::rcv_pulse() called, which has no effect! Use nb_write_pulse() instead");
	static_cast<void>(input);
	static_cast<void>(nrn_adr);
}

void merger_timed::process_empty_registers(){
	// output
	if ( !_out_occupied )
		process_in2out();

	if(_enable){
		// input
		for (size_t n = 0; n< 2; ++n){
			// if there is something on the output register connected to a free input register from this merger, get it.
			if ( !_in_occupied[n] ){
				process_input(n);
			}
		}
	} else {
		if ( !_in_occupied[_select] ){
			process_input(_select);
		}
	}
}

void merger_timed::process_input(bool input){
	if(!_process_input_called[input]) {
		short addr;
		if((_in_source_merger[input]!=NULL) && (_in_source_merger[input]->nb_read_pulse(addr))){
            LOG4CXX_TRACE(logger, _name << "::process_input(" << (int)input << " ) " << sc_time_stamp() << "new addr = " << addr );
			_in_register[input] = addr;
			_in_occupied[input] = true;
		}
		_process_input_called[input] = true;
	}
}

void merger_timed::reset_function_called_flags(){
	// reset changed flags
	_process_input_called[0] = false;
	_process_input_called[1] = false;
	_process_in2out_called = false;
}
