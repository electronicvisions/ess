//_____________________________________________
// Company      :	TUD
// Author       :	Bernhard Vogginger
// E-Mail   	:	Bernhard.Vogginger@tu-dresden.de
//
// Last Change  :   vogginger 11/07/21
// Module Name  :   l1_behav_V2
// Filename     :   l1_behav_V2.cpp
// Project Name	:   p_facets/s_systemsim
// Description	:   Top level behavioural model for
// 					Layer 1 net communication for one HICANN instance
// 					pure C++ implementation
//_____________________________________________

#include "l1_behav_V2.h"
#include "anncore_task_if.h"
#include "boost/assign/list_of.hpp"
#include <log4cxx/logger.h>
#include <stdexcept>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("ESS.HICANN.Layer1");

// some helper functions
template <class type>
void print_vec(std::vector <type> * v)
{
	for (unsigned int i = 0; i < v->size(); i++)
		cout << "v[" << i << "]:" << (*v)[i] << " ";
	cout << endl;
}

template <class type>
void print_vec_vec(std::vector <std::vector <type>  > * v)
{
	for (unsigned int i = 0; i < v->size(); i++){
		for (unsigned int j = 0; j < (*v)[i].size(); j++){
			cout << "v[" << i << "][" << j << "]:" << (*v)[i][j] << " ";
		}
		cout << endl;
	}
	cout << endl;
}


const std::vector< std::string >
l1_behav_V2::l1_source_s = boost::assign::list_of("from_hbus")("from_vbus")("from_top")("from_bottom")("from_left")("from_right");

const std::vector< std::string >
l1_behav_V2::direction_s = boost::assign::list_of("off")("forward")("backward")("spl1_int")("spl1_ext")("spl1_int_and_ext");

l1_behav_V2::l1_behav_V2(unsigned int id, std::string temp_folder, std::string const & name) :
	l1_id(id)
	, sim_folder(temp_folder)
	, name(name)
{
	l1_nb_bottom = NULL;    // l1_behav neighbour bottom,  vertical connections
	l1_nb_top = NULL;    // l1_behav neighbour top,     vertical connections
	l1_nb_right = NULL;   // l1_behav neighbour right, horizontal connections
	l1_nb_left = NULL;    // l1_behav neighbour left,  horizontal connections
    // check here: one anncore must be found!
        
    LOG4CXX_INFO(logger, "l1_behav_V2 with id " << l1_id << " instantiated " );

	initialize_topologies();
	// all switches are disabled
	_config_cb_l.resize(HORIZONTAL_L1_COUNT, std::vector<bool>(_num_cb_switches_per_row,0) );
	_config_cb_r.resize(HORIZONTAL_L1_COUNT, std::vector<bool>(_num_cb_switches_per_row,0) );
	_config_ss_tl.resize(_num_syndr_per_block*2, std::vector<bool>(_num_syndr_switches_per_row,0) );
	_config_ss_tr.resize(_num_syndr_per_block*2, std::vector<bool>(_num_syndr_switches_per_row,0) );
	_config_ss_bl.resize(_num_syndr_per_block*2, std::vector<bool>(_num_syndr_switches_per_row,0) );
	_config_ss_br.resize(_num_syndr_per_block*2, std::vector<bool>(_num_syndr_switches_per_row,0) );
	_connections_hbus_to_vbus.resize( HORIZONTAL_L1_COUNT, -1 );
	_connections_vbus_to_hbus.resize( 2*VERTICAL_L1_COUNT, -1 );
	_conns_vbus_to_syndr_switch.resize( 2*VERTICAL_L1_COUNT );

	_config_rep_l.resize(HORIZONTAL_L1_COUNT/2, 0);
	_config_rep_r.resize(HORIZONTAL_L1_COUNT/2, 0);
	_config_rep_tl.resize(VERTICAL_L1_COUNT/2, 0);
	_config_rep_tr.resize(VERTICAL_L1_COUNT/2, 0);
	_config_rep_bl.resize(VERTICAL_L1_COUNT/2, 0);
	_config_rep_br.resize(VERTICAL_L1_COUNT/2, 0);
	_direction_rep_t.resize(VERTICAL_L1_COUNT, off);
	_direction_rep_b.resize(VERTICAL_L1_COUNT, off);
	_direction_rep_l.resize(HORIZONTAL_L1_COUNT, off);
	_direction_rep_r.resize(HORIZONTAL_L1_COUNT, off);
}

