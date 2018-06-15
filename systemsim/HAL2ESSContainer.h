#pragma once
#include <vector>
#include <bitset>
#include <map>
#include <array>

#include "HAL2ESSEnum.h"
#include "hal/FPGAContainer.h"

#include "HMF/HWNeuronParameter.h"
#include "euter/cellparameters.h"

namespace HMF {
namespace Handle {
	struct HICANN;
}
}

namespace ESS {

//typedefs for Neuron Hardware Parameter and Bio Parameter - only use those in the ESS!
typedef HMF::HWNeuronParameter                 HWParameter;

// add g_l as param for the ESS
class BioParameter : public PyNNParameters::EIF_cond_exp_isfa_ista
{
public:
    double g_l; /// leak conductance in nano Siemens
    void initFromPyNNParameters(PyNNParameters::EIF_cond_exp_isfa_ista const & param);
};


struct RepeaterBlock
{
//functions
	RepeaterBlock();
	RepeaterBlock(RepeaterLocation a);
	RepeaterBlock(size_t i);
	RepeaterBlock& operator= (RepeaterBlock const& rhs);
//data
	std::vector<unsigned char> repeater;
};

struct Synapse_t
{
//functions
	Synapse_t();
	Synapse_t(char _address, char _weight);
	char get_address();
	void set_address(const char &addr);
	char get_weight();
	void set_weight(const char &w);
//data
	char address;   // 4bit-address
	char weight;    // 4bit-weight
};

struct syndr_row
{
	bool senx; //!< connect row to excitatory input of neuron
	bool seni; //!< connect row to inhibitory input of neuron
	uint8_t preout_even; //!< defines, to which upper 2 bit of a spl1 the EVEN neuron of a row listen to. Values: 0-3
	uint8_t preout_odd; //!< defines, to which upper 2 bit of a spl1 the ODD neuron of a row listen to. Values: 0-3
	uint8_t sel_Vgmax; //!< choose V_gmax from blockwise shared FGs. Values: 0-3
	uint8_t gmax_div_x; //!< choice of gmax divisor when using excitatory input of neurons. Values: 1-15
	uint8_t gmax_div_i; //!< choice of gmax divisor when using inhibitory input of neurons. Values: 1-15

	syndr_row():
		senx(0), seni(0), preout_even(0), preout_odd(0), sel_Vgmax(0), gmax_div_x(1), gmax_div_i(1) {}
};

struct syndriver_cfg
{
    bool enable; //!< enable this synapse driver
	bool mirror;  //!< select input from top neighbour switch
	bool locin;  //!< select input from L1
	bool stp_enable; //!< enable short-term plasticity (STP) for this syndriver
	bool stp_mode; //!< set STP mode for this syndriver. 0: facilitation, 1: depression
	unsigned char stp_cap; //!< select size of capacitor in STP circuit.
	syndr_row bottom_row_cfg; //!< config for bottom row
	syndr_row top_row_cfg; //!< config for top row

	syndriver_cfg(): enable(0),mirror(0),locin(0),stp_enable(0),stp_mode(0),stp_cap(0)
		,bottom_row_cfg(syndr_row()),top_row_cfg(syndr_row()) {}
};

/// stores the interconnects between the 4 neurons within a neuron quad
/// hori[0]: enable horizontal connection between the two upper neurons
/// hori[1]: enable horizontal connection between the two lower neurons
/// vert[0]: enable vertical connection between the two left neurons
/// vert[1]: enable vertical connection between the two right neurons
struct nquad_connections
{
    std::bitset<2> hori;
    std::bitset<2> vert;
};

struct neuron	//to be precise this struct stores the data of a single denmem
{
//functions
	neuron();
	neuron(size_t nrn);
	neuron(neuron const& copy); //Copy-Constructor
	neuron& operator= (neuron const& rhs);
//data
	bool activate_firing;	//stores if firing mechanism is activated
	bool enable_fire_input;	//stores if denmem is shorted with a the adjacent denmem from neighboring quad
	bool enable_curr_input;	//stores if this denmem receives current stimuli from a FGCell
	//not necessary for ESS, kept to keep the HALBE-data complete
	bool enable_output;		//stores if this denmem is read out by the analog output
	bool enable_spl1_output;//stores if spl1 output is activated
    //needed for ess
	unsigned int l1_address;
	size_t neuron_address;
	bool is_connected;                      //stores if denmem is connected to other denmems to form a bigger logical neuron
    HWParameter neuron_parameters;          //The Hardware Neuron Parameter
    unsigned int V_reset;                   //The shared hardware parameter V_reset
    bool cap;                               //The Hardware Capacitance switch true -> big cap, false -> small cap
};

//holds the shared parameter of syndriver and synapses
struct SyndriverParameterESS
{
	SyndriverParameterESS();

	//transformation coefficients
	std::vector<double> g_trafo_up;
	std::vector<double> g_trafo_down;

