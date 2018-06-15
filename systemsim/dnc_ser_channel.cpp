//_____________________________________________
// Company      :	Tu-Dresden			      	
// Author       :	Stefan Scholze			
// Refinement	:	Matthias Ehrlich
// E-Mail   	:	*@iee.et.tu-dresden.de
//
// Date         :   Wed Jan 17 09:18:19 2007				
// Last Change  :   mehrlich 03/20/2008			
// Module Name  :   dnc_ser_channel
// Filename     :   dnc_ser_channel.cpp
// Project Name	:   p_facets/s_systemsim
// Description	:   		
//								
//_____________________________________________

#include "dnc_ser_channel.h"
#include "lost_event_logger.h"

#include <log4cxx/logger.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("ESS.Layer2");

/// Function to get events from network.
/// This function implements l2_dnc_task_if.
/// It is accessed by sc_port of another dnc_ser_channel in a DNC or a DNC_IF.
/// It receives a pulse events and saves it temporarally. 
/// In parallel it generates an event for further event handling.
void dnc_ser_channel::receive_event (const sc_uint<L2_EVENT_WIDTH>& rx_event)
{
    event = rx_event.to_uint();
    receive_sc_event.notify();
	LostEventLogger::count_dnc_ser_channel_receive_event(side==DNC_IF);
    LOG4CXX_DEBUG(logger, name() << "::receive event: " << event << " @ " << (uint)sc_simulation_time() );
}

/// Function to get configuration from network.
/// This function implements l2_dnc_task_if. 
/// It is accessed by sc_port of another dnc_ser_channel in a DNC or a DNC_IF.
/// It receives a configuration packet and saves it temporarally. 
/// In parallel it generates an event for further event handling.
void dnc_ser_channel::receive_config(const sc_uint<L2_CFGPKT_WIDTH>& cfg_data)
{
    cfg_packet = cfg_data.to_uint64();
    receive_cfg.notify();
//	printf("in DNC receive cfg: %.8X @ %i\n",cfg_packet, (uint)sc_simulation_time());
//		cout << "in DNC receive cfg: " << cfg_packet << " @ " << sc_simulation_time() << endl;
}

/// Waits for deserialisation time and generates event for FIFO-write.
/// It handles pulse event FIFO write.
void dnc_ser_channel::fill_fifo()
{
	if(!fill_fifo_lock)
	{
		fill_fifo_lock = true;
    	event_save = event;
		write_fifo_event.notify(TIME_DES_PULSE,SC_NS);
		LostEventLogger::count_dnc_ser_channel_fill_fifo(side==DNC_IF);
	}
}

/// It writes pulse events to receive FIFO.
void dnc_ser_channel::write_fifo()
{
	if(fill_fifo_lock)
	{
		if(this->fifo_rx_event.nb_write(event_save))
			LostEventLogger::count_dnc_ser_channel_write_fifo(side==DNC_IF);
		else
			LostEventLogger::log(/*downwards*/ side==DNC_IF, name());
		fill_fifo_lock = false;
	}
}

///Waits for deserialisation time and generates event for FIFO-write.
///It handles config FIFO write.
void dnc_ser_channel::fill_cfg_fifo()
{
	if(!fill_fifo_lock)
	{
		fill_fifo_lock = true;
    	cfg_packet_save = cfg_packet;
		write_fifo_cfg.notify(TIME_DES_CONFIG,SC_NS);
//		printf("in DNC fill cfg fifo: %.8X @ %i\n",cfg_packet_save, (uint)sc_simulation_time());
//		cout << "in DNC fill cfg fifo: " << cfg_packet_save << " @ " << sc_simulation_time() << endl;
	}
}
/// Function to write configuration packet to receive FIFO
void dnc_ser_channel::write_cfg_fifo()
{
	if(fill_fifo_lock)
	{
		this->fifo_rx_cfg.nb_write(cfg_packet_save);
		fill_fifo_lock = false;
//		printf("in DNC write cfg fifo: @ %i\n", (uint)sc_simulation_time());
//		cout << "in DNC write cfg fifo: @ " << sc_simulation_time() << endl;
	}
}

/// Function to transmit events to network.
/// If data in tx_fifo and if SERDES time is reached.
/// Checks permanently the status of the transmit memories. 
/// If data is available in config or pulse event FIFO, 
/// it reads FIFO and generates event for transmission delayed by serialization time.
void dnc_ser_channel::transmit()
{
	uint data;
	if(this->fifo_tx_cfg.num_available() && !transmit_lock)
	{
		transmit_lock = true;
		fifo_tx_cfg.nb_read(value);
		transmit_cfg.notify(TIME_SER_CONFIG,SC_NS);
//		printf("in DNC transmit cfg: %.16X @ %i\n",value, (uint)sc_simulation_time());
	} else if(!this->heap_tx_mem.empty() && !transmit_lock)
	{
		transmit_lock = true;
		heap_tx_mem.get(data);
		value = (uint64)data;
		transmit_event.notify(TIME_SER_PULSE,SC_NS);
		LostEventLogger::count_dnc_ser_channel_transmit(side==DNC);
	}
}

/// Transmits pulse event by accessing TBV port. 
void dnc_ser_channel::transmit_start_event()
{
	if(transmit_lock)
	{
		l2_dnc_if->receive_event((uint)value);
		transmit_lock = false;
		LostEventLogger::count_dnc_ser_channel_transmit_start_event(side==DNC);
	}
}

/// Transmits configuration by accessing TBV port.
void dnc_ser_channel::transmit_start_cfg()
{
	if(transmit_lock)
	{
//			printf("in DNC transmit start cfg: %.8X %.8X @ %i\n",(value >> MEM_DATA_WIDTH)&0xffffffff,(value)&0xffffffff,(uint)sc_simulation_time());
		l2_dnc_if->receive_config(value);
		transmit_lock = false;
	}
}


/// Access for transmitter for pulse event transmission.
void dnc_ser_channel::start_event(const sc_uint<L2_EVENT_WIDTH>& event)
{
	if(this->heap_tx_mem.insert(event.to_uint()))
		LostEventLogger::count_dnc_ser_channel_start_event(side==DNC);
	else
		LostEventLogger::log(/*downwards*/ side==DNC, name());
}

/// Access for transmitter for configuration transmission.
void dnc_ser_channel::start_config(const sc_uint<L2_CFGPKT_WIDTH>& cfg_data)
{
	uint64 value = cfg_data;
//	cout << name() << ": cfg_data = " << cfg_data << endl;
//	printf("in DNC start cfg: %.8X %.8X @ %i\n",(value >> MEM_DATA_WIDTH)&0xffffffff,(value)&0xffffffff,(uint)sc_simulation_time());
	this->fifo_tx_cfg.nb_write(value);
}
