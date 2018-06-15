//_____________________________________________
// Company      :	Tu-Dresden
// Author       :	Stefan Scholze
// Refinement	:	Matthias Ehrlich
// E-Mail   	:	*@iee.et.tu-dresden.de
//
// Date         :   Wed Jan 17 09:18:19 2007
// Last Change  :   mehrlich 03/20/2008
// Module Name  :   dnc_ser_channel
// Filename     :   dnc_ser_channel.h
// Project Name	:   p_facets/s_systemsim
// Description	:   represents a DNC channel in the DNC
//
//_____________________________________________

#ifndef __DNC_SER_CHANNEL_H__
#define __DNC_SER_CHANNEL_H__

// always needed
#include "systemc.h"

// defines and helpers
#include "sim_def.h"
//#include "heap_mem.h"
#include "sized_queue.h"

// functional units
#include "l2_dnc_task_if.h"

/// This class provides a DNC channel for the DNC and the DNC_IF.
/// It offers transmission of config and pulse packets.
class dnc_ser_channel : 	
	public sc_module,
	public l2_dnc_task_if
{
public:
	enum SIDE {
		DNC = 0,
		DNC_IF = 1
	};
	SIDE side;
	void set_side(enum SIDE side_){side=side_;}

	sc_clock clk;
	//heap_mem<uint> heap_tx_mem; 	///< transmit heap mem for pulse events
    ess::sized_queue<uint> heap_tx_mem; 	///< transmit heap mem for pulse events
	sc_fifo<uint> fifo_rx_event; 	///< receive FIFO for pulse events
	sc_fifo<uint64> fifo_rx_cfg;	///< receive FIFO for configuration
	sc_fifo<uint64> fifo_tx_cfg;	///< transmit FIFO for configuration

	bool fill_fifo_lock;
	bool transmit_lock;


	// port instantiation
	sc_port< l2_dnc_task_if,1,SC_ZERO_OR_MORE_BOUND> l2_dnc_if;

	//data
	uint64 value;
	uint event;
	uint event_save;
	uint64 cfg_packet;
	uint64 cfg_packet_save;

	//events
	sc_event receive_sc_event;
	sc_event receive_cfg;
	sc_event write_fifo_event;
	sc_event write_fifo_cfg;
	sc_event transmit_event;
	sc_event transmit_cfg;

	SC_HAS_PROCESS(dnc_ser_channel);

	// constructor

	dnc_ser_channel (sc_module_name dnc_ser_channel_i)
		: clk("clk",CLK_PER_DNC_SER_CHANNEL,SC_NS)
		, heap_tx_mem(CHANNEL_MEM_SIZE_OUT)
		, fifo_rx_event(CHANNEL_MEM_SIZE_IN)
		, fifo_rx_cfg(CHANNEL_MEM_SIZE_IN)
		, fifo_tx_cfg(CHANNEL_MEM_SIZE_OUT)
		, fill_fifo_lock(false)
		, transmit_lock(false)
		// transaction streams and generators
	{
		static_cast<void>(dnc_ser_channel_i);

        SC_METHOD(transmit);
		dont_initialize();
		sensitive_pos << clk;

		SC_METHOD(fill_fifo);
		dont_initialize();
		sensitive << receive_sc_event;

		SC_METHOD(fill_cfg_fifo);
		dont_initialize();
		sensitive << receive_cfg;

		SC_METHOD(write_fifo);
		dont_initialize();
		sensitive << write_fifo_event;

		SC_METHOD(write_cfg_fifo);
		dont_initialize();
		sensitive << write_fifo_cfg;

		SC_METHOD(transmit_start_event);
		dont_initialize();
		sensitive << transmit_event;

		SC_METHOD(transmit_start_cfg);
		dont_initialize();
		sensitive << transmit_cfg;

	}

	void start_event(const sc_uint<L2_EVENT_WIDTH>&);
	void start_config(const sc_uint<L2_CFGPKT_WIDTH>&);
	void transmit_start_event();
	void transmit_start_cfg();
	void transmit();
	bool can_transmit() const{ return !transmit_lock;}
	void fill_fifo();
	void write_fifo();
	void fill_cfg_fifo();
	void write_cfg_fifo();


	virtual void receive_event(const sc_uint<L2_EVENT_WIDTH>&);
	/// interface for config packets.
	virtual void receive_config(const sc_uint<L2_CFGPKT_WIDTH>&);

	void test(){printf("SUccess\n");}

};

#endif // __DNC_SER_CHANNEL_H__
