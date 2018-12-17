//_____________________________________________
// Author       :	Johannes Fieres
// E-Mail	:	fieres@kip.uni-heidelberg.de
//
// Last Change	:	2009/07/03 bvogging
// Module Name  :   anncore
// Filename     :   anncore.cpp
// Project Name	:   p_facets/s_ancmpw1
// Description	:   ANNCORE more detailed model
//
//_____________________________________________
// test
#include "anncore_behav.h"


#include <set>
#include <iostream>
#include <boost/random.hpp>
#include <boost/random/normal_distribution.hpp>
#include <stdexcept>

#include "hw_neuron.h"
#include "hw_neuron_ADEX.h"
#include "hw_neuron_ADEX_clocked.h"
#include "hw_neuron_IFSC.h"

#include "jf_utils.h"

//#include "Tools/stage2/NN_def.h" // for VOID_NEURON
#include "calibtic/HMF/SynapseDecoderDisablingSynapse.h"


#include <log4cxx/logger.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("ESS.HICANN");

anncore_behav::anncore_behav(const sc_module_name& anncore_i, short anncore_id, ess::mutexfile& spike_rcx_file, uint8_t PLL_period_ns, bool enable_spike_debugging) :
	sc_module(anncore_i),
    _anncore_id(anncore_id),
    _spike_debugging(enable_spike_debugging),
    _hw_neuron_count(0),
	_hw_neurons_created(false),
    _bio_neuron_count(0),
    _clock("clock", 4*PLL_period_ns, SC_NS),    //Factor 4 because the HICANN_SLOW_Clock responsible for the current stimuli is 4 times slower than the PLL_frequency
    _fg_stim(),
    _global_cnt(0), 
    spike_rx_file(spike_rcx_file)
{
    if(ROWS_PER_SYNDRIVER!=2)
    {
        LOG4CXX_ERROR(logger, "anncore_behav::anncore_behav: ROWS_PER_SYNDRIVER!=2" );
        throw std::runtime_error("anncore_behav::anncore_behav: ROWS_PER_SYNDRIVER!=2");
    }
    
    //initializing systemc process
	SC_METHOD(check_for_stimchange);
	dont_initialize();
	sensitive << _clock.posedge_event();

    // total number of synapses
    int tot_synapses=
			SYN_PER_ROW * // synapses per row (=array width)
			ROWS_PER_SYNDRIVER * // array rows per syndrivers
			SYNDRIVERS * // number of syndrivers per side
			2; // two sides
    
    LOG4CXX_INFO(logger, name() << " initializing " << tot_synapses << " synapses" );

    // init weights with value 0 !!! was initialized with 1 before! WTF
    _weights.resize(tot_synapses,0);
    _addresses.resize(tot_synapses,-1);
	_weight_distortions.resize(tot_synapses,1.);

    // build and configure synapse drivers and syndriver mirrors
    _syndrivers.reserve(2*SYNDRIVERS);
    _syndriver_mirrors.resize(2*SYNDRIVERS);

    // left side
    for(int i=0;i<SYNDRIVERS;i++)
    {
        LOG4CXX_DEBUG(logger, name() << " creating syndrivers: " << i << " syndriver " << _syndrivers.size() 
				  << " \tgets synapse offset " << ROWS_PER_SYNDRIVER*SYN_PER_ROW*(i*2) 
				  << " row=" << ROWS_PER_SYNDRIVER*(i*2) );
		_syndrivers.push_back(
				syndriver(
                    _anncore_id,
                    i,
					&_weights[ROWS_PER_SYNDRIVER*SYN_PER_ROW*(i*2)],
					&_weight_distortions[ROWS_PER_SYNDRIVER*SYN_PER_ROW*(i*2)],
					&_addresses[ROWS_PER_SYNDRIVER*SYN_PER_ROW*(i*2)],
					&_denmems, i>=SYNDRIVERS/2));
    }

    // right side
    for(int i=0;i<SYNDRIVERS;i++)
    {
        LOG4CXX_DEBUG(logger, name() << " creating syndrivers: " << i << " syndriver " << _syndrivers.size() 
				  << " gets synapse offset "
				  << ROWS_PER_SYNDRIVER*SYN_PER_ROW*(i*2 + 1) 
				  << " row=" << ROWS_PER_SYNDRIVER*(i*2 + 1) );
		_syndrivers.push_back(
				syndriver(
                    _anncore_id,
                    i,
					&_weights[ROWS_PER_SYNDRIVER*SYN_PER_ROW*(i*2+1)],
					&_weight_distortions[ROWS_PER_SYNDRIVER*SYN_PER_ROW*(i*2+1)],
					&_addresses[ROWS_PER_SYNDRIVER*SYN_PER_ROW*(i*2+1)],
					&_denmems, i>=SYNDRIVERS/2));
    }   
}