	//STP parameter
	double tau_rec; //!< recovery time constant in s
	double lambda; //!< stp model parameter lambda
	double N_dep; //!< model parameter N in depression mode
	double N_fac; //!< model parameter N in facilitation mode
};

struct SyndriverParameterHW
{
    unsigned int V_stdf;
    unsigned int V_dep;
    unsigned int V_fac;
    unsigned int V_dtc;
    SyndriverParameterHW();
};

//holds the syndriver of a side
struct SyndriverBlock
{
//functions
	SyndriverBlock();
//data
	std::vector<syndriver_cfg> syndriver;
	// parameter for the syndriver
    std::array<SyndriverParameterHW,2> synapse_params_hw; // 0-> upper syndriver Block 1-> lower syndriver Block
};

struct StimulusContainer
{
//functions
    StimulusContainer();
//data
    std::array<uint16_t, 129> Currents;
    uint8_t PulseLength;
    bool Continuous;
};

typedef std::vector<std::vector<bool> > crossbar_type;

typedef std::vector<std::vector<bool> > synswitch_type;

struct hicann
{
//functions
	hicann(const unsigned int id);
	void reset(); //resets the hicann, not implemented yet
	hicann& operator=(hicann const& rhs);
	std::bitset<4> get_syn_weight(const size_t &row, const size_t &col);
	void set_syn_weight(const std::bitset<4> &weight, const size_t &row, const size_t &col);
	std::bitset<4> get_syn_address(const size_t &row, const size_t &col);
	void set_syn_address(const std::bitset<4> &addr, const size_t &row, const size_t &col);
//data
	bool available;										//for initialize_pcb, is set to true only if HAL2ESS::init is called for this hicanns id
	unsigned int hicann_id;
	unsigned int mX;
	unsigned int mY;
    uint8_t PLL_freq;
	std::vector<neuron> neurons_on_hicann;				//stores the neurons on this hicann
	std::vector<std::vector<bool> > merger_tree_config;	//stores the merger-tree-configuration
	std::vector<RepeaterBlock> repeater_config;			//stores the repeater-configuration
	std::array<crossbar_type, 2> crossbar_config;		//stores the Crossbar-switch-Matrices, 0->Left, 1 -> Right
	std::array<synswitch_type, 4> synswitch_config;		//Configuration of Syndriver-switch-matrices, 00=0->left/down, 01=1->left/up, 10 = 2 ->right/down, 11 = 3 ->right/up
	std::vector<std::vector<Synapse_t> > syn_array;		//stores the synapses, [i=column][j=row], both synarrays are stored in this vector, counting begins with zero at the top
	std::array<SyndriverBlock, 2> syndriver_config;		//stores the syndriver blocks, 0->left 1->right
	std::vector<bool> dnc_link_enable;					//stores config of the dnc<->hicann-if [0] -> enable [1] -> directions
	std::vector<ESS::DNCIfDirections> dnc_link_directions;
	std::array<StimulusContainer, 4> stimulus_config;
    std::array<nquad_connections, 128> connection_config;
    std::map<size_t,size_t> denmem2nrn; 
};

struct adc_config
{
    bool enable;
    uint32_t num_samples;
    std::bitset<4> input;   // activates the 4 different neurons per nquad: 0 -> bot_odd, 1-> top_odd, 2 -> bot_even, 3 -> top_even 
    adc_config() : enable(false), num_samples(0), input(0)
    {}
};

struct wafer
{
//functions
	wafer();
//data
	std::vector<hicann> hicanns;			// stores the hicanns
	//for initalize_pcb, TODO make settable -> mapping ?!
    //these values are the maximum numbers for a complete wafer. not all combinations of x and y values exist becuase the actual wafer is not rectengularbut hicanns are only initialized if the available bit is set, which is done by HAL2ESS::init, so only valid combinations can be initialized
    static const unsigned int num_x_hicanns = 36;		// number of hicanns in x-direction
    static const unsigned int num_y_hicanns = 16;		// number of hicanns in y-direction
    static const unsigned int num_dncs      = 48;		// number of dncs on wafer == number of reticles on wafer
    std::array<adc_config,2*num_dncs> adcs; //configuration of the adc boards, there are 2 adc boards per reticle on the ESS
};

//struct for global parameters and settings
struct global_parameter
{
//functions
    global_parameter();
//data
    double  speedup;
    double  timestep;
    bool    enable_weight_distortion;
    double  weight_distortion; 
    bool    enable_timed_merger;
    bool    enable_spike_debugging;
};

/// Data structure for one entry in the FPGA playback memory
struct playback_entry
{
	typedef uint32_t delta_time_t;
	typedef HMF::FPGA::PulseEvent event_t;

	delta_time_t delta_time; //!< delta time to previous time in fpga clk cycles (=8ns). Pulse is released this time after the previous event.
	event_t      event; //!< pulse event. only the lower 15 bit of the time are used. TODO: we might introduce a "CroppedPulseEvent" or so

	playback_entry(){}

	playback_entry(delta_time_t delta, event_t ev)
		:delta_time(delta)
		,event(ev)
	{}
};

/// Data structure for one entry in the FPGA trace memory
struct trace_entry
{
	typedef uint32_t fpga_time_t; // 32-bit: -> max representable time is (2^32-1)*8e-9 = 34 s, which should be sufficient for ESS sims.
	typedef HMF::FPGA::PulseEvent event_t;

	fpga_time_t fpga_time; //!< time when pulse was recorded to trace memory in fpga clk cycles (=8ns).
	event_t     event; //!< pulse event. only the lower 15 bit of the time are used. TODO: we might introduce a "CroppedPulseEvent" or so

	trace_entry(){}

	trace_entry(fpga_time_t fpga_time, event_t ev)
		:fpga_time(fpga_time)
		,event(ev)
	{}
};

typedef std::vector<playback_entry> playback_container_t;
typedef std::vector<trace_entry> trace_container_t;

struct fpga_config
{
	fpga_config():record(false){}
	bool record;
	playback_container_t playback_pulses;
};

struct dnc_config
{
	std::bitset<64> hicann_directions;
};


}//end namespace ess
