//_____________________________________________
// Company      :	tud
// Author       :	Stefan Scholze
// Refined		: 	Matthias Ehrlich
// E-Mail   	:	*@iee.et.tu-dresden.de
//
// Date         :   Thu Dec 14 14:10:13 2006
// Last Change  :   mehrlich 03/20/2008
// Module Name  :   dnc_if_task_if
// Filename     :   dnc_if_task_if.h
// Project Name	:   p_facets/s_systemsim
// Description	:   
//
//_____________________________________________


#ifndef _DNC_IF_TASK_IF_H_
#define _DNC_IF_TASK_IF_H_

// always needed
#include "systemc.h"

// defines and helpers
#include "sim_def.h"

/// This class provides the high level task interface for the DNC.
class dnc_if_task_if : virtual public sc_interface
{
public:
	/** interface to receive spikes from the spl1_merger tree of a hicann. */
	virtual void l1_task_if(
			const unsigned char&, //!< channel id (0..7)
			const unsigned char&  //!< 6-bit neuron id
			) = 0;

	virtual ~dnc_if_task_if() {}
};

#endif //_DNC_IF_TASK_IF_H_