anncore_behav::~anncore_behav()
{
    for(unsigned int i=0; i< _compound_neurons.size(); ++i) {
		delete _compound_neurons[i];
    }
    LOG4CXX_INFO(logger, "anncore_behav destructor id: " << _anncore_id);
}

//Functions for Stimcurrent

void anncore_behav::check_for_stimchange()
{
    if(_global_cnt == 0)
    {
        for(size_t ii = 0; ii < _fg_stim.size(); ++ii)
            init_stim(_fg_stim[ii]);
    }
    else
    {
        for(size_t ii = 0; ii < _fg_stim.size(); ++ii)
           update_stim(_fg_stim[ii]); 
    }
    _global_cnt++;
}

void anncore_behav::init_stim(CurrentStimulus & stim)
{
     if(stim.enable)
     {
        stim.position = 0;
        uint16_t current = stim.Currents[stim.position];
        if(current != 0)
        {
            double technical = current * stim.scaling;
            for (auto nrn : stim.neurons)
            {
                _denmems[nrn]->inputCurrent(technical);
            }
        }
        stim.clk_counter = 1;
     }
}

void anncore_behav::update_stim(CurrentStimulus & stim)
{
    if(stim.enable)
    {
        if (stim.clk_counter == stim.period)
        {
            stim.clk_counter = 0;
            /* ECM: there was additionally (!) this line:
             * size_t old_position = stim.position;
             */
            uint16_t old_current = stim.Currents[stim.position];
            
            if(stim.position == (stim.Currents.size() - 1) )
            {
                if(stim.continuous == true)
                    stim.position = 0;
                else
                {
                    stim.enable = false;
                    for (auto nrn : stim.neurons) //reset current, is this what happens in HW?
                    {
						_denmems[nrn]->inputCurrent(0.);
                    }
                }
            }
            else
                stim.position++;
            
            uint16_t new_current = stim.Currents[stim.position];
            if(old_current != new_current)
            {
                //TODO log time
                LOG4CXX_TRACE(logger, name() << "current stimulus value changed from " << old_current << " to " << new_current);
                double technical = new_current * stim.scaling;
                for (auto nrn : stim.neurons)
                {
                    LOG4CXX_TRACE(logger, name() << "current stimulus injected into neuron " << nrn);
					_denmems[nrn]->inputCurrent(technical);
                }
            }
        }
        else
            stim.clk_counter++;
    }
}


/** Use this fct to send a pulse with addr to the syndriver_id-th syndriver*/
void anncore_behav::recv_pulse(unsigned int syndr, unsigned int addr)
{
    LOG4CXX_TRACE(logger,name() << ":recv_pulse: synapse driver " << syndr << " received a pulse with address " << addr);

    if(_spike_debugging)
    {
        FILE *fpt = spike_rx_file.aquire_fp();
	    fprintf(fpt,"%.8i %.3i %.3i %.2i\n",(uint)sc_simulation_time(),_anncore_id,(uint)syndr, (unsigned char)addr);
	    fflush(fpt);
	    spike_rx_file.release_fp(fpt);
    }

	// only do stuff if the driver receives L1 Input (hence get_l1() == true)
    if( _syndrivers[syndr].get_l1() )
    {
        int num_pulses = 0;
        // check if this syndriver is enabled and send a pulse if it does
        if( _syndrivers[syndr].get_enable() )
        {
            num_pulses = _syndrivers[syndr].pulse(addr);
            LOG4CXX_TRACE(logger,name() << ":recv_pulse: synapse driver " << syndr << " received a pulse via L1 and sent " << num_pulses << " pulses");
        }
        
        // mirror the signal
        // same procedure in top and bottom block!
        {
            // upward direction (from lower to higher syndriver IDs)
            for(int k = syndr + 1;
	        	k < 2 * SYNDRIVERS &&           // make sure to stay in range
                k != SYNDRIVERS  &&             // dont mirror from syndriver 55 -> 56 (i.e. stay in the upper left block)
                k != SYNDRIVERS / 2 &&          // dont mirror from syndriver 111 -> 112 (i.e. dont go from left to right)
                k != 3*SYNDRIVERS / 2 &&        // dont mirror from syndriver 167 -> 168 (i.e stay in upper right block)
                _syndrivers[k-1].get_mirror();  // check if mirroring is active in lower syndriver
	        	k += 1)
	        {
	        	if( _syndrivers[k].get_enable() )
                {
                    num_pulses = _syndrivers[k].pulse(addr);
                    LOG4CXX_TRACE(logger,name() << ":recv_pulse: synapse driver " << k << " received a upward mirrored pulse from driver "   << syndr << " and sent " << num_pulses << " pulses.");
                }
                else
                {
                    LOG4CXX_TRACE(logger,name() << ":recv_pulse: synapse driver " << k << " received a upward mirrored pulse from driver " << syndr << " but is disabled, so no pulses were sent.");
                }
            }
	        // downward direction (from higher to lower syndriver IDs)
                for(int kk = syndr - 1;
	        	kk >= 0 &&                      // make sure to stay in range
                kk != SYNDRIVERS / 2 - 1 &&     // dont mirror from syndriver 56 -> 55 (i.e. stay in the lower left block)
                kk != SYNDRIVERS  - 1 &&     // dont mirror from syndriver 112 -> 111 (i.e. dont go from right to left)
                kk != 3*SYNDRIVERS / 2 - 1 &&   // dont mirror from syndriver 168 -> 167 (i.e. stay in lower right block)
                _syndrivers[kk].get_mirror();
	        	kk -= 1)
	        {
	        	if( _syndrivers[kk].get_enable() )
                {
	        	    num_pulses = _syndrivers[kk].pulse(addr);
                    LOG4CXX_TRACE(logger,name() << ":recv_pulse: synapse driver " << kk << " received a downward mirrored pulse from driver "   << syndr << " and sent " << num_pulses << " pulses.");
	            }
                else
                {
                    LOG4CXX_TRACE(logger,name() << ":recv_pulse:  synapse driver " << kk << " received a downward mirrored pulse from driver "   << syndr << " but is disabled, so no pulses were sent.");
                }
            }
        }
    }
    else
    {
        LOG4CXX_TRACE(logger,name() << ":recv_pulse: synapse driver " << syndr << " received a pulse via L1 but is not enabled");
    }
}

