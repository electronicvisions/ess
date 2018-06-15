//_____________________________________________
// Company      :	TU-Dresden
// Author       :	Stefan Scholze
// Refinement	:	Matthias Ehrlich
// E-Mail   	:	*@iee.et.tu-dresden.de
//
// Date         :   Thu Jan 18 12:22:48 2007
// Last Change  :   mehrlich 03/20/2008
// Module Name  :   l2_fpga
// Filename     :   l2_fpga.h
// Project Name	:   p_facets/s_systemsim
// Description	:  
//
//_____________________________________________

#ifndef _L2_FPGA_H_
#define _L2_FPGA_H_

// always needed
#include "systemc.h"

#include <vector>

// defines and helpers
#include "sim_def.h"

// functional units
#include "dnc_tx_fpga.h"
#include "HAL2ESSContainer.h"

/// This class provides a Layer2 FPGA.
class l2_fpga :
	public sc_module
{
public:
private:
	int id;
	int wafer_id;

	bool _record; //!< flag for writing pulses to trace memory
	bool _stop; //!< flag to see if stop_trace_memory was called
	ESS::playback_container_t _playback_pulses;
	ESS::trace_container_t _trace_pulses;

	sc_clock clk;
		
	SC_HAS_PROCESS(l2_fpga);
	
public :
	
    dnc_tx_fpga *dnc_tx_fpga_i[DNC_FPGA];

	/// constructor
	l2_fpga (sc_module_name l2_fpga_i, int id, int wafer_id, const ESS::fpga_config & config);

	/// destructor
	~l2_fpga();

	/// checks l2 channel for received data from DNCs.
	void fifo_from_l2_ctrl();

	/// records one pulse event to trace memory
	void record_rx_event(const int dnc_id, const sc_uint<HYP_EVENT_WIDTH>&);

	/// plays pulses from playback memory
	/// This is a sc_thread, which is started at the beginning of simulation.
	/// between events, the thread waits for a given time and then resumes
	/// with sending new events
	void play_tx_event();

    // Dummys for halbe_to_ess
    void setStopTrace(bool stop) {_stop=stop;}
    bool getStopTrace() const {return _stop;}

	// TODO: Should we append playback pulses to the existing Pulses, instead of replacing them?
	// Appending would be more like the real hardware...
	void setPlaybackPulses( const ESS::playback_container_t & playback_pulses ) {_playback_pulses = playback_pulses;}

	// TODO: When do we clear the traced spikes?
	const ESS::trace_container_t & getTracePulses() const { return _trace_pulses; }
    
};

#endif // _L2_FPGA_H_
