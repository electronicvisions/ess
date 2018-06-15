//_____________________________________________
// Company      :	TUD
// Author       :	Bernhard Vogginger
// E-Mail   	:	Bernhard.Vogginger@tu-dresden.de
//
// Last Change  :   vogginger 11/07/21
// Module Name  :   l1_behav_V2
// Filename     :   l1_behav_V2.h
// Project Name	:   p_facets/s_systemsim
// Description	:   Top level behavioural model for
// 					Layer 1 net communication for one HICANN instance
// 					pure C++ implementation
//_____________________________________________

#ifndef __L1_BEHAV_V2_H__
#define __L1_BEHAV_V2_H__

// defines & helpers
#include "sim_def.h"
#include <vector>
#include <set>
#include <string>
#include <memory>

#include "l1_task_if_V2.h"

class anncore_task_if;

/** behavioral implementation of the Layer 1 network of HICANN V2.*/
class l1_behav_V2 : public l1_task_if_V2
{

	private:
		/** enum that describes, where an L1 event is coming from.*/
		enum l1_source {
			from_hbus = 0,   //!< event comes from an horizontal bus
			from_vbus = 1,   //!< event comes from a vertical bus
			from_top = 2,    //!< event comes from top neigbour hicann
			from_bottom = 3, //!< event comes from bottom neigbour hicann
			from_left = 4,   //!< event comes from left neigbour hicann
			from_right = 5   //!< event comes from right neigbour hicann
		};
		/** enum for storing the configuration of a repeater. The last 3 choices(spl1_X) are only valid for the spl1-repeaters in the left repeater block.*/
		enum direction {
			off = 0, //!< repeater is off (used as a normal repeater)
			forward = 1, //!< repeater reproduces signal from this hicann for the neighbour neighbour (used as a normal repeater)
			backward = 2, //!< repeater reproduces signal from neighbour hicann for this hicann (used as a normal repeater)
			spl1_int = 3, //!< input from spl1-merger is applied to the bus on this hicann.
			spl1_ext = 4, //!< input from spl1-merger is applied to the bus on the left neighbour hicann.
			spl1_int_and_ext = 5 //!< input from spl1-merger is applied to both direction(bus on this and the bus on the left neighbour hicann)
		};

		static const std::vector< std::string > l1_source_s;
		static const std::vector< std::string > direction_s;

		/** initializes the topologies of the crossbar and syndriver switches. 
		 * fills vectors _topology_X (X from [cb_l, cb_r, ss_tl, ss_tr, ss_dl, ss_dr])
		 * according to the Hicann V2's routing topology.
		 * called by constructor.
		 */
		void initialize_topologies();

		/** shifts a vertical bus id by shift and returns the new one.
		 * assures that the new bus id stays in the same domain, namely either 0..127 or 128..255.
		 * new_bus = (bus_id+shift)%128 + (bus_id/128)*128
		 */
		unsigned int shift_vbus(
				unsigned int bus_id, //!< vertical bus id
				int shift //!< shift
				) const;

		/** shifts an horizontal bus id by shift and returns the new one.
		 * assures that the new bus id stays within the range(0..63).
		 * new_bus = (bus_id+shift)%64
		 */
		unsigned int shift_hbus(
				unsigned int bus_id, //!< vertical bus id
				int shift //!< shift
				) const;

		/** determines the direction of a repeater from its config_byte.
		 * See doc of enum direction for possible values.*/
		static direction determine_direction(
				unsigned char config_byte //!< repeater config byte, as defined in Hicann Documentation.
				);

		/** determines the direction of an spl1 repeater from its config_byte.
		 * See doc of enum direction for possible values.*/
		static direction determine_direction_spl1(
				unsigned char config_byte //!< repeater config byte, as defined in Hicann Documentation.
				);

		/** process an event on a horizontal bus.
		 * depending on the source of the signal (given in param from_where) the signal is further processed.
		 * cases:
		 * * from_left: checks connection to vertical buses and forwards event to right repeater block
		 * * from_right: checks connection to vertical buses and forwards event to left repeater block
		 * * from_vbus: forwards event to both left and right repeater block.
		 */
		void process_hbus(
				const unsigned int hbus,    //!< horizontal bus carrying the signal
				const unsigned int nrn_id,  //!< the 6-bit neuron address of the sender neuron
				l1_source from_where        //!< the source of this signal: one of [from_left,  from_right, from_vbus]
				) const;

