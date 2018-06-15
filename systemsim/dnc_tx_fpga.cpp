//_____________________________________________
// Company      :	Tu-Dresden			      	
// Author       :	Stefan Scholze			
// Refinement	:	Matthias Ehrlich
// E-Mail   	:	*@iee.et.tu-dresden.de				
//								
// Date         :   Wed Jan 17 09:18:19 2007				
// Last Change  :   mehrlich 03/20/2008			
// Module Name  :   dnc_tx_fpga
// Filename     :   dnc_tx_fpga.cpp
// Project Name	:   p_facets/s_systemsim
// Description	:  		
//								
//_____________________________________________

#include "dnc_tx_fpga.h"
#include "lost_event_logger.h"

/// Function to get events from network.
void dnc_tx_fpga::receive_event (const sc_uint<HYP_EVENT_WIDTH>& rx_event)
{
    event = rx_event.to_uint64();
    receive_sc_event.notify();
	LostEventLogger::count_dnc_tx_fpga_receive_event(side==DNC);
}

void dnc_tx_fpga::receive_config(const sc_uint<DNC_TARGET_WIDTH>& cfg_addr, const sc_uint<L2_CFGPKT_WIDTH>& cfg_data)
{
    cfg_packet = cfg_data.to_uint64();
    cfg_packet_target = (unsigned char) cfg_addr.to_uint();
//	printf("in Fpga channel receive_config: %.8X %.8X @ %i\n",(cfg_packet >> 32)&0xffffffff,(cfg_packet)&0xffffffff,(uint)sc_simulation_time());
    receive_cfg_event.notify();
}

void dnc_tx_fpga::fill_fifo()
{
 	if(!fill_fifo_lock)
	{
    	fill_fifo_lock = true;
		event_save = event;
		write_fifo_event.notify(TIME_HYP_IN_PULSE,SC_NS);
		LostEventLogger::count_dnc_tx_fpga_fill_fifo(side==DNC);
	}
}

void dnc_tx_fpga::fill_cfg_fifo()
{
 	if(!fill_cfg_fifo_lock)
	{
    	fill_cfg_fifo_lock = true;
		cfg_packet_save = cfg_packet;
		cfg_packet_target_save = cfg_packet_target;
		write_fifo_cfg_event.notify(TIME_HYP_IN_CONFIG,SC_NS);
	}
}

void dnc_tx_fpga::write_fifo()
{
	if(fill_fifo_lock)
	{
		if(this->fifo_rx_event.nb_write(event_save))
			LostEventLogger::count_dnc_tx_fpga_write_fifo(side==DNC);
		else
			LostEventLogger::log(/*downwards*/ side==DNC, name());
    	fill_fifo_lock = false;
	}
}

void dnc_tx_fpga::write_cfg_fifo()
{
	if(fill_cfg_fifo_lock)
	{
		this->fifo_rx_cfg.nb_write(cfg_packet_save);
		this->fifo_rx_cfg_target.nb_write(cfg_packet_target_save);
    	fill_cfg_fifo_lock = false;
	}
}


/// Function to transmit events to FPGA.
void dnc_tx_fpga::transmit()
{
		if(this->fifo_tx_event.num_available() && !transmit_lock)
		{
			transmit_lock = true;
			fifo_tx_event.nb_read(value);
			transmit_event.notify(TIME_HYP_OUT_PULSE,SC_NS);
			LostEventLogger::count_dnc_tx_fpga_transmit(side==FPGA);
//			printf("in Fpga transmit pls: %.16X @ %i\n",value,(uint)sc_simulation_time());
		} else if(this->fifo_tx_cfg.num_available() && !transmit_lock)
		{
			transmit_lock = true;
			fifo_tx_cfg.nb_read(value);
			fifo_tx_cfg_target.nb_read(value_target);
			transmit_cfg.notify(TIME_HYP_OUT_CONFIG,SC_NS);
//			printf("in Fpga transmit cfg: %.8X %.8X @ %i\n",(value >> 32)&0xffffffff,(value)&0xffffffff, (uint)sc_simulation_time());
		}
}

void dnc_tx_fpga::transmit_start_event()
{
		if(transmit_lock)
		{
            l2_dncfpga_if->receive_event(value);
			transmit_lock = false;
			LostEventLogger::count_dnc_tx_fpga_transmit_start_event(side==FPGA);
		}
}

void dnc_tx_fpga::transmit_start_cfg()
{
		if(transmit_lock)
		{
            l2_dncfpga_if->receive_config(value_target ,value);
			transmit_lock = false;
		}
}

/// Function to externally write into tx_fifo.
void dnc_tx_fpga::start_event(const sc_uint<HYP_EVENT_WIDTH>& event){
//	printf("start_event dnc_tx_fpga packet: %.8X\n",event.to_uint());
	if(this->fifo_tx_event.nb_write(event.to_uint()))
		LostEventLogger::count_dnc_tx_fpga_start_event(side==FPGA);
	else
		LostEventLogger::log(/*downwards*/ side==FPGA, name());
}

void dnc_tx_fpga::start_cfg(const sc_uint<DNC_TARGET_WIDTH>& cfg_addr, const sc_uint<L2_CFGPKT_WIDTH>& cfg_data)
{
	uint64 value = cfg_data.to_uint64();
	unsigned char value_target = (unsigned char) cfg_addr.to_uint();
	//printf("in Fpga start cfg: target %i = %.8X %.8X @ %i\n",value_target,(value >> 32)&0xffffffff,(value)&0xffffffff,(uint)sc_simulation_time());
	this->fifo_tx_cfg.nb_write(value);
	this->fifo_tx_cfg_target.nb_write(value_target);
}

void dnc_tx_fpga::tx_instant_config(const ess::l2_packet_t& cfg_packet)
{
	this->l2_dncfpga_if->rx_instant_config(cfg_packet);
}

void dnc_tx_fpga::rx_instant_config(const ess::l2_packet_t& cfg_packet)
{
	if (dnc_access != NULL)
		this->dnc_access->instant_config(cfg_packet);
}