//destructor
l1_behav_V2::~l1_behav_V2()
{
	// if(l1_nb_bottom != NULL)
	// 	delete l1_nb_bottom;
	// if(l1_nb_top!= NULL)
	// 	delete l1_nb_top;   
	// if(l1_nb_right != NULL)
	// 	delete l1_nb_right; 
	// if(l1_nb_left != NULL)
	// 	delete l1_nb_left;  
}

void
l1_behav_V2::set_switch_row_config(
		switch_loc switch_location,
		unsigned int row_addr,
		const std::vector<bool>& config
		){
	std::vector<bool>* current_config_row = NULL;
	std::vector<unsigned int>* current_topology_row = NULL;
	bool is_syndr_switch = true;
	unsigned int switch_row_offset = 0;

	switch( switch_location ) {
		case CBL:
			current_config_row = &(_config_cb_l.at(row_addr));
			current_topology_row = &(_topology_cb_l.at(row_addr));
			is_syndr_switch = false;
			break;
		case CBR:
			current_config_row = &(_config_cb_r.at(row_addr));
			current_topology_row = &(_topology_cb_r.at(row_addr));
			is_syndr_switch = false;
			break;
		case SYNTL:
			current_config_row = &(_config_ss_tl.at(row_addr));
			current_topology_row = &(_topology_ss_tl.at(row_addr));
			break;
		case SYNTR:
			current_config_row = &(_config_ss_tr.at(row_addr));
			current_topology_row = &(_topology_ss_tr.at(row_addr));
			break;
		case SYNBL:
			current_config_row = &(_config_ss_bl.at(row_addr));
			current_topology_row = &(_topology_ss_bl.at(row_addr));
			switch_row_offset = 2*_num_syndr_per_block;
			break;
		case SYNBR:
			current_config_row = &(_config_ss_br.at(row_addr));
			current_topology_row = &(_topology_ss_br.at(row_addr));
			switch_row_offset = 2*_num_syndr_per_block;
			break;
	}
	assert( current_config_row->size() == config.size() );

	if( is_syndr_switch ) {
		//  Syndriver Switch
		//  have to fix row address for bottom blocks
		unsigned int syndriver_switch_row = row_addr + switch_row_offset;
		int num_entries = 0;
		for( size_t i_cfg = 0; i_cfg < config.size(); ++i_cfg){
			unsigned int vbus_id = (*current_topology_row)[ i_cfg ];
			if( !config[i_cfg] ){
				// erase entry if it exists:
				_conns_vbus_to_syndr_switch.at( vbus_id ).erase( syndriver_switch_row );
			}
			else {
				// insert entry
				_conns_vbus_to_syndr_switch.at( vbus_id ).insert( syndriver_switch_row );
				++num_entries;
			}
		}
		if (num_entries > 1)
            LOG4CXX_WARN(logger, "Wrong Configuration of Syndriver Switch: Several vertical buses connect to one Syndriver." );
	} else {
		// crossbar switch
		// is a little more tricky: max one connected bus for each v- or h-bus.
		int hbus_id = row_addr;
		for( size_t i_cfg = 0; i_cfg < config.size(); ++i_cfg){
			int vbus_id = (*current_topology_row)[ i_cfg ];
			if( !config[i_cfg] ){
				// check whether we have to un-set this switch.(this switch was enabled before)
				if (_connections_hbus_to_vbus.at( hbus_id ) ==  vbus_id) {
					_connections_hbus_to_vbus[ hbus_id ] = -1;
					_connections_vbus_to_hbus[ vbus_id ] = -1;
				}
			} else {
				// check for consistency:
				int old_vbus = _connections_hbus_to_vbus.at( hbus_id );
				int old_hbus = _connections_vbus_to_hbus.at( vbus_id );
				_connections_hbus_to_vbus[ hbus_id ] = vbus_id;
				_connections_vbus_to_hbus[ vbus_id ] = hbus_id;
				if ( (old_vbus != -1 && old_vbus != vbus_id) || (old_hbus != -1 && old_hbus != hbus_id) ) {
                    LOG4CXX_WARN(logger, "Wrong Configuration of Crossbar Switches: Multiple connections for buses! \n"
						<< "you connected hbus " << hbus_id << "with vbus " << vbus_id 
						<< " and thereby erased connection between hbus" << old_hbus << " and vbus " << old_vbus );
				}
			}
		}
	}
	current_config_row->assign( config.begin(), config.end() );
}

