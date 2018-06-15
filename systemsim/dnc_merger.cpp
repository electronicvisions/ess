#include "dnc_merger.h"
#include "spl1_merger.h"
#include <log4cxx/logger.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("ESS.HICANN.Layer1");

dnc_merger::dnc_merger(
				std::string name,
				spl1_merger* output_register_if,
				unsigned int output_register_id
				)
	:merger(name)
	,_output_register_if(output_register_if)
	,_output_register_id(output_register_id)
{}

dnc_merger::dnc_merger()
	:merger()
	,_output_register_if(NULL)
	,_output_register_id(0)
{}

dnc_merger::~dnc_merger()
{}

void dnc_merger::connect_output_to(
		merger* target_merger,
		bool input
		){
    LOG4CXX_WARN(logger, "dnc_merger::connect_output_to() has no effect");
	static_cast<void>(target_merger);
	static_cast<void>(input);
}

void dnc_merger::rcv_pulse(
	bool input,
	short nrn_adr //!< 6-bit neuron adr
	) const
{
    LOG4CXX_TRACE(logger, "dnc_merger(" << _name << ")::rcv_pulse("<< input << ", " << nrn_adr << ") called");
	if( _enable  || input == _select) {
        LOG4CXX_TRACE(logger, " and forwarded to outputregister ");
		_output_register_if->send_pulse_to_output_register( nrn_adr, _output_register_id);
	}
}