		/** process an event on a vertical bus.
		 * depending on the source of the signal (given in param from_where) the signal is further processed.
		 * cases:
		 * * from_top: checks connection to horizontal buses and forwards event to bottom repeater block
		 * * from_bottom: checks connection to horizontal buses and forwards event to top repeater block
		 * * from_hbus: forwards event to both top and bottom repeater block.
		 * In ALL cases the concerned syndriver switches are checked and the event is possibly forwarded to the anncore of this or the neighour hicann.
		 */
		void process_vbus(
				const unsigned int vbus,    //!< vertical bus carrying the signal
				const unsigned int nrn_id,  //!< the 6-bit neuron address of the sender neuron
				l1_source from_where        //!< the source of this signal: one of [from_top,  from_bottom, from_hbus]
				) const;

		/** process an event at the right repeater block.
		 * the horizontal bus shifting is performed in here. param hbus really means the horizontal bus, on which the signal is.
		 * depending on its origin, it checks whether to forward the event to the right neighbour or to the horizontal bus or to discard it.
		 */
		void process_repeater_right(
				const unsigned int hbus,    //!< horizontal bus carrying the signal
				const unsigned int nrn_id,  //!< the 6-bit neuron address of the sender neuron
				l1_source from_where        //!< the source of this signal: one of [from_hbus,  from_right]
				) const;

		/** process an event at the left repeater block.
		 * param hbus really means the horizontal bus, on which the signal is.
		 * (busshifting is done in the right repeater block).
		 * depending on its origin, it checks whether to forward the event to the left neighbour or to the horizontal bus or to discard it.
		 */
		void process_repeater_left(
				const unsigned int hbus,    //!< horizontal bus carrying the signal
				const unsigned int nrn_id,  //!< the 6-bit neuron address of the sender neuron
				l1_source from_where        //!< the source of this signal: one of [from_hbus,  from_left]
				) const;

		/** process an event at the top repeater block.
		 * the vertical bus shifting is performed in here. param vbus really means the vertical bus, on which the signal is.
		 * depending on its origin, it checks whether to forward the event to the top neighbour or to the vertical bus or to discard it.
		 */
		void process_repeater_top(
				const unsigned int vbus,    //!< vertical bus carrying the signal
				const unsigned int nrn_id,  //!< the 6-bit neuron address of the sender neuron
				l1_source from_where        //!< the source of this signal: one of [from_vbus,  from_top]
				) const;

		/** process an event at the bottom repeater block.
		 * no bus shifting done here. param vbus really means the vertical bus, on which the signal is.
		 * depending on its origin, it checks whether to forward the event to the top neighbour or to the vertical bus or to discard it.
		 */
		void process_repeater_bottom(
				const unsigned int vbus,    //!< vertical bus carrying the signal
				const unsigned int nrn_id,  //!< the 6-bit neuron address of the sender neuron
				l1_source from_where        //!< the source of this signal: one of [from_vbus,  from_bottom]
				) const;

		/** ID of this l1_behav_V2 instance */
		unsigned int l1_id;

		static const unsigned int _num_cb_switches_per_row = 4;  //!< Number of Switches per Row in one Crossbar Switch (left or right)
		static const unsigned int _num_syndr_switches_per_row = 16;  //!< Number of Switches per Row in one Syndriver Switch (ul ur dl dr)
		static const unsigned int _num_syndr_per_block = 56;  //!< Number of Syndrivers per Syndriver block (ul ur dl dr)
		static const int _hor_shift_to_right = -2;  //!< shift happening to a horizontal bus, when crossing the border towards its right neighbour.
		static const int _ver_shift_to_top = 2;  //!< shift happening to a vertical bus, when crossing the border towards its top neighbour.

		std::string sim_folder; ///< Folder for temporary and debug files during simulation
		std::string name; ///< Folder for temporary and debug files during simulation

		//////////////////////
		//  Repeater Config 
		//////////////////////

		std::vector <unsigned char> _config_rep_tl; //!< repeater config top left containing the config byte
		std::vector <unsigned char> _config_rep_tr; //!< repeater config top right containing the config byte
		std::vector <unsigned char> _config_rep_bl; //!< repeater config bottom left containing the config byte
		std::vector <unsigned char> _config_rep_br; //!< repeater config bottom right containing the config byte
		std::vector <unsigned char> _config_rep_r; //!< repeater config right containing the config byte
		std::vector <unsigned char> _config_rep_l; //!< repeater config left containing the config byte