const std::vector<bool>&
l1_behav_V2::get_switch_row_config(
		switch_loc switch_location,
		unsigned int row_addr
		) const {
	switch( switch_location ) {
		case CBL:
			return _config_cb_l.at(row_addr);
			break;
		case CBR:
			return _config_cb_r.at(row_addr);
			break;
		case SYNTL:
			return _config_ss_tl.at(row_addr);
			break;
		case SYNTR:
			return _config_ss_tr.at(row_addr);
			break;
		case SYNBL:
			return _config_ss_bl.at(row_addr);
			break;
		case SYNBR:
			return _config_ss_br.at(row_addr);
			break;
        default:
            LOG4CXX_ERROR(logger, name << " get_switch_row_config called with invalid switch_location" );
            throw std::runtime_error("l1_behav_V2: get_switch_row_config called with invalid switch_location" );
	}
}


void l1_behav_V2::process_vbus(
				const unsigned int vbus,    //!< vertical bus carrying the signal
				const unsigned int nrn_id,  //!< the 6-bit neuron address of the sender neuron
				l1_source from_where        //!< the source of this signal: one of [from_top,  from_bottom, from_hbus]
				) const
{
    LOG4CXX_TRACE(logger, name << " l1_behav_V2::process_vbus() vbus: " << vbus << " nrn_id: " << nrn_id << " from_where: " << from_where );
	if (from_where ==  from_hbus ) {
		// check top and bottom repeaters
		process_repeater_top( vbus, nrn_id, from_vbus );
		process_repeater_bottom( vbus, nrn_id, from_vbus );
	}
	else if (from_where == from_top) {
		// check crossbar and bottom repeater
		int connected_hbus = _connections_vbus_to_hbus.at( vbus );
		if (connected_hbus != -1 )
			process_hbus( connected_hbus, nrn_id, from_vbus );
		process_repeater_bottom( vbus, nrn_id, from_vbus );
	}
	else if (from_where == from_bottom) {
		// check crossbar and top repeater
		int connected_hbus = _connections_vbus_to_hbus.at( vbus );
		if (connected_hbus != -1 )
			process_hbus( connected_hbus, nrn_id, from_vbus );
		process_repeater_top( vbus, nrn_id, from_vbus );
	}
	else {
        LOG4CXX_WARN(logger, name << "l1_behav_V2::process_vbus() was called from " << from_where << "\tbut allowed: [," << from_top << ", " << from_bottom << ", " << from_hbus << "]." );
	}

	bool on_left_side = (vbus < VERTICAL_L1_COUNT);
	// syndriver switches:
	const std::set< unsigned int >& current_vbus_conns = _conns_vbus_to_syndr_switch.at(vbus);
	std::set< unsigned int >::const_iterator it_target_switches = current_vbus_conns.begin();
	for(; it_target_switches != current_vbus_conns.end(); ++it_target_switches) {
		unsigned int switch_id = *it_target_switches;
		// even numbered switch rows go to the right, odd ones go the the left.
		// e.g. if we are on the right side of the hicann, and have an odd switch,
		// this signal feeds a syndriver of this hicann.
		bool to_left = static_cast<bool>(switch_id%2);
		unsigned int syndriver_id = switch_id/2;

        LOG4CXX_TRACE(logger, name << "l1_behav_V2::process_vbus() switch_id " << switch_id << " syndriver_id " << syndriver_id << " on_left_side: " << on_left_side << " to_left: " << to_left );
		// Note: I don't check here, if the port of the neighbour anncore is bound.
		// because the syndriver switch should not be enabled, if there is no neighbour.
		if ( on_left_side ) {
			if ( to_left ) {
				if ( auto ptr = anncore_if_left.lock()) {
					ptr->recv_pulse( syndriver_id + 2*_num_syndr_per_block , nrn_id ); 
				}
			}
			else {
				if ( auto ptr = anncore_if.lock() ) {
					ptr->recv_pulse( syndriver_id, nrn_id );
				}
			}
		}
		else {    // on right side
			if ( to_left ) {
				if (auto ptr = anncore_if.lock()) {
					ptr ->recv_pulse( syndriver_id + 2*_num_syndr_per_block, nrn_id );
				}
			}
			else {
				if ( auto ptr = anncore_if_right.lock() ) {
					ptr->recv_pulse( syndriver_id, nrn_id );
				}
			}
		}
	}
}

