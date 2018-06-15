#include "systemc.h"
#include "tb_bg_event_generator.h"
#include "bg_event_generator.h"
#include "logger.h"
#include <string>

tb_bg_event_generator::tb_bg_event_generator():
	 _log(Logger::instance())
{
	bg_event_generator_i = new bg_event_generator("bg_event_generator_i", this);
	_spiketimes = std::vector<double>();
}

tb_bg_event_generator::~tb_bg_event_generator()
{
	delete bg_event_generator_i;
}

bool tb_bg_event_generator::nb_write_pulse(
	bool input, //!< input, either 0 or 1
	short nrn_adr //!< 6-bit neuron address
	) {
	write_pulse(input,nrn_adr);
	return true;
}
bool tb_bg_event_generator::write_pulse(
	bool input, //!< input, either 0 or 1
	short nrn_adr //!< 6-bit neuron address
	) {
	static_cast<void>(input);
	static_cast<void>(nrn_adr);
	_spiketimes.push_back( sc_time_stamp().to_seconds() );
	return true;
}


std::vector< double >& tb_bg_event_generator::spiketimes()
{
	return _spiketimes;
}
