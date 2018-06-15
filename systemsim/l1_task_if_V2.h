//_____________________________________________
// Company      :	TUD
// Author       :	Bernhard Vogginger
// E-Mail   	:	Bernhard.Vogginger@tu-dresden.de
//
// Last Change  :   vogginger 11/07/21
// Module Name  :   l1_task_if_V2
// Filename     :   l1_task_if_V2.h
// Project Name	:   p_facets/s_systemsim
// Description	:   Layer 1 Task Interface providing access to Layer1 net for pulse transmission and configuration
// 					between instances of l1_behav
// 					pure C++ implementation


#include "hardware_base.h" // needed for enums
#include "HAL2ESSEnum.h"

#ifndef __L1_TASK_IF_V2_H__
#define __L1_TASK_IF_V2_H__

/** interface of the behavioral layer 1 network.
 *  defines interface methods to set and get configuration of layer 1 stuff
 *  and to receive pulses from neighbour hicanns and the same hicann.
 */
class l1_task_if_V2
{
	public:
	virtual ~l1_task_if_V2() {}
		/** receive an l1 pulse from the left neighbour hicann on a horizontal bus.
		 *  the bus id is the one determined by counting IDs at the border crossing.
		 *  (here identical to hbus id of right(receiving) hicann)
		 *  only call this from left neighbour hicann!.*/
		virtual void rcv_pulse_from_left(
				unsigned int hbus_id, //!< id of horizontal bus
				unsigned int nrn_id  //!< the 6-bit neuron address of the sender neuron
				) const = 0;

		/** receive an l1 pulse from the right neighbour hicann on a horizontal bus.
		 *  the bus id is the one determined by counting IDs at the border crossing.
		 *  (here identical to hbus id of right(sending) hicann)
		 *  only call this from right neighbour hicann!.*/
		virtual void rcv_pulse_from_right(
				unsigned int hbus_id, //!< id of horizontal bus
				unsigned int nrn_id  //!< the 6-bit neuron address of the sender neuron
				) const = 0;

		/** receive an l1 pulse from top neighbour hicann on a vertical bus.
		 *  the bus id is the one determined by counting IDs at the border crossing.
		 *  (here identical to vbus id of top(sending) hicann)
		 *  only call this from top neighbour hicann!.*/
		virtual void rcv_pulse_from_top(
				unsigned int vbus_id, //!< id of vertical bus
				unsigned int nrn_id  //!< the 6-bit neuron address of the sender neuron
				) const = 0;

		/** receive an l1 pulse from bottom neighbour hicann on a vertical bus.
		 *  the bus id is the one determined by counting IDs at the border crossing.
		 *  (here identical to vbus id of top(receiving) hicann)
		 *  only call this from bottom neighbour hicann!.*/
		virtual void rcv_pulse_from_bottom(
				unsigned int vbus_id, //!< id of vertical bus
				unsigned int nrn_id  //!< the 6-bit neuron address of the sender neuron
				) const = 0;

		/** receive an l1 pulse from this hicann on a spl1 repeater.
		 *  i.e. a pulse from a neuron, bg-event-generator or dnc_if is fed into the l1 network.
		 */
		virtual void rcv_pulse_from_this_hicann(
				unsigned int spl1_rep_id,  //!< id of the corresponding spl1 repeater (0..7)
				unsigned int nrn_id  //!< the 6-bit neuron address of the sender neuron
				) const = 0;

		/** configures one repeater of a repeater block with the config byte.*/
		virtual void set_repeater_config(
				ESS::RepeaterLocation repeater_location,  //!< repeater location, i.e. the block
				unsigned int rep_id,					  //!< id of repeater in the block
				unsigned char config					  //!< config byte as given in hicann-documentation
				) = 0;

		/** returns the config byte of one repeater of a repeater block.*/
		virtual unsigned char get_repeater_config(
				ESS::RepeaterLocation repeater_location,  //!< repeater location, i.e. the block
				unsigned int rep_id        //!< id of repeater in the block
				) const = 0;

		/** configures the switches of one row of a switch (Crossbar or Syndriver).
		 * the length of vector config must correspond to the number of switches in a row (CB:4, S: 16)
		 */
		virtual void set_switch_row_config(
				switch_loc switch_location,  //!< switch location, i.e. the type and block
				unsigned int row_addr,       //!< the row within the switch to be configured
				const std::vector<bool>& config  //!< vector containing the config for the switches in that row. (true enables switch)
				) = 0;

		/** returns the configuration of the switches of one row of a switch (Crossbar or Syndriver).
		 */
		virtual const std::vector<bool>& get_switch_row_config(
				switch_loc switch_location,  //!< switch location, i.e. the type and block
				unsigned int row_addr        //!< the row within the switch to be configured
				) const = 0;
};
#endif // __L1_TASK_IF_V2_H__