void anncore_behav::configWeight(int x, int y, sc_uint<SYN_NUMCOLS_PER_ADDR*SYN_COLDATA_WIDTH> weight)
{
    if(x<0 || x >= pow((double)2,(double)SYN_COLADDR_WIDTH) ||
       y<0 || y >= SYNDRIVERS * ROWS_PER_SYNDRIVER)
    {
        LOG4CXX_ERROR(logger, "anncore_behav::programWeight(x,y,w): out of bounds (x=" << x << ", y=" << y << ")");
		throw std::runtime_error("anncore_behav::programWeight(x,y,w): out of bounds ");
    }

    for(uint i=0;i<SYN_NUMCOLS_PER_ADDR;i++)
			_weights[x + i*pow((double)2,(double)SYN_COLADDR_WIDTH) + y*SYN_PER_ROW] =
			  (sc_uint<SYN_COLDATA_WIDTH>)(weight>>(SYN_COLDATA_WIDTH*(SYN_NUMCOLS_PER_ADDR-(i+1))));
}

const sc_uint<SYN_NUMCOLS_PER_ADDR*SYN_COLDATA_WIDTH> anncore_behav::getWeight(int x, int y) const
{
    if(x<0 || x >= pow((double)2,(double)SYN_COLADDR_WIDTH) ||
       y<0 || y >= SYNDRIVERS * ROWS_PER_SYNDRIVER)
    {
        LOG4CXX_ERROR(logger, "anncore_behav::getWeight(x,y,w): out of bounds (x=" << x << ", y=" << y << ")");
        throw std::runtime_error("anncore_behav::getWeight(x,y,w): out of bounds ");
    }

		// assemble result vector
	const sc_uint<SYN_NUMCOLS_PER_ADDR*SYN_COLDATA_WIDTH> weights = 0;

	/* ---> old stuff, this should return just one weight!!!
	for(uint i=0;i<SYN_NUMCOLS_PER_ADDR;i++)
			weights += (_weights[x + i*pow((double)2,(double)SYN_COLADDR_WIDTH) + y*SYN_PER_ROW]) << (SYN_COLDATA_WIDTH*(SYN_NUMCOLS_PER_ADDR-(i+1)));
	*/
	return weights;
}

const std::vector<char>& anncore_behav::getWeights() const 
{
    return _weights;
}

void anncore_behav::configWeights(const std::vector<char>& weights)
{
    if(weights.size()!=this->_weights.size())
    {
        LOG4CXX_ERROR(logger, "anncore_behav::configWeigths(): wrong size! expected " <<(int)this->_weights.size()<< " weights, got "<< (int)weights.size()<<"!");
        throw std::runtime_error("anncore_behav::configWeigths(): wrong size!");
    }
    else
    {
		// do explicit loop since don't know whether vector::operator= will re-allocate, and don't know
		// whether std::copy() is supported by possible basics:: implementation.
		for(size_t i = 0; i < weights.size(); ++i)
        {
            if (weights[i] != 0)
                LOG4CXX_DEBUG(logger, "anncore_behav::configWeigths(): setting weight of synapse " << i << " to " << (int)weights[i]);

            _weights[i] = weights[i];
        }
    }
    //printf("anncore(%s)::programWeights: programmed weights!\n",name());
}


