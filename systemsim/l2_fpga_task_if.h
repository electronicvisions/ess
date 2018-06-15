//_____________________________________________
// Company      :	TU-Dresden
// Author       :	Stefan Scholze
// Refinement	:	Matthias Ehrlich
// E-Mail   	:	*@iee.et.tu-dresden.dee
//
// Date         :   Thu Jan 18 12:22:48 2007
// Last Change  :   mehrlich 03/20/2008
// Module Name  :   l2_fpga_task_if
// Filename     :   l2_fpga_task_if.h
// Project Name	:   p_facets/s_systemsim
// Description	:   
//
//_____________________________________________

#ifndef _L2_FPGA_TASK_IF_H_
#define _L2_FPGA_TASK_IF_H_

// always needed
#include "systemc.h"

// defines and helpers
#include "sim_def.h"

/// This abtract class provides provides a high level task interface for the Layer2 FPGA.
class l2_fpga_task_if : 
	virtual public sc_interface
{
public:
	/// receive_fpga_event(...) is one hypertransport interface.
	virtual void receive_fpga_event(const sc_uint<HYP_EVENT_WIDTH>&) = 0;
	virtual ~l2_fpga_task_if() {}
};

#endif //_L2_FPGA_TASK_IF_H_
