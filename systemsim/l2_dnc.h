//_____________________________________________
// Company      :	Tu-Dresden
// Author       :	Stefan Scholze
// Refinement	:	Matthias Ehrlich
// E-Mail   	:	*@iee.et.tu-dresden.de
//
// Date         :   Wed Jan 17 09:18:19 2007
// Last Change  :   mehrlich 03/20/2008
// Module Name  :   l2_dnc
// Filename     :   l2_dnc.h
// Project Name	:   p_facets/s_systemsim
// Description	:   represents a DNC chip on the PCB
//
//_____________________________________________

#ifndef _L2_DNC_H_
#define _L2_DNC_H_

// always needed
#include "systemc.h"

#include <array>
#include <bitset>

// defines and helpers
#include "sim_def.h"
#include "types.h"
#include "generic_array.h"

// functional units
#include "dnc_ser_channel.h"
#include "sized_queue.h"
#include "HAL2ESSContainer.h"

//forward declaration
class dnc_tx_fpga;

/// Represents a layer2 DNC chip on the PCB.
class l2_dnc :
	public sc_module
{
	short id;
	char wafer;

	sc_clock clk;

	//Routing table
    ess::generic_array<unsigned int, DNC_TO_ANC_COUNT, L2_LABEL_COUNT, 1> routing_mem; ///< routing information memory	
	//Delay memory downto ANC
	// heap_mem<uint> delay_mem[DNC_TO_ANC_COUNT]; ///< delay memory for pulse vents
    ess::sized_queue<uint> delay_mem[DNC_TO_ANC_COUNT]; ///< delay memory for pulse vents

	/// holds the direction for all hicanns and all dnc if channels on it.
	/// 0: TO_DNC
	/// 1: TO_HICANN
	/// numbering: 
	/// BIT     | 0 ............7 | 8.............15|16 ...................... 63 |
	/// HICANN  | 0               | 1               | 2               | ...
	/// CHANNEL | 0 1 2 3 4 5 6 7 | 0 1 2 3 4 5 6 7 | 0 1 2 3 4 5 6 7 | ...
	/// where HICANN is as seen from the DNC, i.e. NOT equal to HMF::Coordinate::HICANNOnDNC.id()
	/// and the CHANNEL is the dnc if CHANNEL in hardware numbering: 7 - GbitLinkOnHICANN()
	sc_uint<64> l2direction;

	/// holds for each hicann channel the time limits, which effect the release time of an event to the hicann.
	/// size: 8
	/// inner values: range from 0 .. 1023
	std::array< sc_uint<10>, DNC_TO_ANC_COUNT >time_limits;

public:
	dnc_ser_channel *dnc_channel_i[DNC_TO_ANC_COUNT];	///< serial communication channels
	dnc_tx_fpga *dnc_tx_fpga_i;						///< FPGA communication channel

	SC_HAS_PROCESS(l2_dnc);

	l2_dnc (sc_module_name l2_dnc_i,char wafer,short id, const ESS::dnc_config & config);
	~l2_dnc();

	/// Function to continously read from HICANN receive fifo and init L2 transfer.
	void fifo_from_anc_ctrl();

	/// Function to continously read from fpga receive fifo and init L2 transfer to HICANN.
	void fifo_from_fpga_ctrl();

	/// Function check delay mem for next pulse packet, i.e. whether it shall be releases now to HICANN
	void delay_mem_ctrl();

	/// Function to transmit events from HICANN.
	void transmit_from_anc(const sc_uint<L2_EVENT_WIDTH>&, int);

	/// Function to transmit events from FPGA.
	void transmit_from_fpga(const sc_uint<HYP_EVENT_WIDTH>&);

	/// Function that adds pulses to delay memory to be sent to the HICANN
	void transmit_to_anc(const sc_uint<L2_EVENT_WIDTH>&, int);

	/// Function to forward HICANN cfg packet data.
	void config_from_anc(int i,uint64 cfg_data);

	/// Function to fill routing memory and forward HICANN configuration.
	void config_from_fpga(unsigned char target,uint64 cfg_data);

	///evaluates configuration packets and configures DNC 
    void instant_config(const ess::l2_packet_t& cfg_packet);

	/// sets the time limits for all hicann channels , which effect the release time of an event to the hicann.
	void set_time_limits(const std::array< sc_uint<10>, DNC_TO_ANC_COUNT > & limits);

	/// sets the direction for all hicanns and all dnc if channels on it.
	/// see member l2direction for details
	void set_hicann_directions(const std::bitset<64> & hicann_directions);
};


#endif // _L2_DNC_H_