void anncore_behav::configAddressDecoders(const std::vector<char> &addr)
{
    if(addr.size()!=this->_addresses.size())
    {
        LOG4CXX_ERROR(logger, "anncore_behav::configAddressDecoders(): wrong size! expected " <<(int)this->_addresses.size()<< " weights, got "<< (int)addr.size()<<"!");
        throw std::runtime_error("anncore_behav::configAddressDecoders(): wrong size!");
    }
    else
    {
		// do explicit loop since don't know whether vector::operator= will re-allocate, and don't know
		// whether std::copy() is supported by possible basics:: implementation.
		forindex(i,addr)
			this->_addresses[i]=addr[i];
		//printf("%s: programmed addresses for syndriver %d\n",name(),syndriver);
    }
}


bool anncore_behav::getSyndriverMirror(int side, int position) const
{
    return _syndrivers[SYNDRIVERS*side+position].get_mirror();
}


void anncore_behav::configSyndriver(unsigned int syndr_id,
                                    const ESS::syndriver_cfg& syndriver_cfg,
                                    const std::vector<double>& synapse_trafo_up,
                                    const std::vector<double>& synapse_trafo_down,
									double tau_rec,
									double lambda,
									double N_dep,
									double N_fac) {
	//_pre_out[0][0] = preout_0: EVEN synapses in EVEN rows
	//_pre_out[0][1] = preout_2: ODD synapses in EVEN rows
	//_pre_out[1][0] = preout_1: EVEN synapses in ODD rows
	//_pre_out[1][1] = preout_3: ODD synapses in ODD rows
    
    //set the enable bit
    _syndrivers.at(syndr_id).set_enable(syndriver_cfg.enable);
   
    //set the l1 bit
    _syndrivers.at(syndr_id).set_l1(syndriver_cfg.locin);

    //set the mirror bit
    _syndrivers.at(syndr_id).set_mirror(syndriver_cfg.mirror);

    //set the driver decoder values
    _syndrivers.at(syndr_id).set_pre_out(0, syndriver_cfg.bottom_row_cfg.preout_even);
	_syndrivers.at(syndr_id).set_pre_out(2, syndriver_cfg.bottom_row_cfg.preout_odd);
	_syndrivers.at(syndr_id).set_pre_out(1, syndriver_cfg.top_row_cfg.preout_even);
	_syndrivers.at(syndr_id).set_pre_out(3, syndriver_cfg.top_row_cfg.preout_odd);
	//	type_up and type_down must be either 0=excitatory or 1=inhibitory
	char type_up = 0;
	if(!syndriver_cfg.top_row_cfg.senx && syndriver_cfg.top_row_cfg.seni)
		type_up = 1;
	char type_down = 0;
	if(!syndriver_cfg.bottom_row_cfg.senx && syndriver_cfg.bottom_row_cfg.seni)
		type_down = 1;
	_syndrivers[syndr_id].set_syn_type(1,type_up);
	_syndrivers[syndr_id].set_syn_type(0,type_down);

	_syndrivers[syndr_id].set_synapse_trafo(1,synapse_trafo_up);
	_syndrivers[syndr_id].set_synapse_trafo(0,synapse_trafo_down);

	// only for storing the config
	_syndrivers[syndr_id].set_gmax_div(0,0,syndriver_cfg.bottom_row_cfg.gmax_div_x);
	_syndrivers[syndr_id].set_gmax_div(0,1,syndriver_cfg.bottom_row_cfg.gmax_div_i);
	_syndrivers[syndr_id].set_gmax_div(1,0,syndriver_cfg.top_row_cfg.gmax_div_x);
	_syndrivers[syndr_id].set_gmax_div(1,1,syndriver_cfg.top_row_cfg.gmax_div_i);

	_syndrivers[syndr_id].set_sel_Vgmax(0,syndriver_cfg.bottom_row_cfg.sel_Vgmax);
	_syndrivers[syndr_id].set_sel_Vgmax(1,syndriver_cfg.top_row_cfg.sel_Vgmax);

	// config STP
	// stp mode: 0=depression 1=facilitation
	_syndrivers[syndr_id].set_STP(syndriver_cfg.stp_enable, syndriver_cfg.stp_mode, syndriver_cfg.stp_cap,
							tau_rec, lambda, N_dep, N_fac);
}