		std::vector <direction> _direction_rep_t; //!< configured direction of top repeaters (see doc of enum direction)
		std::vector <direction> _direction_rep_b; //!< configured direction of bottom repeaters (see doc of enum direction)
		std::vector <direction> _direction_rep_l; //!< configured direction of left repeaters, includes spl1_repeaters (see doc of enum direction)
		std::vector <direction> _direction_rep_r; //!< configured direction of right repeaters (see doc of enum direction)

		//////////////////////
		//  CrossBar Switches
		//////////////////////


		/** topology of left crossbar switches.
		   outer vector: one element for every horizontal bus (i.e. 64)
		   inner vector: indices of vertical buses for which a switch is available from this vbus (4)
		*/
		std::vector < std::vector<unsigned int> > _topology_cb_l;

		/** config of left crossbar switches.
		   outer vector: one element for every horizontal bus (i.e. 64)
		   inner vector: switches of this horizontal bus(4), they enable switching to horizontal buses defined in _topology_cb_l with the same index.
		*/
		std::vector < std::vector<bool> > _config_cb_l;

		/** topology of right crossbar switches.
		   outer vector: one element for every horizontal bus (i.e. 64)
		   inner vector: indices of vertical buses for which a switch is available from this vbus (4)
		*/
		std::vector < std::vector<unsigned int> > _topology_cb_r;

		/** config of right crossbar switches.
		   outer vector: one element for every horizontal bus (i.e. 64)
		   inner vector: switches of this horizontal bus(4), they enable switching to horizontal buses defined in _topology_cb_r with the same index.
		*/
		std::vector < std::vector<bool> > _config_cb_r;

		/** connected vertical bus for every horizontal bus.
		   one element for every horizontal bus (i.e. 64).
		   if entry = -1, no switch is connected.
		   The switch config in general allows to connect several vertical buses with a horizontal bus,
		   however this is not possible due to the capacitive load.
		   So each hbus is connected to max one vbus and vice versa.
		*/
		std::vector < int >  _connections_hbus_to_vbus;

		/** connected horizontal bus for every vertical bus.
		   one element for every vertical bus (i.e. 256).
		   if entry = -1, no switch is connected.
		   The switch config in general allows to connect several vertical buses with a horizontal bus,
		   however this is not possible due to the capacitive load.
		   So each hbus is connected to max one vbus and vice versa.
		*/
		std::vector < int >  _connections_vbus_to_hbus;


		///////////////////////////////
		// Syndriver Switches
		///////////////////////////////

		/** topology of top left syndriver switches.
		   outer vector: one element for every row of the switch (there are 112 rows)
		   inner vector: indices of vertical buses for which a switch is available from this vbus (16), vbus ids are global(left=0..127, right 128..255)
		*/
		std::vector < std::vector<unsigned int> > _topology_ss_tl;

		/** topology of top right syndriver switches.
		   outer vector: one element for every row of the switch (there are 112 rows)
		   inner vector: indices of vertical buses for which a switch is available from this vbus (16), vbus ids are global(left=0..127, right 128..255)
		*/
		std::vector < std::vector<unsigned int> > _topology_ss_tr;

		/** topology of bottom left syndriver switches.
		   outer vector: one element for every row of the switch (there are 112 rows)
		   inner vector: indices of vertical buses for which a switch is available from this vbus (16), vbus ids are global(left=0..127, right 128..255)
		*/
		std::vector < std::vector<unsigned int> > _topology_ss_bl;

		/** topology of bottom right syndriver switches.
		   outer vector: one element for every row of the switch (there are 112 rows)
		   inner vector: indices of vertical buses for which a switch is available from this vbus (16), vbus ids are global(left=0..127, right 128..255)
		*/
		std::vector < std::vector<unsigned int> > _topology_ss_br;

		/** config of switches in syndriver switch top left.
		 * outer vector: rows, inner vector: 16 switches per row.
		 * not used during simulation, only for storing the config.*/
		std::vector < std::vector<bool> > _config_ss_tl;

		/** config of switches in syndriver switch top right.
		 * outer vector: rows, inner vector: 16 switches per row.
		 * not used during simulation, only for storing the config.*/
		std::vector < std::vector<bool> > _config_ss_tr;

		/** config of switches in syndriver switch bottom left.
		 * outer vector: rows, inner vector: 16 switches per row.
		 * not used during simulation, only for storing the config.*/
		std::vector < std::vector<bool> > _config_ss_bl;

		/** config of switches in syndriver switch bottom right.
		 * outer vector: rows, inner vector: 16 switches per row.
		 * not used during simulation, only for storing the config.*/
		std::vector < std::vector<bool> > _config_ss_br;

