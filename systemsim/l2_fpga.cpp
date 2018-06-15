//_____________________________________________
// Company      :	TU-Dresden		      	
// Author       :	Stefan Scholze
// Refinement	:	Matthias Ehrlich
// E-Mail   	:	*@iee.et.tu-dresden.de
//
// Date         :   Thu Jan 18 12:22:48 2007				
// Last Change  :   mehrlich 03/20/2008			
// Module Name  :   l2_fpga					
// Filename     :   l2_fpga.cpp				
// Project Name	:   p_facets/s_systemsim			
// Description	:  		
//
//_____________________________________________

#include "l2_fpga.h"
#include "dnc_tx_fpga.h"

#include <iostream>
#include <sstream>

#include "lost_event_logger.h"
#include <log4cxx/logger.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("ESS.Layer2");

l2_fpga::l2_fpga (sc_module_name l2_fpga_i, int id, int wafer_id, const ESS::fpga_config & config)
	: id(id)
	, wafer_id(wafer_id)
    ,_record(config.record)
	,_stop(false)
	, clk("clk",CLK_PER_L2_FPGA,SC_NS)
	{
		for(size_t i=0;i<DNC_FPGA;i++)
		{
			std::stringstream buffer;
			buffer << "dnc_tx_fpga_i" << i;
			dnc_tx_fpga_i[i] = new dnc_tx_fpga(buffer.str().c_str(),NULL);
			dnc_tx_fpga_i[i]->set_side(dnc_tx_fpga::FPGA);
		}
		SC_METHOD(fifo_from_l2_ctrl);
		dont_initialize();
		sensitive << clk;
		
		SC_THREAD(play_tx_event);

		// apply fpga config
		setPlaybackPulses(config.playback_pulses);
	}

l2_fpga::~l2_fpga()
{ 
	for(unsigned int i=0;i<DNC_FPGA;i++)
	{
		delete dnc_tx_fpga_i[i];
	}
}

void l2_fpga::fifo_from_l2_ctrl()
{
	uint64 value;
	sc_uint<HYP_EVENT_WIDTH> out;
	unsigned char target;
	int i;

	for(i=0;i<DNC_FPGA;++i)
	{
		if(dnc_tx_fpga_i[i]->fifo_rx_event.num_available())
		{
			dnc_tx_fpga_i[i]->fifo_rx_event.nb_read(value);
			out = value;

			if(_record)
				record_rx_event(i,out);

			LostEventLogger::count_l2_fpga_fifo_from_l2_ctrl();
		} else if (dnc_tx_fpga_i[i]->fifo_rx_cfg.num_available())
		{
			dnc_tx_fpga_i[i]->fifo_rx_cfg.nb_read(value);
			dnc_tx_fpga_i[i]->fifo_rx_cfg_target.nb_read(target);
            LOG4CXX_TRACE(logger, name() << ":fifo_from_l2_ctrl received from FPGA=" << (int)id << " DNC=" << i << " HICANN=" << (int)target << " PACKET=" << std::hex << std::uppercase << value);
		}
	}
}


void l2_fpga::play_tx_event()
{
	static unsigned int num_times_warning_called = 0;
	for (auto entry : _playback_pulses )
	{
		ESS::playback_entry::delta_time_t wait_time = entry.delta_time;

		if (wait_time < 1 ) {
			wait_time = 1;
			if (num_times_warning_called < 5 ) {
                LOG4CXX_WARN(logger, name() << ":play_tx_event(): Pulses come to close, pulse is delayed by one FPGA CLK cycle ( 8 ns)");
				num_times_warning_called++;
				if (num_times_warning_called == 5 ){
                    LOG4CXX_WARN(logger, name() << ":play_tx_event(): Future warnings will be suppressed");
				}
			}
		}
		wait(wait_time*SYS_CLK_PER_L2_FPGA_NS,SC_NS);

		size_t dnc_id = entry.event.getDncAddress();

		/// we need to create an L2 Event:
		/// MSB | <5bit unused> | <12 label(without dnc)> | <15bit time stamp> | LSB
		uint32_t event = entry.event.getTime() % (1<<TIMESTAMP_WIDTH); // set time
		event |= (entry.event.getLabel() % (1<<L2_LABEL_WIDTH)) << TIMESTAMP_WIDTH; // crop dnc from label and shift to the right position
        
        //TODO understand syntax 
        LOG4CXX_TRACE(logger, name() << ":play_tx_event(): event started: " << event);
		dnc_tx_fpga_i[dnc_id]->start_event(event);
		LostEventLogger::count_fpga();
	}
}

void l2_fpga::record_rx_event(const int dnc_id, const sc_uint<HYP_EVENT_WIDTH>& event)
{
	ESS::trace_entry entry;
	entry.fpga_time = (uint) sc_simulation_time() / SYS_CLK_PER_L2_FPGA_NS; // FIXME: replace sc_simulation_time by sc_time_stamp

	uint16_t pulse_label = (dnc_id << L2_LABEL_WIDTH) | ((event.to_uint() >> TIMESTAMP_WIDTH) % (1<<L2_LABEL_WIDTH) );
	uint16_t time_stamp = event.to_uint() % (1<<TIMESTAMP_WIDTH);
    LOG4CXX_TRACE(logger, name() << ":record_rx_event(): pulse recorded with label " << pulse_label << " @ " << time_stamp << " ns");
	entry.event.setLabel(pulse_label);
	entry.event.setTime(time_stamp);
	_trace_pulses.push_back(entry);
	LostEventLogger::count_l2_fpga_record_rx_event();
}