unsigned int anncore_behav::initializeNeurons(
				const std::map< unsigned int, unsigned int >& firing_denmems_and_addresses,
				const std::map< unsigned int, unsigned int >& denmems2nrns
                )
{
	_bio_neuron_count = firing_denmems_and_addresses.size();
	_hw_neuron_count = 0;
	// instantiate as many hw_neurons as needed
	if ( !_hw_neurons_created ) {
		char buffer[1000];

		std::map< unsigned int, unsigned int >::const_iterator it_fdna =  firing_denmems_and_addresses.begin();
		std::map< unsigned int, unsigned int >::const_iterator it_fdna_end =  firing_denmems_and_addresses.end();

		for(;it_fdna != it_fdna_end; ++it_fdna)
		{
			unsigned int denmem_id = it_fdna->first;
			unsigned int l1_address = it_fdna->second;
            LOG4CXX_DEBUG(logger, "Processing denmem " << denmem_id << " with address " << l1_address);
			snprintf(buffer,sizeof(buffer),"anncore_i%i_compound_neuron_i%i",_anncore_id,denmem_id);
			unsigned int wta_id = (denmem_id%256)/32;

			std::set< unsigned int > connected_denmems;
            getConnectedDenmems(connected_denmems, denmems2nrns, _hw_neuron_count, denmem_id);
            LOG4CXX_DEBUG(logger, "Number of denmem connected to this neuron: " << connected_denmems.size() << ": ");

			CompoundNeuronModule* cmn = new CompoundNeuronModule(buffer,connected_denmems.size(), denmem_id, wta_id, &(this->out_port));
			_compound_neurons.push_back(cmn);
			_firing_denmem_2_compound_neuron[denmem_id] = cmn;

			// connect synapse columns to hw_neurons
			std::set< unsigned int >::iterator it_cd =  connected_denmems.begin();
			std::set< unsigned int >::iterator it_cd_end =  connected_denmems.end();
			size_t id_in_cn = 0; // id within multi-denmem
			for (; it_cd != it_cd_end; ++it_cd) {
				_denmems.at( *it_cd ).reset( new DenmemIF(cmn->mCompoundNeuron, id_in_cn) );
                LOG4CXX_DEBUG(logger, "Denmem " << *it_cd << " connected." );
				if (*it_cd == denmem_id)
					cmn->setFiringDenmem(id_in_cn);
				id_in_cn++;
			}
			++_hw_neuron_count;
		}

		_hw_neurons_created = true;
	} else {
        LOG4CXX_ERROR(logger, "anncore_behav"<< _anncore_id << ": initializeNeurons() called several times. Which is currently not supported.");
        throw std::runtime_error("anncore_behav: initializeNeurons() called several times. Which is currently not supported.");
	}
    if( _hw_neuron_count != _bio_neuron_count )
    {
        LOG4CXX_ERROR(logger, "anncore_behav"<< _anncore_id << ": initializeNeurons(): hw_neuron_count = " << _hw_neuron_count << " != bio_neuron_count = " << _bio_neuron_count );
        throw std::runtime_error("anncore_behav : initializeNeurons(): hw_neuron_count  != bio_neuron_count ");
    }
    return _hw_neuron_count;
}

void anncore_behav::configSingleDenmem( unsigned int denmem_id, ESS::BioParameter & _neuron_parameter)
{
	if ( _denmems.at(denmem_id) ) {

		DenmemParams denmem_params(_neuron_parameter);

		if (denmem_params.delta_T < std::numeric_limits<double>::epsilon() ) {
			denmem_params.delta_T = std::numeric_limits<double>::epsilon();
			LOG4CXX_DEBUG(logger, "anncore_behav::configSingleDenmem() setting delta_T to " << denmem_params.delta_T << " to avoid division by zero" );
		}

		if (denmem_params.tau_w < std::numeric_limits<double>::epsilon() ) {
			denmem_params.tau_w = std::numeric_limits<double>::epsilon();
			LOG4CXX_DEBUG(logger, "anncore_behav::configSingleDenmem() setting tau_w to " << denmem_params.tau_w << " to avoid division by zero" );
		}

		_denmems[denmem_id]->setDenmemParams(denmem_params);
        LOG4CXX_DEBUG(logger, "Denmem " << denmem_id << " configured.");
	}
	else {
        LOG4CXX_WARN(logger, "Trying to configure a denmem (" << denmem_id << "), for which no DenmemIF exists. Not doing anything.");
	}
}

ESS::BioParameter anncore_behav::get_neuron_parameter( unsigned int denmem_id) const
{
	if ( _denmems.at(denmem_id) ) {
		return _denmems[denmem_id]->getDenmemParams().toBioParameter();
	}
	else {
		throw std::runtime_error("anncore_behav:get_neuron_parameter(): trying"
				" to get neuron parameters from a neuron which was not"
				" initialized");
	}
}

