#include "merger.h"
#include <log4cxx/logger.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("ESS.HICANN.Layer1");

merger::merger(std::string name)
	:_select(false)
	,_enable(false)
	,_slow(false)
	,_name(name)
{}

merger::merger()
	:_select(false)
	,_enable(false)
	,_slow(false)
{}

merger::~merger()
{}

void merger::rcv_pulse(
	bool input,
	short nrn_adr //!< 6-bit neuron adr
	) const
{
    LOG4CXX_TRACE(logger, "merger(" << _name << ")::rcv_pulse("<< input << ", " << nrn_adr << ") called");

	if( _enable  || input == _select) {
        LOG4CXX_TRACE(logger, "\t\tand forwarded to " <<  target_mergers.size() << " mergers");
		std::map< merger_pulse_if*, bool >::const_iterator it_tm = target_mergers.begin();
		std::map< merger_pulse_if*, bool>::const_iterator it_end = target_mergers.end();
		for ( ; it_tm != it_end; ++it_tm) {
			it_tm->first->rcv_pulse(it_tm->second,nrn_adr);
		}
	}
}

bool merger::write_pulse(
		bool input, //!< input, either 0 or 1
		short nrn_adr //!< 6-bit neuron address
		) {
	rcv_pulse(input,nrn_adr);
	return true;
}

bool merger::nb_write_pulse(
		bool input, //!< input, either 0 or 1
		short nrn_adr //!< 6-bit neuron address
		) {
	rcv_pulse(input,nrn_adr);
	return true;
}
bool merger::nb_read_pulse(
		short& /*nrn_adr*/ //!< 6-bit neuron address
		) {
	return false;
}
void merger::connect_input_to(
		merger* source_merger,
		bool input
		) {
	static_cast<void>(source_merger);
	static_cast<void>(input);
}

void merger::connect_output_to(
		merger* target_merger,
		bool input
		)
{
	// if not yet connect to that merger, add it
	if (target_mergers.find(target_merger) == target_mergers.end() )
		target_mergers[ target_merger ] = input;
}