void l1_behav_V2::process_hbus(
				const unsigned int hbus,    //!< horizontal bus carrying the signal
				const unsigned int nrn_id,  //!< the 6-bit neuron address of the sender neuron
				l1_source from_where        //!< the source of this signal: one of [from_left,  from_right, from_vbus]
				) const
{
    LOG4CXX_TRACE(logger, name << " l1_behav_V2::process_hbus: " << hbus << ", " << nrn_id );
	if (from_where == from_vbus) {
		process_repeater_left( hbus, nrn_id, from_hbus );
		process_repeater_right( hbus, nrn_id, from_hbus );
	}
	else if ( from_where == from_left ) {
		// check crossbar and right repeater
		int connected_vbus = _connections_hbus_to_vbus.at( hbus );
		if (connected_vbus != -1 )
			process_vbus( connected_vbus, nrn_id, from_hbus );
		process_repeater_right( hbus, nrn_id, from_hbus );
	}
	else if ( from_where == from_right) {
		// check crossbar and left repeater
		int connected_vbus = _connections_hbus_to_vbus.at( hbus );
		if (connected_vbus != -1 )
			process_vbus( connected_vbus, nrn_id, from_hbus );
		process_repeater_left( hbus, nrn_id, from_hbus );
	}
	else {
        LOG4CXX_WARN(logger, name << "l1_behav_V2::process_hbus() was called from " << from_where << "\tbut allowed: [," << from_right << ", " << from_left << ", " << from_vbus << "]." );
	}
}

void l1_behav_V2::process_repeater_right( const unsigned int hbus, const unsigned int nrn_id, l1_source from_where) const
{
    LOG4CXX_TRACE(logger, name << " l1_behav_V2::process_repeater_right() was called from " << from_where << " hbus:" << hbus << " time:" << sc_simulation_time() );
	// for even numbered horizontal buses, the right repeaters work
	if (from_where == from_hbus ) {
        LOG4CXX_TRACE(logger, " ... got signal from hbus" );
		// shift bus before checking repeaters:
		unsigned int hbus_tmp = shift_hbus( hbus, _hor_shift_to_right );
		bool forward_to_right_neighbour = true;
		if ( !(hbus_tmp%2) ) { // repeater is on this hicann
            LOG4CXX_TRACE(logger, " ... repeater is on this hicann" );
			unsigned int repeater_id = hbus_tmp/2;
			forward_to_right_neighbour = false;
			// check repeater
			if ( _direction_rep_r[ repeater_id ] == forward ) {
				forward_to_right_neighbour = true;
                LOG4CXX_TRACE(logger, " ... direction of repeater is 'forward'" );
			}
		}
		// forward event to right neighbour, check if there is a neighbour HICANN
		// here, we have to check this, compared to anncore_if_right/left in process_vbus

        LOG4CXX_TRACE(logger, " ... forwarding=" << forward_to_right_neighbour << " neighbor=" << l1_nb_right );

		if (forward_to_right_neighbour && ( l1_nb_right != NULL ) ) {
			l1_nb_right->rcv_pulse_from_left(hbus_tmp, nrn_id);
		}
	}
	else if (from_where == from_right) {
		bool forward_to_hbus = true;
		if ( !(hbus%2) ) { // repeater is on this hicann
			forward_to_hbus = false;
			unsigned int repeater_id = hbus/2;
			// check repeater
			if ( _direction_rep_r[ repeater_id ] == backward )
				forward_to_hbus = true;
		}
        LOG4CXX_TRACE(logger, " ... forward_to_hbus=" << forward_to_hbus );
		if ( forward_to_hbus ) {
			process_hbus( shift_hbus( hbus, -_hor_shift_to_right), nrn_id, from_right );
		}
	}
	else {
        LOG4CXX_WARN(logger, name << " l1_behav_V2::process_repeater_right() was called from " << from_where << "\tbut allowed: [," << from_right << ", " << from_hbus << "]." );
	}
}

void l1_behav_V2::process_repeater_left(
				const unsigned int hbus,    //!< horizontal bus carrying the signal
				const unsigned int nrn_id,  //!< the 6-bit neuron address of the sender neuron
				l1_source from_where        //!< the source of this signal: one of [from_hbus,  from_left]
				) const
{
    LOG4CXX_TRACE(logger, name << " l1_behav_V2::process_repeater_left() " << from_where << " hbus:" << hbus << " time:" << sc_simulation_time() );
	// for odd numbered horizontal buses, the left repeaters work
	if (from_where == from_hbus ) {
        LOG4CXX_TRACE(logger, " ... got signal from hbus" );
		bool forward_to_left_neighbour = true;
		if ( hbus%2 ) { // repeater is on this hicann, if odd
            LOG4CXX_TRACE(logger, " ... repeater is on this hicann" );
			unsigned int repeater_id = hbus/2;
			forward_to_left_neighbour = false;
			// check repeater
			if ( _direction_rep_l[ repeater_id ] == forward ) {
				forward_to_left_neighbour = true;
                LOG4CXX_TRACE(logger, " ... direction of repeater is 'forward'" );
			}
		}
		// forward event to left neighbour, check if there is a neighbour HICANN
		// here, we have to check this, compared to anncore_if_right/left in process_vbus
        LOG4CXX_TRACE(logger, " ... forwarding=" << forward_to_left_neighbour << " neighbor=" << l1_nb_left );
		if (forward_to_left_neighbour && ( l1_nb_left != NULL ) )
			l1_nb_left->rcv_pulse_from_right(hbus, nrn_id);
	}
	else if (from_where == from_left) {
		bool forward_to_hbus = true;
		if ( hbus%2 ) { // repeater is on this hicann
			forward_to_hbus = false;
			unsigned int repeater_id = hbus/2;
			// check repeater
			if ( _direction_rep_l[ repeater_id ] == backward )
				forward_to_hbus = true;
		}
        LOG4CXX_TRACE(logger, " ... forward_to_hbus=" << forward_to_hbus );
		if ( forward_to_hbus ) {
			process_hbus( hbus, nrn_id, from_left);
		}
	}
	else {
        LOG4CXX_WARN(logger, name <<"l1_behav_V2::process_repeater_left() was called from " << from_where << "\tbut allowed: [," << from_left << ", " << from_hbus << "]." );
	}
}