unsigned int anncore_behav::get_6_bit_address( unsigned int denmem_id ) const
{

	auto it = _firing_denmem_2_compound_neuron.find(denmem_id);
	if ( it != _firing_denmem_2_compound_neuron.end() ) {
		return it->second->get_6_bit_address();
	}
	else {
		throw std::runtime_error("anncore_behav:get_6_bit_address(): getting 6"
				" bit address only supported for firing denmems.");
	}
}

void anncore_behav::configCompoundNeuron( unsigned int denmem_id, unsigned int addr, bool rec, std::string fn, double dt)
{
	if ( _firing_denmem_2_compound_neuron.find(denmem_id) != _firing_denmem_2_compound_neuron.end() )
	{
		CompoundNeuronModule* cmn = _firing_denmem_2_compound_neuron[denmem_id];
		cmn->init(addr%L1_ADDR_RANGE, rec, fn,dt);
        LOG4CXX_DEBUG(logger, "Hardware Neuron (Denmem) " << denmem_id << "configured.");
	}
	else {
        LOG4CXX_WARN(logger, "Trying to configure a denmem (" << denmem_id << "), for which no hw_neuron exists. Not doing anything.");
	}
}

hw_neuron * anncore_behav::get_neuron(unsigned int nrn)
{
	auto it = _firing_denmem_2_hw_neuron.find(nrn);
	if( it != _firing_denmem_2_hw_neuron.end() ) {
		return it -> second;
    }
    else {  //if i return a NULL-Poiner this will probably segfault, so throw an error
        LOG4CXX_ERROR(logger, "anncore_behav"<< _anncore_id << " get_neuron(): " << " Neuron with id: " << nrn << " was not initialized!");
        throw std::runtime_error("anncore_behav : get_neuron(): trying to get neuron which was not initialized");
    }
}

hw_neuron const* anncore_behav::get_neuron(unsigned int nrn) const
{
	auto it = _firing_denmem_2_hw_neuron.find(nrn);
	if(it != _firing_denmem_2_hw_neuron.end())
    {
		return it -> second;
    }
    else {  //if i return a NULL-Poiner this will probably segfault, so throw a runtime error
        LOG4CXX_ERROR(logger, "anncore_behav"<< _anncore_id << " get_neuron(): " << " Neuron with id: " << nrn << " was not initialized!");
        throw std::runtime_error("anncore_behav : get_neuron(): trying to get neuron which was not initialized");
    }
}

syndriver & anncore_behav::get_syndriver(unsigned int drv)
{
    return _syndrivers[drv];
}

syndriver const& anncore_behav::get_syndriver(unsigned int drv) const
{
    return _syndrivers[drv];
}

void anncore_behav::configCurrentStimulus(ESS::StimulusContainer const& stim, ESS::HICANNSide side, ESS::HICANNBlock block)
{
    //TODO check addresses!
    size_t addr;
    if(side == ESS::HICANNSide::S_LEFT)
    {
        if(block == ESS::HICANNBlock::BL_UP)
            addr = 0;
        else
            addr = 1;
    }
    else
    {
        if(block == ESS::HICANNBlock::BL_UP)
            addr = 2;
        else
            addr = 3;
    }
    const auto& curr_input = stim.Currents;
    for(size_t ii = 0; ii < curr_input.size(); ++ii)
    {
        if(curr_input[ii] != 0)
        {
            _fg_stim.at(addr).enable = true;
            break;
        }
        else
            continue;
    }
    _fg_stim.at(addr).Currents = curr_input;
    _fg_stim.at(addr).period = stim.PulseLength;
    _fg_stim.at(addr).continuous = stim.Continuous;
}

void anncore_behav::setCurrentInput(unsigned int denmem)
{
    if(denmem < 256) // FG_Blocks 0 and 1
    {
        if(denmem%2) //odd denmem -> Block 1
            _fg_stim[1].neurons.push_back(denmem);
        else        //even denmem -> Block 0
            _fg_stim[0].neurons.push_back(denmem);
    }
    else            // FG_Blocks 2 and 3
    {
        if(denmem%2)    // odd denmem -> Block 3
            _fg_stim[3].neurons.push_back(denmem);
        else            // even denmem Block 2
            _fg_stim[2].neurons.push_back(denmem);
    }   
}

//void anncore_behav::handle_spike(const int& bio_neuron, const sc_uint<LD_NRN_MAX>& logical_neuron, const short& addr,  int wta_id )
//{
//	log(Logger::DEBUG2) << "ANNCORE " << _anncore_id << ": Pulse from biological neuron nr "<< bio_neuron << ", logical nrn: " << logical_neuron << " with addr " << addr << " with wta_id = " << wta_id << " at: " << sc_time_stamp() << endl;
//
//	// send to l1: rcv_pulse_hor(int bus_id, int addr). bus_id range: 0..63
//	//l1_if->rcv_pulse_hor(wta2l1bus[wta_id], addr%64);
//
//	// send to l2: l1_pulse(unsigned char l1bus, unsigned char nrnid). l1bus range: 0..7
//	// caution: first parameter is in range 0..7
//	//dnc_if_if->l1_task_if((unsigned char)(wta_id), (unsigned char) addr%64);
//
//}

