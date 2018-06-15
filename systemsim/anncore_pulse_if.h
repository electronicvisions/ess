//_____________________________________________
// Company      :	Tu-Dresden
// Author       :	Stefan Scholze
// E-Mail   	:	scholze@iee.et.tu-dresden.de
//
// Date         :   Wed Jan 17 09:18:19 2007
// Last Change  :   bvogging 08/09/2011
// Module Name  :   anncore_pulse_if
// Filename     :   anncore_pulse_if.h
// Project Name	:   p_facets/s_systemsim
// Description	:   interface for pulses from pulseneuron to WTA in ANNCORE
//
//_____________________________________________

#ifndef __ANNCORE_PULSE_IF_H__
#define __ANNCORE_PULSE_IF_H__

// always needed
#include "systemc.h"

// defines and helpers
#include "sim_def.h"

/// This class provides a TLM task interface for pulses from Anncore
/// to WTAs within the Hicann
class anncore_pulse_if : virtual public sc_interface
{
public:
	virtual void handle_spike(
		    const sc_uint<LD_NRN_MAX>& logical_neuron, //!< ID of sending logical neuron(0..511) for debug information
		    const short& addr,                         //!< 6-bit L1 address
		    int wta_id                                 //!< the Priority Encoder receiving the spike(0..7)
		   ) = 0;
	virtual ~anncore_pulse_if() {}
};

#endif //__ANNCORE_PULSE_IF_H__
