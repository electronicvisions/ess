//_____________________________________________
// Company      :	Tu-Dresden
// Author       :	Stefan Scholze
// Refinement	:	Matthias Ehrlich
// E-Mail   	:	*@iee.et.tu-dresden.de
//
// Date         :   Wed Jan 17 09:18:19 2007
// Last Change  :   mehrlich 03/20/2008
// Module Name  :   l2_hyp_task_if
// Filename     :   l2_hyp_task_if.h
// Project Name	:   p_facets/s_systemsim
// Description	:   task interface for hypertransport channel
//
//_____________________________________________

#ifndef _L2_HYP_TASK_IF_H_
#define _L2_HYP_TASK_IF_H_


// always needed
#include "systemc.h"

// defines and helpers
#include "sim_def.h"
#include "types.h"

/// This class provides a high level task interface for a hypertransport - HTX channel.
class l2_hyp_task_if : 
	virtual public sc_interface
{
public:
	/// provide L2 channel interfaces for other DNCs.
	/// interface for pulse events.
	virtual void receive_event(const sc_uint<HYP_EVENT_WIDTH>&) = 0; 
	/// interface for config packets.
	virtual void receive_config(const sc_uint<DNC_TARGET_WIDTH>&, const sc_uint<L2_CFGPKT_WIDTH>&) = 0; 
	virtual void rx_instant_config(const ess::l2_packet_t &) = 0; 

	virtual ~l2_hyp_task_if() {}
};

#endif //_L2_HYP_TASK_IF_H_