int anncore_behav::print_cfg(std::string fn)
{
	assert(fn.size() > 0 );
	std::ostringstream oss;
	oss << "_" << _anncore_id;
	fn.append(oss.str());
	std::ofstream fs(fn.c_str());
	if (fs.is_open())
	{
		fs << "anncore_behav cfg anncore_id: " << _anncore_id << "\n";

		if(_bio_neuron_count==0)
		{
			fs << " NO Biological Neurons mapped on this Anncore!\n";
		}
		else
		{
			// Configuration of Syndriver Mirrors:
			fs << "Mirrored Syndrivers: up:= get Input from Syndriver above\n";
			/* ECM (2018-01-11) removed
			 * int mir_count=0;
			 */

			for(unsigned int i = 0; i<_syndrivers.size(); i++)
			{
				if ( _syndrivers[i].get_mirror() ){
					fs << i << " ";
				}
			}

			// fs << "\n" << mir_count << " _syndrivers mirrored\n\n";

			// Number of logical Neurons:
			fs << "Number of logical Neurons(configured HW-Neurons): " <<  _hw_neuron_count << "\n\n";

			// Number of biological Neurons:
			fs << "Number of biological Neurons mapped on this Anncore: " <<  _bio_neuron_count << "\n\n";

			// Parameters of logical Neurons:
			fs << "Parameters of used Denmems on this Anncore: \n" ;

			for(size_t i = 0; i<_denmems.size(); ++i) {
				if (_denmems[i]) {
					fs << "denmem " << i << ":\n";
					// Biological Parameters
					ESS::BioParameter params = _denmems[i]->getDenmemParams().toBioParameter();
					fs << "\t g_l = " << params.g_l;
					fs << "\t v_rest = " << params.v_rest;
					fs << "\t a = " << params.a;
					fs << "\t tau_w = " << params.tau_w;
					fs << "\t b = " << params.b;
					fs << "\t v_thresh = " << params.v_thresh;
					fs << "\t delta_T = " << params.delta_T;
					fs << "\t e_rev_I = " << params.e_rev_I;
					fs << "\t e_rev_E = " << params.e_rev_E;
					fs << "\t tau_syn_I = " << params.tau_syn_I;
					fs << "\t tau_syn_E = " << params.tau_syn_E;
					fs << "\t tau_refrac = " << params.tau_refrac;
					fs << "\t v_spike = " << params.v_spike;
					fs << "\t v_reset = " << params.v_reset;
					fs << "\t cm = " << params.cm;
					fs << "\n";
				}
			}
			fs << "\n\n";


			// TODO: print out which denmems are combined ...
			// Hardware Neurons associated with logical Neuron ID:
			//fs << "Synapse Columns connected to logical Neuron: col(log_neuron_id) \n";
			//for(unsigned i = 0; i < _cols2nrns.size(); i++)
			//	if( _cols2nrns.at(i)!= NULL )
			//		fs << i << "(" << _cols2nrns.at(i)->get_logical_neuron_id() << ") ";
			//fs << "\n\n";

			// Biological Neurons associated with logical Neurons:
			fs << "Firing denmems: denmem:addr:wta \n";

			std::map<unsigned int, CompoundNeuronModule*>::const_iterator it_d2n =  _firing_denmem_2_compound_neuron.begin();
			std::map<unsigned int, CompoundNeuronModule*>::const_iterator it_d2n_end =  _firing_denmem_2_compound_neuron.end();
			for(;it_d2n != it_d2n_end; ++it_d2n) {
				unsigned int current_denmem = it_d2n->first;
				CompoundNeuronModule* ccn = it_d2n->second;
				fs << current_denmem << ":a" << ccn->get_6_bit_address() << ":wta" << ccn->get_wta_id() << " ";
			}
			fs << "\n\n";

			// Active Synapses row:column:addr:weight
			//
			fs << "Active Synapses: column(addr:weight) \n";
			for(unsigned int i = 0; i < 2*SYNDRIVERS*ROWS_PER_SYNDRIVER; i++)
			{
				fs << "Row " << i << ":\t";
				for(unsigned int j=0; j< SYN_PER_ROW; j++)
				{
					if (_addresses[i*SYN_PER_ROW+j] != HMF::HICANN::four_bit_address_disabling_synapse)
						fs << j << "(a" << (int) _addresses[i*SYN_PER_ROW+j] << ":w" << (int) _weights[i*SYN_PER_ROW+j] << ") ";
				}
				fs << "\n";
			}
			fs << "\n";

			// Syndriver Configuration
			//
			fs << "Syndriver Configuration (left: 0..111, right: 112..224) \n";
			fs << "only printing enabled syndriver \n";
			fs << "formatting: | up: syntype(0=exc), synapse_trafo | down: syntype, synapse_trafo | STP: enable mode(0=depr) U_SE tau_rec lambda N | pre_out[0:3]\n";
			for(unsigned int i = 0; i < 2*SYNDRIVERS; i++)
			{
                if ( _syndrivers[i].get_enable() )
                {
				    fs << "ID " << i << ":\t";
				    fs << "| up: " << (int) _syndrivers[i].get_syn_type(1) << ", ";
				    const std::vector<double>& synapse_trafo_1 = _syndrivers[i].get_synapse_trafo(1);
				    for(size_t np=0; np<synapse_trafo_1.size(); ++np)
				    	fs << synapse_trafo_1[np] << " ";
				    fs << "\t";
				    fs << "| down: " << (int) _syndrivers[i].get_syn_type(0) << ", ";
				    const std::vector<double>& synapse_trafo_0 = _syndrivers[i].get_synapse_trafo(0);
				    for(size_t np=0; np<synapse_trafo_0.size(); ++np)
				    	fs << synapse_trafo_0[np] << " ";
				    fs << "\t";
				    fs << "| STP: " << (int) _syndrivers[i].get_stp_enable() << " " << (int) _syndrivers[i].get_mode() << " " << _syndrivers[i].get_U_SE() << " "
						<< _syndrivers[i].get_tau_rec() << " " << _syndrivers[i].get_lambda() << " " << _syndrivers[i].get_N() <<  "\t";
				    fs << "| SLs: " << (int) _syndrivers[i].get_pre_out(0) << " " << (int) _syndrivers[i].get_pre_out(1) <<" " << (int) _syndrivers[i].get_pre_out(2) << " " << (int) _syndrivers[i].get_pre_out(3);
				    fs << "\n";
			    }
            }
			fs << "\n";

		}

	fs.close();
	return 1;

	}
	else
	{
        LOG4CXX_WARN(logger, "anncore_behav " << _anncore_id << " print_config(): " <<  " could not open file: " << fn.c_str() );
		fs.close();
		return 0;
	}
}

