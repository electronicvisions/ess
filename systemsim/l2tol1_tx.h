//_____________________________________________
// Company      :	TU-Dresden
// Author       :	Stefan Scholze
// Refined		: 	Matthias Ehrlich
// E-Mail   	:	scholze@iee.et.tu-dresden.de
//
// Date         :   Tue Apr 18 14:08:03 2007
// Last Change  :   mehrlich 03/20/2008
// Module Name  :   l2tol1_tx
// Filename     :   l2tol1_tx.h
// Project Name	:   p_facets/s_systemsim
// Description	:   gets data from l2, delays until time and release to l1 bus
//
//_____________________________________________

#ifndef __L2TOL1_TX_H__
#define __L2TOL1_TX_H__

// always needed
#include "systemc.h"

// defines and helpers
#include "sim_def.h"

// functional units
//#include "l1bus_tx.h"
#include "l2tol1_buffer_element.h"
#include "spl1_task_if.h"

/// This class provides a Layer1 to Layer2 delay line.
/// Gets data from l2, delays until time and release to l1 bus
class l2tol1_tx : public sc_module
{
public:
	enum direction {
		TO_DNC = 0,
		TO_HICANN= 1
	};
	sc_port <spl1_task_if, 1, SC_ZERO_OR_MORE_BOUND> l1bus_tx_if;  //!< interface to the spl1-merger tree for sending spikes to the hicann
	sc_clock clk; ///< reference clock

	double sim_time; ///< contains simulation time
	short hicannid; ///< id of hicanna
	int channel_id; ///< id of channel (0..7)
	direction _direction;
	bool output_lock; ///<  ensures that output is disabled for one clock cycle after release
	sc_uint <TIMESTAMP_WIDTH+L1_ADDR_WIDTH> buffer; ///< buffer for events (in!)
	sc_uint <L1_ADDR_WIDTH> out_buffer; ///< buffer for events, is filled, when two pulse shall fire at the same time.
	bool out_buffer_valid; ///< valid bit of buffer for events
	sc_event rx_data; ///< signal at event arrival

    ess::l2tol1_buffer_element memory[DNC_IF_2_L2_BUFFERSIZE]; ///< memory structure to store events and to offer fast access to timestamp

	///SystemC macro to allow continously running event controlled methods
	SC_HAS_PROCESS(l2tol1_tx);

	/// constructor for l2tol1_tx
	///\param l2tol1_tx: instance name
	///\param hicann_id: identificationnumber of hicann
	///\param channel_id:
	l2tol1_tx (sc_module_name l2tol1_tx, short hicann_id, int channel_id)
		:clk("clk",CLK_PER_L2TOL1_TX,SC_NS)
		, hicannid(hicann_id)
		, channel_id(channel_id)
		, _direction(l2tol1_tx::TO_HICANN)
		, output_lock(false)
		, out_buffer(0)
		, out_buffer_valid(false)
	{

        //void cast to avoid unused parameter warning
        (void) l2tol1_tx;

		int k;
		for(k=0;k<DNC_IF_2_L2_BUFFERSIZE;++k)
		{
            memory[k].valid = false;
		}

		SC_METHOD(check_time);
		dont_initialize();
		sensitive_pos << clk;

		SC_METHOD(add_buffer);
		dont_initialize();
		sensitive << rx_data;
	}

	void transmit(sc_uint <TIMESTAMP_WIDTH+L1_ADDR_WIDTH>);

	void check_time();
	void serialize(sc_uint <L1_ADDR_WIDTH>);
	void add_buffer();
	void set_direction(enum l2tol1_tx::direction dir);
};

#endif //__L2TOL1_TX_H__