void l1_behav_V2::process_repeater_top(
				const unsigned int vbus,    //!< vertical bus carrying the signal
				const unsigned int nrn_id,  //!< the 6-bit neuron address of the sender neuron
				l1_source from_where        //!< the source of this signal: one of [from_vbus,  from_top]
				) const
{
	// for odd numbered vertical buses, the top repeaters work
	if (from_where == from_vbus ) {
		// shift bus before checking repeaters:
		unsigned int vbus_tmp = shift_vbus( vbus, _ver_shift_to_top );
		bool forward_to_top_neighbour = true;
		if ( vbus_tmp%2 ) { // repeater is on this hicann when odd
			unsigned int repeater_id = vbus_tmp/2;
			forward_to_top_neighbour = false;
			// check repeater
			if ( _direction_rep_t[ repeater_id ] == forward )
				forward_to_top_neighbour = true;
		}
		// forward event to top neighbour, check if there is a neighbour HICANN
		// here, we have to check this, compared to anncore_if_right/left in process_vbus
		if (forward_to_top_neighbour && ( l1_nb_top != NULL ) )
			l1_nb_top->rcv_pulse_from_bottom(vbus_tmp, nrn_id);
	}
	else if (from_where == from_top) {
		// we first check the repeaters, and then shift the bus
		bool forward_to_vbus = true;
		if ( vbus%2 ) { // repeater is on this hicann, if odd
			forward_to_vbus = false;
			unsigned int repeater_id = vbus/2;
			// check repeater
			if ( _direction_rep_t[ repeater_id ] == backward )
				forward_to_vbus = true;
		}
		if ( forward_to_vbus ) {
			process_vbus( shift_vbus( vbus, -_ver_shift_to_top), nrn_id, from_top);
		}
	}
	else {
        LOG4CXX_WARN(logger, name << "l1_behav_V2::process_repeater_top() was called from " << from_where << "\tbut allowed: [," << from_top << ", " << from_vbus << "]." );
	}
}

void l1_behav_V2::process_repeater_bottom(
				const unsigned int vbus,    //!< vertical bus carrying the signal
				const unsigned int nrn_id,  //!< the 6-bit neuron address of the sender neuron
				l1_source from_where        //!< the source of this signal: one of [from_vbus,  from_bottom]
				) const
{
	// for even numbered vertical buses, the bottom repeaters work
	if (from_where == from_vbus ) {
		bool forward_to_bottom_neighbour = true;
		if ( !(vbus%2) ) { // repeater is on this hicann when even 
			unsigned int repeater_id = vbus/2;
			forward_to_bottom_neighbour = false;
			// check repeater
			if ( _direction_rep_b[ repeater_id ] == forward )
				forward_to_bottom_neighbour = true;
		}
		// forward event to bottom neighbour, check if there is a neighbour HICANN
		// here, we have to check this, compared to anncore_if_right/left in process_vbus
		if (forward_to_bottom_neighbour && ( l1_nb_bottom != NULL ) )
			l1_nb_bottom->rcv_pulse_from_top(vbus, nrn_id);
	}
	else if (from_where == from_bottom) {
		// we first check the repeaters, and then shift the bus
		bool forward_to_vbus = true;
		if ( !(vbus%2) ) { // repeater is on this hicann, if even
			forward_to_vbus = false;
			unsigned int repeater_id = vbus/2;
			// check repeater
			if ( _direction_rep_b[ repeater_id ] == backward )
				forward_to_vbus = true;
		}
		if ( forward_to_vbus ) {
			process_vbus( vbus, nrn_id, from_bottom);
		}
	}
	else {
        LOG4CXX_WARN(logger, name <<  "l1_behav_V2::process_repeater_bottom() was called from " << from_where << "\tbut allowed: [," << from_bottom << ", " << from_vbus << "]." );
	}
}