void anncore_behav::set_weight_distortion(double mean, double stddev)
{
    LOG4CXX_DEBUG(logger, "anncore_behav " << _anncore_id << " set_weight_distortion(" << mean << "," << stddev << ")" );
	static boost::mt11213b rng(139975001);
	boost::normal_distribution<> nd(mean,stddev);
	boost::variate_generator< boost::mt11213b&,
							  boost::normal_distribution<> > var_nor(rng,nd);
	std::vector<float>::iterator it_wd = _weight_distortions.begin();
	std::vector<float>::iterator it_wd_end = _weight_distortions.end();
	for ( ; it_wd != it_wd_end; ++it_wd)
	{
		double d = var_nor();
		*it_wd = (d > 0) ? d : 0.;
	}
}

void anncore_behav::disable_weight_distortion()
{
    LOG4CXX_DEBUG(logger, "anncore_behav " << _anncore_id << " weight distortion disabled");
	std::vector<float>::iterator it_wd = _weight_distortions.begin();
	std::vector<float>::iterator it_wd_end = _weight_distortions.end();
	for ( ; it_wd != it_wd_end; ++it_wd)
	{
		*it_wd = 1.;
	}
}

void anncore_behav::getConnectedDenmems(    std::set<unsigned int>& connected_denmems,
                                            std::map<unsigned int, unsigned int> const & denmems2nrns,
                                            unsigned int const hw_neuron_id,
                                            unsigned int const denmem_id) const
{
    // check that hw_neuron_id corresponds to the hw_neuron id associated to denmem_id in denmems2nrns
    auto iterator = denmems2nrns.find(denmem_id);
    unsigned int hw_neuron_expected = iterator->second;
    if (hw_neuron_id != hw_neuron_expected)
    {
        LOG4CXX_WARN(logger, "anncore_behav " << _anncore_id << " for denmem " << denmem_id 
                << " the logical neuron id " << hw_neuron_id << " does not match to the logica neuron id in denmems2nrns: " << hw_neuron_expected );
    }
    for (auto it_den2nrn : denmems2nrns )
    {
        if (it_den2nrn.second == hw_neuron_expected)
        {
            connected_denmems.insert(it_den2nrn.first);
        }
    }
}
