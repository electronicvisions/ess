#ifndef _spl1_merger_h_
#define _spl1_merger_h_
// always needed
#include "systemc.h"

#include <vector>
#include <stdint.h>

// defines and helpers
#include "sim_def.h"
#include "mutexfile.h"
#include "HAL2ESSEnum.h"

#include "anncore_pulse_if.h"
#include "spl1_task_if.h"
#include "dnc_if_task_if.h"
#include "l1_task_if_V2.h"

//Forward declarations
class bg_event_generator;
class merger;
class merger_config_if;
class priority_encoder;

/** The SPL1 Merger class.
 * Models the SPL1 Merger tree including the WTAs and Background event generators.
 * It can receive spikes from either neurons via the anncore_pulse_if or from the dnc_if via the spl1_task_if.
 * It processes those spikes and possibly sends them to the spl1_repeaters of the l1_behav.
 */
class spl1_merger: public anncore_pulse_if, public spl1_task_if, public sc_module
{
public:
	/** mapping of output register to spl1 repeater id.
	 * This considers the fact that the order between both is reversed in the HICANN.
	 */
	static const std::vector< unsigned int > output_register_to_spl1_repeater;

	/** mapping of priority encoder (wta) to background merger id.
	 * This considers the fact that the order between both is reversed in the HICANN.
	 */
	static const std::vector< unsigned int > wta_to_bg_merger;

	/** configures the spl1_merger.
	 * for the details of addresses and the meaning of the config bits, see hicann-doc.*/
	void set_config(
			unsigned int adr,  //!< the adr of the register row, that is configured (0..19)
			const std::vector<bool>& config  //!< vector of size 16 containing the config of one address row.
			);
	void set_dnc_if_directions(
			const std::vector<ESS::DNCIfDirections>& directions //!< vector of size 8 containing the config for each dnc if channel (1 means from Hicann to DNC )
			);

	/** returns the configuration of the spl1_merger.
	 * for the details of addresses and the meaning of the config bits, see hicann-doc.*/
	const std::vector<bool>& get_config(
			unsigned int adr  //!< the adr of the register row, that is configured (0..19)
			) const;

	/** Constructor.*/
	spl1_merger(
			sc_module_name name,            //!< instance name
			short hicann_id,                //!< ID of Hicann to which this merger belongs to.
			ess::mutexfile& spike_tcx_file, //!> File to handle recording of transmitted events	
			uint8_t PLL_period_ns,          //!> the PLL period
            bool enable_timed,              //!> Flag for timed merger
            bool enable_spike_debugging    	//!> Flag for spike debugging
			);

	virtual ~spl1_merger();

	/** doc in anncore_pulse_if */
	virtual void handle_spike(
		    const sc_uint<LD_NRN_MAX>& logical_neuron, //!< ID of sending logical neuron(0..511) for debug information
		    const short& addr,                         //!< 6-bit L1 address
		    int wta_id                                 //!< the Priority Encoder(=WTA) receiving the spike (0..7)
		   );

	/** doc in spl1_task_if */
	virtual bool rcv_pulse_from_dnc_if_channel(
			short addr,  //!< 6-bit neuron address
			int  channel_id //!< channel ID (0..7)
			);


	void send_pulse_to_output_register(
			short addr,  //!< 6-bit neuron address
			unsigned int  channel_id //!< channel ID (0..7)
			);

	void print_cfg(std::string fn);

    bg_event_generator const& get_background(size_t i) const;
	merger const& get_bg_merger(size_t i) const;

	sc_port<dnc_if_task_if> dnc_if_if; //!< Interface to dnc_if to send spikes to layer 2

	l1_task_if_V2* l1_task_if;  //!< interface to the layer 1 network

	void print_config(std::ostream & out);

protected:
	void connect_mergers(); //!< connect the mergers according to hicann spec.
	short hicannid;         //!< ID of this HICANN, to which this merger belongs to in the wafer-system, for debugging
	bool timed;             //!< flag for timed merger
    bool _spike_debugging;  //!< flag for spike_debugging
    ess::mutexfile& spike_tx_file;	//!< FILE handle for recording of transmitted events

	///** default constructor, should not be called.*/
	//spl1_merger();
	merger*  bg_merger_i[ANNCORE_WTA];  //!< background mergers (8)
	merger* level_merger_i[ANNCORE_WTA];  //!< level 0 to 2 mergers (8, the last one is not used!!)
	merger* dnc_merger_i[ANNCORE_WTA];  //!< dnc mergers

	std::vector< merger_config_if* > bg_and_level_mergers;  //!< bg and level mergers in one vector, needed for configuration. (16)

	uint16_t bg_generator_seed; //!< common seed of the background event generators

	// we us an array here, for the sc_module hierarchy
	bg_event_generator* bg_event_generator_i[ANNCORE_WTA]; //!< background event generators
  
	std::vector< ESS::DNCIfDirections > dnc_if_directions; //!< if true: from dnc to hicann

	priority_encoder* priority_encoder_i[ANNCORE_WTA]; //!< priority encoders, these are only used if timed == true
};
#endif //_spl1_merger_h_