void l1_behav_V2::rcv_pulse_from_left(
		unsigned int hbus_id,
		unsigned int nrn_id
		) const
{
	process_repeater_left( hbus_id, nrn_id, from_left );
}

void l1_behav_V2::rcv_pulse_from_right(
		unsigned int hbus_id,
		unsigned int nrn_id
		) const
{
	process_repeater_right( hbus_id, nrn_id, from_right);
}

void l1_behav_V2::rcv_pulse_from_top(
		unsigned int vbus_id,
		unsigned int nrn_id
		) const
{
	process_repeater_top( vbus_id, nrn_id, from_top);
}

void l1_behav_V2::rcv_pulse_from_bottom(
		unsigned int vbus_id,
		unsigned int nrn_id
		) const
{
	process_repeater_bottom( vbus_id, nrn_id, from_bottom);
}

void l1_behav_V2::rcv_pulse_from_this_hicann(
		unsigned int spl1_rep_id,
		unsigned int nrn_id
		) const
{
    LOG4CXX_TRACE(logger, name << " rcv_pulse_from_this_hicann: spl1_rep_id: " << spl1_rep_id << " nrn_id: " << nrn_id << ")" );

	// get id of real repeater and corresponding hbus
	unsigned int repeater_id = 4*spl1_rep_id;
	unsigned int hbus_id = repeater_id*2 + 1;

	// get direction of that repeater
	direction current_spl1_direction = _direction_rep_l[ repeater_id ];

	if (current_spl1_direction == spl1_int ){
		// this hbus, by saying "from_left" we prevent a 2nd check in process_repeater_left()
		process_hbus( hbus_id, nrn_id, from_left );
	}
	else if (current_spl1_direction == spl1_ext ){
		// if there is a left neighbour, we send it there.
		if(l1_nb_left != NULL)
			l1_nb_left->rcv_pulse_from_right(hbus_id, nrn_id);
	}
	else if (current_spl1_direction == spl1_int_and_ext ){
		// this hbus and left neighbour
		process_hbus( hbus_id, nrn_id, from_left );
		if(l1_nb_left != NULL)
			l1_nb_left->rcv_pulse_from_right(hbus_id, nrn_id);
	}
	else {
		// remark: the following may happen, thats no problem
        LOG4CXX_DEBUG(logger, name << "l1_behav_V2::rcv_pulse_from_this_hicann() was called, but the config of the corresponding  spl1-repeater is " << direction_s[current_spl1_direction] );
	}
}

