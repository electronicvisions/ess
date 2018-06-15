#include "priority_encoder.h"
#include "sim_def.h"
#include "merger_pulse_if.h"
#include "lost_event_logger.h"
#include <log4cxx/logger.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("ESS.HICANN.Layer1");

priority_encoder::priority_encoder(
    sc_module_name name, merger_pulse_if* bg_merger, uint8_t PLL_period_ns)
    : sc_module(name),
      _clock("clock", PLL_period_ns, SC_NS),
      _bg_merger(bg_merger),
      _num_inputs_to_process(0),
      _event_buffer(0),
      _event_buffer_occupied(false)
{
	SC_METHOD(check_for_events);
	sensitive << _clock.posedge_event();

	_input_channel = std::vector<bool>(64,false);

	_addresses.reserve(64);
	// For now, we have a linear mapping of neuron ids to input slots.
	for(size_t n_ad = 0; n_ad < 64; ++n_ad)
		_addresses[n_ad] = n_ad;
}

priority_encoder::~priority_encoder(){}

bool priority_encoder::rcv_event(
			short input //!< input channel connected to the denmem that has spiked!
			){
	if (_input_channel.at(input) == false ){
        LOG4CXX_TRACE(logger, name() << "::priority_encoder::rcv_event(" << input << ") at time " << sc_time_stamp() );
		// mark in input channel, that neuron has spiked
		_input_channel[input] = true;
		_num_inputs_to_process++;
		// in the case of event-based implementation, trigger output generation
		// otherwise this is done automatically by the clock.
		LostEventLogger::count_priority_encoder_rcv_event();
		return true;
	} else {
		std::stringstream ss;
		ss << name() << "::rcv_event(" << input << ") EVENT LOST (channel occupied)";
		LostEventLogger::log_wafer(ss.str());
        LOG4CXX_WARN(logger, name() << "::priority_encoder::rcv_event(" << input << ") at time " << sc_time_stamp() << " EVENT LOST (channel occupied)" );
		return false;
	}
}

void priority_encoder::check_for_events()
{
	if (_event_buffer_occupied) {
		LOG4CXX_DEBUG(
		    logger, name() << "::priority_encoder::check_for_events() sends event at time "
		                   << sc_time_stamp() << " with addr " << _event_buffer);
		// note that, that an existing event in the input[0] of _bg_merger is overwritten here.
		if (_bg_merger->write_pulse(
		        0, _event_buffer)) { // priority_encoder is connected to input 0 of bg-merger
			LostEventLogger::count_priority_encoder_send_event();
		}
		else {
			std::stringstream ss;
			ss << name() << "::check_for_events() overwritten input of register in background merger.";
			LostEventLogger::log_wafer(ss.str());
		}
		_event_buffer_occupied = false;
	} else {
		// If there are inputs to handle
		if (_num_inputs_to_process > 0) {
			size_t n_in = 0;
			for (; n_in < 64; ++n_in)
				if (_input_channel[n_in])
					break;
			_event_buffer = _addresses[n_in];
			_event_buffer_occupied = true;

			_input_channel[n_in] = false;
			_num_inputs_to_process--;
		}
	}
}
void priority_encoder::set_neuron_address(
			short input_channel, //!< input channel
			short neuron_address //!< 6-bit neuron address (0..63)
			){
	_addresses.at(input_channel) = neuron_address;
}
