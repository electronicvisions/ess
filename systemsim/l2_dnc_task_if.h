//_____________________________________________
// Company      :	Tu-Dresden
// Author       :	Stefan Scholze
// Refined		:   Matthias Ehrlich
// E-Mail   	:	*@iee.et.tu-dresden.de
//
// Date         :   Wed Jan 17 09:18:19 2007
// Last Change  :   mehrlich 03/20/2008
// Module Name  :   l2_dnc_task_if
// Filename     :   l2_dnc_task_if.h
// Project Name	:   p_facets/s_systemsim
// Description	:   Provides a high level task interface for the layer 2 DNC
//
//_____________________________________________

#ifndef _L2_DNC_TASK_IF_H_
#define _L2_DNC_TASK_IF_H_

// always needed
#include "systemc.h"

// defines and helpers
#include "sim_def.h"

/// This class provides provides a high level task interface for the Layer2 DNC.
class l2_dnc_task_if : 
	virtual public sc_interface
{
public:
	/// receive interface for pulse event packets.
	/// interface 2 other DNCs
	virtual void receive_event(const sc_uint<L2_EVENT_WIDTH>&) = 0;
	/// receive interface for configuration packets.	
	virtual void receive_config(const sc_uint<L2_CFGPKT_WIDTH>&) = 0;
	
	virtual ~l2_dnc_task_if() {}
};

#endif //_L2_DNC_TASK_IF_H_