		/** connected (individual) syndriver switches for every vertical bus.
		 * for each vertical bus, a set with the positions of the enabled switches.
		 * positions are in range (0..223)
		 * from the position of the switch, we can determine the connected syndriver 
		 * positions >= 112 refer to the lower block.
		 * even numbered switches lead to syndrivers at the right of the current vertical bus bundle.
		 * odd numbered switches to the left.
		 *  e.g. if we are on the right side of the hicann, and have an odd switch,
		 *  this signal feeds a syndriver of this hicann.
		 *  is updated during set-method and used during simulation
		*/
		std::vector < std::set< unsigned int > > _conns_vbus_to_syndr_switch;

	protected:
		/** Constructor initializes data structures storing configuration data with default values.*/
		l1_behav_V2();

	public:
		/////////////////////////////////////////
		////// C O N F I G U R A T I O N ////////
		/////////////////////////////////////////
		/** doc in l1_task_if_V2.h .*/
		virtual void set_switch_row_config(switch_loc switch_location, unsigned int row_addr, const std::vector<bool>& config);

		/** doc in l1_task_if_V2.h .*/
		virtual const std::vector<bool>& get_switch_row_config(
				switch_loc switch_location,
				unsigned int row_addr
				) const;

		/** doc in l1_task_if_V2.h .*/
		virtual void set_repeater_config(
				ESS::RepeaterLocation repeater_location,
				unsigned int rep_id,
				unsigned char config
				);

		/** doc in l1_task_if_V2.h .*/
		virtual unsigned char get_repeater_config(
				ESS::RepeaterLocation repeater_location,
				unsigned int rep_id
				) const;

		////////////////////////////////////////////
		////// P U L S E    H A N D L I N G ////////
		////////////////////////////////////////////

		/** doc in l1_task_if_V2.h .*/
		void rcv_pulse_from_left(unsigned int hbus_id, unsigned  int nrn_id) const;

		/** doc in l1_task_if_V2.h .*/
		void rcv_pulse_from_right(unsigned int hbus_id, unsigned  int nrn_id) const;

		/** doc in l1_task_if_V2.h .*/
		void rcv_pulse_from_top(unsigned int vbus_id, unsigned int nrn_id) const;

		/** doc in l1_task_if_V2.h .*/
		void rcv_pulse_from_bottom(unsigned int vbus_id, unsigned int nrn_id) const;

		/** doc in l1_task_if_V2.h .*/
		void rcv_pulse_from_this_hicann(unsigned int spl1_rep_id, unsigned int nrn_id) const;

		/** prints config to a file, whose name is defined in the passed string.*/
		int print_cfg(
				std::string fn  //!< name of file, to which the config will be printed
				) const;

		/** test stuff */
		void test();

		//TODO smart_ptr
		// Ports to 4 l1_behav_V2 neighbours
		l1_task_if_V2* l1_nb_bottom;  //!< l1_behav neighbour bottom,  vertical connections
		l1_task_if_V2* l1_nb_top;     //!< l1_behav neighbour top,     vertical connections
		l1_task_if_V2* l1_nb_right;   //!< l1_behav neighbour right, horizontal connections
		l1_task_if_V2* l1_nb_left;    //!< l1_behav neighbour left,  horizontal connections

		std::weak_ptr<anncore_task_if> anncore_if;  //!< Interface to anncore of same HICANN

		std::weak_ptr<anncore_task_if> anncore_if_right;  //!< Interface to anncore of right neighbour HICANN
		std::weak_ptr<anncore_task_if> anncore_if_left;   //!< Interface to anncore of left neighbour HICANN


		/** constructor of l1_behav_V2.*/
		l1_behav_V2(
				unsigned int id,  //!< ID of this l1 instance
				std::string temp_folder, //!< temporary folder were debug information is printed to.
				std::string const & name
				);
		
		/** destructor */
		virtual ~l1_behav_V2();

};

inline
unsigned int
l1_behav_V2::shift_vbus(
		unsigned int bus_id,
		int shift
		) const
{
	return (bus_id/VERTICAL_L1_COUNT)*VERTICAL_L1_COUNT + (bus_id + shift)%VERTICAL_L1_COUNT;
}


inline
unsigned int
l1_behav_V2::shift_hbus(
		unsigned int bus_id,
		int shift
		) const
{
	return (bus_id + shift)%HORIZONTAL_L1_COUNT;
}


#endif // __L1_BEHAV_V2_H__
