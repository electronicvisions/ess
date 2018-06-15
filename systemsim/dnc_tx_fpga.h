//_____________________________________________
// Company      :	Tu-Dresden
// Author       :	Stefan Scholze
// Refinement	:	Matthias Ehrlich
// E-Mail   	:	*@iee.et.tu-dresden.de
//
// Date         :   Wed Jan 17 09:18:19 2007
// Last Change  :   mehrlich 03/20/2008
// Module Name  :   dnc_tx_fpga
// Filename     :   dnc_tx_fpga.h
// Project Name	:   p_facets/s_systemsim
// Description	:   represents a DNC channel in the DNC
//
//_____________________________________________

#ifndef __DNC_TX_FPGA_H__
#define __DNC_TX_FPGA_H__

#include "systemc.h"
#include "sim_def.h"
#include "types.h"
#include "l2_hyp_task_if.h"
#include "l2_dnc.h"

/// This class represents a hypertransport channel between one DNC and one FPGA.
/// Each DNC has one of these channel, while each FPGA offers DNC_FPGA channel.
/// It implement l2_hyp_task_if for configuration and pulse event transmission
class dnc_tx_fpga : 	public sc_module,
                        public l2_hyp_task_if
{
public:
	enum SIDE{
		FPGA = 0,
		DNC = 1
	};
	void set_side(enum SIDE side_) { side = side_;}
private:
	// instantiations
	sc_clock clk;

	sc_fifo<uint64> fifo_tx_event;
	sc_fifo<uint64> fifo_tx_cfg;
	sc_fifo<unsigned char> fifo_tx_cfg_target;

	bool fill_fifo_lock;
	bool transmit_lock;

	bool fill_cfg_fifo_lock;

	bool side; // false = FPGA, true: DNC

	//data
	uint64 event;
	uint64 event_save;
	uint64 value;
	unsigned char value_target;

	uint64 cfg_packet;
	unsigned char cfg_packet_target;
	uint64 cfg_packet_save;
	unsigned char cfg_packet_target_save;
	uint64 cfg_value;

	sc_event receive_sc_event;
	sc_event write_fifo_event;
	sc_event receive_cfg_event;
	sc_event write_fifo_cfg_event;

	sc_event transmit_event;
	sc_event transmit_cfg;

	SC_HAS_PROCESS(dnc_tx_fpga);

	// function declarations

	void transmit();
	void transmit_start_event();
	void transmit_start_cfg();
	void fill_fifo();
	void write_fifo();
	void fill_cfg_fifo();
	void write_cfg_fifo();

public:

	// port instantiation
	sc_port< l2_hyp_task_if,1,SC_ZERO_OR_MORE_BOUND> l2_dncfpga_if;

	// instantiations
	sc_fifo<uint64> fifo_rx_event;
	sc_fifo<uint64> fifo_rx_cfg;
	sc_fifo<unsigned char> fifo_rx_cfg_target;

	l2_dnc *dnc_access;

	void start_event(const sc_uint<HYP_EVENT_WIDTH>&);
	void start_cfg(const sc_uint<DNC_TARGET_WIDTH>&, const sc_uint<L2_CFGPKT_WIDTH>&);
	void tx_instant_config(const ess::l2_packet_t&);  // configuration at time 0
	virtual void receive_config(const sc_uint<DNC_TARGET_WIDTH>&, const sc_uint<L2_CFGPKT_WIDTH>&); // interface for config packets
	virtual void receive_event(const sc_uint<HYP_EVENT_WIDTH>&); // interface for pule event packets
	virtual void rx_instant_config(const ess::l2_packet_t&);  // configuration at time 0

	// constructor

	dnc_tx_fpga (sc_module_name dnc_tx_fpga_i,l2_dnc *dnc_access)
		: clk("clk",CLK_PER_DNC_TX_FPGA,SC_NS)
		, fifo_tx_event(CHANNEL_HYP_SIZE_OUT)
		, fifo_tx_cfg(CHANNEL_HYP_SIZE_OUT)
		, fifo_tx_cfg_target(CHANNEL_HYP_SIZE_OUT)
		, fill_fifo_lock(false)
		, transmit_lock(false)
		// transaction streams and generators
		, fifo_rx_event(CHANNEL_HYP_SIZE_IN)
		, fifo_rx_cfg(CHANNEL_HYP_SIZE_IN)
		, fifo_rx_cfg_target(CHANNEL_HYP_SIZE_IN)
		, dnc_access(dnc_access)
	{
        static_cast<void>(dnc_tx_fpga_i);

		SC_METHOD(transmit);
		dont_initialize();
		sensitive_pos << clk;

		SC_METHOD(transmit_start_event);
		dont_initialize();
		sensitive << transmit_event;

		SC_METHOD(transmit_start_cfg);
		dont_initialize();
		sensitive << transmit_cfg;

		SC_METHOD(fill_fifo);
		dont_initialize();
		sensitive << receive_sc_event;

		SC_METHOD(fill_cfg_fifo);
		dont_initialize();
		sensitive << receive_cfg_event;

		SC_METHOD(write_fifo);
		dont_initialize();
		sensitive << write_fifo_event;

		SC_METHOD(write_cfg_fifo);
		dont_initialize();
		sensitive << write_fifo_cfg_event;
	}
};


#endif // __DNC_TX_FPGA_H__
