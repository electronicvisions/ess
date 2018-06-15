#include <assert.h>

#include "HAL2ESSContainer.h"
#include "bitter/integral.h"
#include "hal/Coordinate/HMFGrid.h"

namespace ESS{

void BioParameter::initFromPyNNParameters(PyNNParameters::EIF_cond_exp_isfa_ista const & param)
{
    a           = param.a;
	b           = param.b;
	cm          = param.cm;
	delta_T     = param.delta_T;
	e_rev_E     = param.e_rev_E;
	e_rev_I     = param.e_rev_I;
	tau_m       = param.tau_m;
	tau_refrac  = param.tau_refrac;
	tau_syn_E   = param.tau_syn_E;
	tau_syn_I   = param.tau_syn_I;
	tau_w       = param.tau_w;
	v_reset     = param.v_reset;
	v_rest      = param.v_rest;
	v_spike     = param.v_spike;
	v_thresh    = param.v_thresh;
    // calculate the conductance from cm and tau_m 
    // factor 100 = ratio hw_cap to bio_cap
    g_l         = 1000.*cm / tau_m; // 1000*nF/ms -> nano Siemens
}

//functions for RepeaterBlock

RepeaterBlock::RepeaterBlock() : repeater(64,0)
{}

RepeaterBlock::RepeaterBlock(RepeaterLocation a)
{
	size_t num_rep = 0;
	switch(a)
	{
		case REP_L: num_rep = 32;  break;
		case REP_R: num_rep = 32;  break;
		case REP_UL: num_rep = 64; break;
		case REP_DL: num_rep = 64; break;
		case REP_UR: num_rep = 64; break;
		case REP_DR: num_rep = 64; break;
	}
	std::vector<unsigned char> init(num_rep,0);
	this->repeater = init;
}

RepeaterBlock::RepeaterBlock(size_t i)
{
	assert(i < 6);
	size_t num_rep = 0;
	if(i < 2)
		num_rep = 32;
	else
		num_rep = 64;

	std::vector<unsigned char> init(num_rep,0);
	this->repeater = init;
}

RepeaterBlock& RepeaterBlock::operator= (RepeaterBlock const& rhs)
{
	this->repeater = rhs.repeater;
	return *this;
}

//functions for Synapse_t

Synapse_t::Synapse_t() : address(0), weight(0)
{}

Synapse_t::Synapse_t(char _address, char _weight) : address(_address), weight(_weight)
{}

char Synapse_t::get_address()
{
	return address;
}

void Synapse_t::set_address(const char &addr)
{
	address = addr;
}

char Synapse_t::get_weight()
{
	return weight;
}

void Synapse_t::set_weight(const char &w)
{
	weight = w;
}

//functions for neuron

neuron::neuron() :	activate_firing{false},
					enable_fire_input{false},
					enable_curr_input{false},
					enable_output{false},
					enable_spl1_output{false},
                    l1_address{0},
					neuron_address{0},
					is_connected{false},
                    neuron_parameters{},
                    V_reset{0},
                    cap{true}
{}

neuron::neuron(size_t nrn) :	activate_firing{false},
								enable_fire_input{false},
								enable_curr_input{false},
								enable_output{false},
					            enable_spl1_output{false},
								l1_address{0},
								neuron_address{nrn},
					            is_connected{false},
								neuron_parameters{},
                                V_reset{0},
                                cap{true}
{}


neuron::neuron(neuron const& copy) :	activate_firing(copy.activate_firing),
										enable_fire_input(copy.enable_fire_input),
										enable_curr_input(copy.enable_curr_input),
										enable_output(copy.enable_output),
										enable_spl1_output(copy.enable_spl1_output),
                                        l1_address(copy.l1_address),
										neuron_address(copy.neuron_address),
					                    is_connected(copy.is_connected),
										neuron_parameters(copy.neuron_parameters),
                                        V_reset(copy.V_reset),
                                        cap(copy.cap)
{}

neuron& neuron::operator=(neuron const& rhs)
{
	// TODO write operator != and check this != rhs
    this->activate_firing = rhs.activate_firing;
    this->enable_fire_input = rhs.enable_fire_input;
    this->enable_curr_input = rhs.enable_curr_input;
	this->enable_output = rhs.enable_output;
    this->enable_spl1_output = rhs.enable_spl1_output;
    this->l1_address = rhs.l1_address;
    this->neuron_address = rhs.neuron_address;
	this->is_connected = rhs.is_connected,
	this->neuron_parameters = rhs.neuron_parameters;
	this->V_reset = rhs.V_reset;
    this->cap = rhs.cap;
    return *this;
}
    

//initializer for Syndriver Params

// default STP parameters as in calibtic/HMF/ModelSharedParameter.h
SyndriverParameterESS::SyndriverParameterESS(): tau_rec(10.e-6), lambda(1.0),
	N_dep(0.), N_fac(1.)
{}

//TODO initialize with reasonable values
SyndriverParameterHW::SyndriverParameterHW() : V_stdf(511), V_dep(0), V_fac(511), V_dtc(511)
{}

//functions for SyndriverBlock

SyndriverBlock::SyndriverBlock() : syndriver(112, syndriver_cfg()), synapse_params_hw()
{}

//functions for stimulus container    

StimulusContainer::StimulusContainer() : PulseLength(15), Continuous(false)   
{Currents.fill(0);} 

//functions for hicann

hicann::hicann(const unsigned int id)
{
	this->available = false;
	this->hicann_id = id;
	this->mX = HMF::Coordinate::HICANNOnWaferGrid.at(id).first;
	this->mY = HMF::Coordinate::HICANNOnWaferGrid.at(id).second;
    this->PLL_freq = 100;               //default value in MHz

	this->neurons_on_hicann.resize(size::neurons_on_hicann);

	//initialize the neurons
	for(size_t addr = 0; addr < size::neurons_on_hicann; addr++)
	{
		neuron nrn{addr};
		this->neurons_on_hicann[addr] = nrn;
	}

	//initialize the merger config
	std::vector<std::vector<bool> > mergerconfig(20,std::vector<bool>(16,false));
	this->merger_tree_config = mergerconfig;

	this->repeater_config.resize(6);
	for(size_t i = 0; i < 6; ++i)
	{
		this->repeater_config[i] = RepeaterBlock(i);
	}

	//initialize the crossbar_config
	crossbar_type crossbar(size::hlines_on_hicann, std::vector<bool>(size::switches_per_row,false));
	this->crossbar_config[0] = crossbar;
	this->crossbar_config[1] = crossbar;

	//initialize the synswitch_config
	synswitch_type synswitch(size::synswitch_rows, std::vector<bool>(size::synswitches_per_row,false));
	for(size_t i = 0; i < this->synswitch_config.size(); ++i)
	{
		this->synswitch_config[i] = synswitch;
	}

	//initialize the synapse_arrays
	Synapse_t synapse;
	this->syn_array.resize(size::synapses_per_row,std::vector<Synapse_t>(2*size::synapses_per_col,synapse));

	//initialize the syndriver_blocks
	SyndriverBlock syndr{};
	for(size_t i = 0; i < this->syndriver_config.size(); ++i)
	{
		this->syndriver_config[i] = syndr;
	}

	//initialize the dnc-if-config
	dnc_link_enable.resize(size::dnc_channels,false);
	dnc_link_directions.resize(size::dnc_channels,DNCIfDirections::OFF);
    
    //initialize the stimulus_config
    for(size_t i = 0; i < stimulus_config.size(); ++i)
        stimulus_config[i] = StimulusContainer{};
    
    std::map<size_t,size_t> denmem2nrn; 
}

hicann& hicann::operator=(hicann const& rhs)
{
	this->available = rhs.available;
	this->hicann_id = rhs.hicann_id;
	this->neurons_on_hicann = rhs.neurons_on_hicann;
	this->mX = rhs.mX;
	this->mY = rhs.mY;
	this->PLL_freq = rhs.PLL_freq;
    this->repeater_config = rhs.repeater_config;
	this->crossbar_config = rhs.crossbar_config;
	this->syn_array = rhs.syn_array;
	this->syndriver_config = rhs.syndriver_config;
	this->dnc_link_enable = rhs.dnc_link_enable;
	this->dnc_link_directions = rhs.dnc_link_directions;
    this->stimulus_config = rhs.stimulus_config;
    return *this;
}

std::bitset<4> hicann::get_syn_weight(const size_t &row, const size_t &col)
{
	std::bitset<4> returnval;

	char weight = syn_array.at(col).at(row).get_weight();

	for(size_t i = 0; i < returnval.size(); ++i)
	{
		returnval[i] = bit::test(weight,i);
	}

	return returnval;
}

void hicann::set_syn_weight(const std::bitset<4> &weight, const size_t &row, const size_t &col)
{
	char w = 0;
	char aux = 0;

	for(size_t i = 0; i < weight.size(); ++i)
	{
		if(weight[i] == 1)
			w |= bit::set(aux,i);
		aux = 0;
	}
	syn_array.at(col).at(row).set_weight(w);
}

std::bitset<4> hicann::get_syn_address(const size_t &row,const size_t &col)
{
	std::bitset<4> returnval;

	char addr = syn_array.at(col).at(row).get_address();

	for(size_t i = 0; i < returnval.size(); ++i)
	{
		returnval[i] = bit::test(addr,i);
	}

	return returnval;
}

void hicann::set_syn_address(const std::bitset<4> &addr,const size_t &row, const size_t &col)
{
		char addresse = 0;
		char aux = 0;

		for(size_t i = 0; i < addr.size(); ++i)
		{
			if(addr[i] == true)
				addresse |= bit::set(aux,i);
			aux = 0;
		}

		syn_array.at(col).at(row).set_address(addresse);
}

//functions for wafer

const unsigned int wafer::num_x_hicanns;		// number of hicanns in x-direction
const unsigned int wafer::num_y_hicanns;		// number of hicanns in y-direction
const unsigned int wafer::num_dncs;     		// number of dncs on wafer == number of reticles on wafer

wafer::wafer()
{
    //initialize the hicanns
	hicann hica{1};
	std::vector<hicann> init(size::hicanns_on_wafer, hica);
	for(unsigned int id = 0; id < size::hicanns_on_wafer; id++)
	{
		hicann hic{id};
		init[id] = hic;
	}
	this->hicanns = init;
}

//functions for global_parameter

//constructor with default values
global_parameter::global_parameter() :
    speedup(10000.),
    timestep(0.01), //timestep in ns
    enable_weight_distortion(false),
    weight_distortion(0.),
    enable_timed_merger(true),
    enable_spike_debugging(false)
{}

} //end namespace ESS
