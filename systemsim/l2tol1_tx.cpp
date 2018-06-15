//_____________________________________________
// Company      :	TU-Dresden
// Author       :	Stefan Scholze
// Refined		: 	Matthias Ehrlich
// E-Mail   	:	scholze@iee.et.tu-dresden.de
//
// Date         :   Tue Apr 18 14:08:03 2007
// Last Change  :   mehrlich 03/20/2008
// Module Name  :   l2tol1_tx
// Filename     :   l2tol1_tx.cpp
// Project Name	:   p_facets/s_systemsim
// Description	:   gets data from l2, delays until time and release to l1 bus
//
//_____________________________________________

#include "l2tol1_tx.h"
#include "lost_event_logger.h"
#include <sstream>
#include <log4cxx/logger.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("ESS.Layer2");

/// Gets received data from dnc_if and adds it into buffers.
/// External interface!
/// @param data_in: event containing timestamp and neuron number
void l2tol1_tx::transmit(sc_uint <TIMESTAMP_WIDTH+L1_ADDR_WIDTH> data_in)
{
	if (_direction == TO_HICANN) {
		buffer = data_in;
		rx_data.notify(TIME_DES_PULSE,SC_NS);
		LostEventLogger::count_l2tol1_tx_transmit();
	}
	else {
        LOG4CXX_WARN(logger, name() << "::transmit() received event but direction is set TO_DNC");
	}
}

/// Adds received data into buffer. 
void l2tol1_tx::add_buffer()
{
	if (_direction == TO_HICANN) {
		sim_time = sc_simulation_time();	// sc_simulation_time returns time as a double in SC_NS(per default, comp. sc_get_default_time_unit
		uint current_clock_cycle = (uint)(sim_time/SYSTIME_PERIOD_NS) & 0x7fff;
		// check for expired events:
		uint rel_time = (buffer & 0x7fff);
		if ( ((rel_time - current_clock_cycle )>>(TIMESTAMP_WIDTH-1)) & 0x1 ) {
            LOG4CXX_TRACE(logger, name() << ":add_buffer(): EXPIRED EVENT: \tsim_time= " <<  sim_time
				<< "\t, current_clock_cycle=" << current_clock_cycle
				<< "\t, rel_time= " <<  rel_time
				<< "\t, delta=" << (int)(current_clock_cycle-rel_time));
			std::stringstream ss;
			ss << name() << " event expired";
			LostEventLogger::log(/*downwards*/ true, ss.str());
			return;
		}
		// check if event is already expired:
		//if ( ((buffer & 0x1fff) - current_clock_cycle )
		for(size_t i=0;i<DNC_IF_2_L2_BUFFERSIZE;i++)
		{
			if(memory[i].valid == false)
			{
				memory[i].valid = true;
				memory[i].value = buffer;
				/*
				_log(Logger::INFO) << "l2tol1_tx::add_buffer(): sim_time= " <<  sim_time 
					<< "\t, current_clock_cycle=" << current_clock_cycle
					<< "\t, rel_time= " <<  (memory[i].value & 0x7fff)
					<< "\t, delta=" << (int)(current_clock_cycle-(memory[i].value & 0x7fff))
					<< "\t, memory=" << i;
				*/
				LostEventLogger::count_l2tol1_tx_add_buffer();
				return;
			}
		}
		// if no space in memory left -> pulse is lost
		std::stringstream ss;
		ss << name() << " no input buffer free";
		LostEventLogger::log(/*downwards*/ true, ss.str());
	}
}

//****************************** Needs to be modified to handle correct timestamps
/// Compare recv time stamp with current sim time.
/// continously running process that compares 
/// received timestamps with current simulation time
/// reinvoked with help of clock
void l2tol1_tx::check_time()
{
	if (_direction == TO_HICANN) {
	// flag to unlock the output_lock.
	// it is set to false, if an event has been serialized
	bool unlock_at_end = true;

	if (!output_lock && out_buffer_valid ) {
		serialize(out_buffer);
		out_buffer_valid = false;
		unlock_at_end = false;
		output_lock = true;
	}

	sim_time = sc_simulation_time();	// sc_simulation_time returns time as a double in SC_NS(per default, comp. sc_get_default_time_unit
	uint current_clock_cycle = (uint)(sim_time/SYSTIME_PERIOD_NS) & 0x7fff;
	for(size_t j=0;j<DNC_IF_2_L2_BUFFERSIZE;++j)
	{
		if(memory[j].valid == true)
		{
			// only 10 last bit:
			if((memory[j].value & 0x3ff) == (current_clock_cycle & 0x3ff))
			// if( ((memory[j].value & 0x7fff) - (current_clock_cycle+1))>>(TIMESTAMP_WIDTH-1) & 0x1)
			// if((memory[j].value & 0x7fff) <= current_clock_cycle)
			{

				if( !output_lock ) {
					memory[j].valid = false;
					serialize(memory[j].value >> TIMESTAMP_WIDTH);
					LostEventLogger::count_l2tol1_tx_check_time();
					/*
					_log(Logger::INFO) << "l2tol1_tx::check_time():SERIALIZE  sim_time= " <<  sim_time 
						<< "\t, current_clock_cycle=" << current_clock_cycle
						<< "\t, rel_time= " <<  (memory[j].value & 0x7fff)
						<< "\t, delta=" << (int)(current_clock_cycle-(memory[j].value & 0x7fff))
						<< "\t, memory=" << j;
					*/

					output_lock = true;
					unlock_at_end = false;
				}
				// check for output_buffer
				else if (!out_buffer_valid) {
					memory[j].valid = false;
					out_buffer = (memory[j].value >> TIMESTAMP_WIDTH);
					out_buffer_valid = true;
					/*
					_log(Logger::INFO) << "l2tol1_tx::check_time():OUT_BUFFER sim_time= " <<  sim_time 
						<< "\t, current_clock_cycle=" << current_clock_cycle
						<< "\t, rel_time= " <<  (memory[j].value & 0x7fff)
						<< "\t, delta=" << (int)(current_clock_cycle-(memory[j].value & 0x7fff))
						<< "\t, memory=" << j;
					*/
					LostEventLogger::count_l2tol1_tx_check_time();
				}
				else {
					// event is expired -> drop it
					memory[j].valid = false;
                    LOG4CXX_DEBUG(logger, name() << ":check_time():DROP        sim_time= " <<  sim_time 
						<< "\t, current_clock_cycle=" << current_clock_cycle
						<< "\t, rel_time= " <<  (memory[j].value & 0x7fff)
						<< "\t, delta=" << (int)(current_clock_cycle-(memory[j].value & 0x7fff))
						<< "\t, memory=" << j);
					std::stringstream ss;
					ss << name() << " check_time(): neither output nor output buffer free";
					LostEventLogger::log(/*downwards*/ true, ss.str());
				}
			}
		}
	}
	if (unlock_at_end) {
		output_lock=false;
	}
	} // direction is TO_HICANN
}

/// Serializes event at release time.
/// @param neuron: Neuron number of event
void l2tol1_tx::serialize(sc_uint <L1_ADDR_WIDTH> neuron)
{
	if( l1bus_tx_if->rcv_pulse_from_dnc_if_channel(neuron, channel_id) ) {
		LostEventLogger::count_l2tol1_tx_serialize();
	} else {
		std::stringstream ss;
		ss << name() << " input of dnc_merger is not empty";
		LostEventLogger::log(/*downwards*/ true, ss.str());
	}
}

void l2tol1_tx::set_direction(enum l2tol1_tx::direction dir) {
	_direction = dir;
}