void l1_behav_V2::initialize_topologies ()
{
	// Syndriver Switches
	size_t group_len_h = 4;
	size_t group_len_v = 2;
	size_t step_h = 32;
	size_t offset = 28; // first group starts at (0,28)
	int sign = -1; // groups move from higher to lower vbus numbers
	_topology_ss_tl.resize(2*_num_syndr_per_block, std::vector<unsigned int>( _num_syndr_switches_per_row ) );
	_topology_ss_tr.resize(2*_num_syndr_per_block, std::vector<unsigned int>( _num_syndr_switches_per_row ) );
	_topology_ss_bl.resize(2*_num_syndr_per_block, std::vector<unsigned int>( _num_syndr_switches_per_row ) );
	_topology_ss_br.resize(2*_num_syndr_per_block, std::vector<unsigned int>( _num_syndr_switches_per_row ) );

	for( size_t i_row = 0; i_row < 2*_num_syndr_per_block; ++i_row ) {
		unsigned int row_offset = (sign*(i_row / group_len_v)*group_len_h + offset)%step_h;
		for( size_t i_sw = 0; i_sw < _num_syndr_switches_per_row; ++i_sw) {
			unsigned int vport = row_offset + (i_sw/group_len_h)*step_h + i_sw%group_len_h;
			_topology_ss_tl[i_row][i_sw] = vport;
			_topology_ss_tr[i_row][i_sw] = vport + VERTICAL_L1_COUNT;
			_topology_ss_bl[i_row][i_sw] = vport;
			_topology_ss_br[i_row][i_sw] = vport + VERTICAL_L1_COUNT;
		}
	}

	// Crossbar Switches
	group_len_v = 2;
	step_h = 32;
	_topology_cb_l.resize(HORIZONTAL_L1_COUNT, std::vector<unsigned int>( _num_cb_switches_per_row ) );
	_topology_cb_r.resize(HORIZONTAL_L1_COUNT, std::vector<unsigned int>( _num_cb_switches_per_row ) );
	for( size_t i_row = 0; i_row < HORIZONTAL_L1_COUNT; ++i_row ) {
		unsigned int row_offset = (i_row / group_len_v)%step_h;
		for( size_t i_sw = 0; i_sw < _num_cb_switches_per_row; ++i_sw) {
			unsigned int vport = row_offset + i_sw * step_h;
			_topology_cb_l[i_row][i_sw] = vport;
			_topology_cb_r[i_row][i_sw] = vport + VERTICAL_L1_COUNT;
		}
	}
}
void
l1_behav_V2::set_repeater_config(ESS::RepeaterLocation repeater_location, unsigned int rep_id, unsigned char config)
{
    LOG4CXX_DEBUG(logger, name << " l1_behav_V2::set_repeater_config(), location = " << repeater_location
						 << " rep id " << rep_id
						 << " config " << (int)config );
	switch( repeater_location ) {
		case ESS::REP_L:
			_config_rep_l.at( rep_id ) = config;
			if (rep_id % 4 > 0 ) // normal repeater
				_direction_rep_l.at( rep_id ) = determine_direction( config );
			else  // SPL1 Repeater
				_direction_rep_l.at( rep_id ) = determine_direction_spl1( config );
			break;
		case ESS::REP_R:
			_config_rep_r.at( rep_id ) = config;
			_direction_rep_r.at( rep_id ) = determine_direction( config );
			break;
		case ESS::REP_UL:
			_config_rep_tl.at( rep_id ) = config;
			_direction_rep_t.at( rep_id ) = determine_direction( config );
			break;
		case ESS::REP_UR:
			_config_rep_tr.at( rep_id ) = config;
			_direction_rep_t.at( rep_id + VERTICAL_L1_COUNT/2 ) = determine_direction( config );
			break;
		case ESS::REP_DL:
			_config_rep_bl.at( rep_id ) = config;
			_direction_rep_b.at( rep_id ) = determine_direction( config );
			break;
		case ESS::REP_DR:
			_config_rep_br.at( rep_id ) = config;
			_direction_rep_b.at( rep_id + VERTICAL_L1_COUNT/2 ) = determine_direction( config );
			break;
	}
}
unsigned char
l1_behav_V2::get_repeater_config(
		ESS::RepeaterLocation repeater_location,
		unsigned int rep_id
		) const
{
	switch( repeater_location ) {
		case ESS::REP_L:
			return _config_rep_l.at( rep_id );
			break;
		case ESS::REP_R:
			return _config_rep_r.at( rep_id );
			break;
		case ESS::REP_UL:
			return _config_rep_tl.at( rep_id );
			break;
		case ESS::REP_UR:
			return _config_rep_tr.at( rep_id );
			break;
		case ESS::REP_DL:
			return _config_rep_bl.at( rep_id );
			break;
		case ESS::REP_DR:
			return _config_rep_br.at( rep_id );
			break;
	    default: 
            LOG4CXX_ERROR(logger, name << " get_repeater_config called with invalid repeater_location!" );
            throw std::runtime_error("l1_behav_V2: get_repeater_config called with invalid repeater_location!" );
    }
}

l1_behav_V2::direction
l1_behav_V2::determine_direction( unsigned char config_byte ) {
	unsigned int cfg = config_byte >> 6; // bit 7: enable, bit 6: direction (1=backward, 0=forward)
	if (cfg == 3)
		return backward;
	else if (cfg == 2)
		return forward;
	else
		return off;
}

l1_behav_V2::direction
l1_behav_V2::determine_direction_spl1( unsigned char config_byte ) {
	// PM: FIXME: magic numbers/bits defined locally, cf halbe_to_ess also
	bool recen = static_cast<bool>( (config_byte >> 7) & 1 );
	bool dir = static_cast<bool>( (config_byte >> 6) & 1 );
	bool tinen = static_cast<bool>( (config_byte >> 4) & 1 );

	if ( tinen ) {
		if ( recen ) {
			if ( dir ) {
				return spl1_int_and_ext;
			}
			else {
				return spl1_ext;
			}
		}
		else { // recen = 0
			if ( dir ) {
				return spl1_int;
			}
			else {
				return off;
			}
		}
	}
	else { // tinen = 0
		if ( recen ) {
			if ( dir ) {
				assert (false);
				return off; // unspecified behavior: both drivers are activated, receiver should be ext
			}
			else {
				return forward;
			}
		}
		else { // recen = 0
			if ( dir ) {
				return backward;
			}
			else {
				return off;
			}
		}
	}
}

