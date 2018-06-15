//_____________________________________________
// Company      :	tud
// Author       :	Stefan Scholze
// Refinement	:	Matthias Ehrlich
// E-Mail   	:	*@iee.et.tu-dresden.de
//
// Date         :   Thu Dec 14 14:10:13 2006
// Last Change  :   mehrlich 03/20/2008
// Module Name  :   dnc_if
// Filename     :   dnc_if.h
// Project Name	:   p_facets/s_systemsim
// Description	:   
//
//_____________________________________________

#ifndef _DNC_IF_H_
#define _DNC_IF_H_

// always needed
#include "systemc.h"

#include "stdio.h"

// defines and helpers
#include "sim_def.h"
#include "types.h"
#include "string_builder.h"
#include "jf_utils.h"
#include "logger.h"

// functional units
#include "dnc_ser_channel.h"
#include "dnc_if_task_if.h"
#include "l2tol1_tx.h"

/// This class provides the DNC interface functionality.
class dnc_if :
		public dnc_if_task_if,
		public sc_module
{
public:
	enum direction {
		TO_DNC = 0,
		TO_HICANN= 1
	};
	short hicannid; ///< identification number of hicann
	sc_clock clk;

	direction l1direction[DNCL1BUSINCOUNT]; ///< direction of channels TO_DNC, TO_HICANN
	bool enable[DNCL1BUSINCOUNT]; ///< enable of channels: true means enabled


	dnc_ser_channel *dnc_channel_i; ///< layer2 communication channel

	l2tol1_tx *l2tol1_tx_i[DNCL1BUSINCOUNT]; ///< l2tol1_tx instance
	double sim_time; ///< stores simulation time
	uint64 nrnr;	///< logical neuron number
	short nrnid;	///<physical neuron number
	uint64 value_cfg;

	// sc_event pulse_event;
	// sc_event cfg_packet;

	SC_HAS_PROCESS(dnc_if); ///< SystemC macro to allow event sensitive methods

	///constructor for dnc_if
	dnc_if(sc_module_name dnc_if_i, short hicannid)
		: sc_module(dnc_if_i)
		, hicannid(hicannid)
		, clk("clk",CLK_PER_DNC_IF,SC_NS),
		log(Logger::instance())

	{
		for (size_t i = 0; i<DNCL1BUSINCOUNT; ++i) {
			l1direction[i] = TO_HICANN;
			enable[i] = false;
		}

		dnc_channel_i = new dnc_ser_channel("dnc_channel_i");
		dnc_channel_i->set_side(dnc_ser_channel::DNC_IF);
		for (size_t i=0;i<DNCL1BUSINCOUNT;++i)
           l2tol1_tx_i[i] = new l2tol1_tx( ess::string_builder("l2tol1_tx_i",i),hicannid, i);

		SC_METHOD(start_pulse_dnc);
		dont_initialize(); // avoid ghost events at time 0 with uninitialized nrn id (#1586)
		// sensitive << pulse_event;

		SC_METHOD(rx_l2_ctrl);
		dont_initialize();
		sensitive << clk;

	}
	/// destructor for dnc_if
	~dnc_if(){}

	void rx_l2_ctrl();

	void start_pulse_dnc();

	/// (NEW) Function to send pulse events to the DNC.
	/// It first checks, if direction is configured to DNC and then starts the transmission.
	/// In configuration phase, direction needs to be set (initial=TOWARDS_DNC).
	/// Captures simulation time with resolution of TIMESTAMP_WIDTH.
	/// Finaly it starts event for next function to start transmission over serial dnc channel.
	void l1_task_if(
			const unsigned char& channel, //!< channel id (0..7)
			const unsigned char& _nrnid //!< 6-bit neuron id
			);
	void receive_event(const sc_uint<L2_EVENT_WIDTH>&);
	/// sets direction for each L2toL1 channel.
	void set_directions(const std::vector< enum dnc_if::direction >& directions);
	/// sets enable for each L2toL1 channel.
	void set_enables(const std::vector<bool>& enables);

protected:
	Logger& log; ///< Logger for a unique outputstream for debugging.

};

#endif // _DNC_IF_H_




