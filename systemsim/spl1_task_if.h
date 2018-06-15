//_____________________________________________
// Company      :	tud
// Author       :	Stefan Scholze
// Refined		: 	Matthias Ehrlich
// E-Mail   	:	*@iee.et.tu-dresden.de
//
// Date         :   Thu Dec 14 14:10:13 2006
// Last Change  :   bvogging 08/09/2011
// Module Name  :   dnc_if_task_if
// Filename     :   dnc_if_task_if.h
// Project Name	:   p_facets/s_systemsim
// Description	:   
//
//_____________________________________________


#ifndef _SPL1_TASK_IF_H_
#define _SPL1_TASK_IF_H_

// always needed
#include "systemc.h"

/// This class provides the high level task interface for the DNC.
class spl1_task_if : virtual public sc_interface
{
public:
	/// provide interface to layer1 to receive pulses from dnc_if.
	/// @return true, if sending of pulse was successful, false otherwise.
	virtual bool rcv_pulse_from_dnc_if_channel(
			short addr,  //!< 6-bit neuron address
			int  channel_id //!< channel ID (0..7)
			) = 0;

	virtual ~spl1_task_if() {}
};

#endif //_SPL1_TASK_IF_H_