int l1_behav_V2::print_cfg(
	std::string fn  //!< name of file, to which the config will be printed
	) const
{
    LOG4CXX_DEBUG(logger, name << l1_id << " print_cfg: " << fn );
	assert(fn.size() > 0 );
	std::ostringstream oss;
	oss << "_" << l1_id;
	fn.append(oss.str());
	std::ofstream fs(fn.c_str());

	assert (fs.is_open());

	if (fs.is_open()){
		fs << "# l1_behav cfg l1_id : " << l1_id << "\n";
		fs << "repeater left\n";
		for(size_t n = 0; n<_direction_rep_l.size();++n)
			if (_direction_rep_l[n] != off)
				fs << n << ":\t" <<  direction_s[_direction_rep_l[n]] << "\n";
		fs << "repeater right\n";
		for(size_t n = 0; n<_direction_rep_r.size();++n)
			if (_direction_rep_r[n] != off)
				fs << n << ":\t" <<  direction_s[_direction_rep_r[n]] << "\n";
		fs << "repeater top\n";
		for(size_t n = 0; n<_direction_rep_t.size();++n)
			if (_direction_rep_t[n] != off)
				fs << n << ":\t" <<  direction_s[_direction_rep_t[n]] << "\n";
		fs << "repeater bottom\n";
		for(size_t n = 0; n<_direction_rep_b.size();++n)
			if (_direction_rep_b[n] != off)
				fs << n << ":\t" <<  direction_s[_direction_rep_b[n]] << "\n";
		fs << "\n";

		fs << "Crossbar Connections:\n";
		fs << "For a better overview, each connection is stated twice. The following doesnt mean the direction in which the pulse go, but just the information that the switch between the two is enabled.:\n";
		fs << "From Horizontal to Vertical buses:\n";
		for(size_t n = 0; n<_connections_hbus_to_vbus.size();++n)
			if (_connections_hbus_to_vbus[n] != -1)
				fs << n << " <-> " <<  _connections_hbus_to_vbus[n] << "\n";
		fs << "From Vertical to Horizontal buses:\n";
		for(size_t n = 0; n<_connections_vbus_to_hbus.size();++n)
			if (_connections_vbus_to_hbus[n] != -1)
				fs << n << " <-> " <<  _connections_vbus_to_hbus[n] << "\n";
		fs << "\n";

		fs << "Syndriver Switches:\n";
		fs << "EVEN numbered syndriver switches go to a syndriver at the RIGHT of the vbus.\n";
		fs << "ODD numbered syndriver switches go to a syndriver at the LEFT of the vbus.\n";
		for(size_t n = 0; n<_conns_vbus_to_syndr_switch.size();++n){
			if (_conns_vbus_to_syndr_switch[n].size() > 0) {
				fs << n << ": ";
				for( std::set<unsigned int>::iterator it_syndr = _conns_vbus_to_syndr_switch[n].begin(); it_syndr != _conns_vbus_to_syndr_switch[n].end(); ++it_syndr){
					fs << *it_syndr << ", ";
				}
				fs << "\n";
			}
		}
		fs << "\n";
		fs.close();
                assert (!fs.is_open());
		return 1;
	}
	else {
        LOG4CXX_WARN(logger, name << l1_id << ": could not open file: " << fn.c_str() );
		fs.close();
		return 0;
	}
}

void l1_behav_V2::test()
{
	// test determine direction

	unsigned char t_config = 0;
	unsigned char recen = 1 << 7;
	unsigned char dir  = 1 << 6;
	unsigned char tinen = 1 << 4;
	std::vector<unsigned char> cases;
	for ( size_t i = 0; i<2;++i){
		unsigned char test = t_config | (i*recen);
		for ( size_t j = 0; j<2;++j){
			unsigned char test_2 = test | (j*dir);
			for ( size_t k = 0; k<2;++k){
			unsigned char test_3 = test_2 | (k*tinen);
				cases.push_back(test_3);
			}
		}
	}
    LOG4CXX_INFO(logger, name << " testing the direction conversion:" << "1.) normal repeaters." );
	for ( size_t i = 0; i< cases.size(); ++i) 
        LOG4CXX_INFO(logger, "in: \t"  << (unsigned int) cases[i] << "\tout:\t" << direction_s[ determine_direction(cases[i])] );

    LOG4CXX_INFO(logger, "2.) spl1 repeaters." );
	for ( size_t i = 0; i< cases.size(); ++i) 
        LOG4CXX_INFO(logger, "in: \t"  << (unsigned int) cases[i] << "\tout:\t" << direction_s[ determine_direction_spl1(cases[i])] );

}
