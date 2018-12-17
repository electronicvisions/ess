#include <sstream>
#include <algorithm>
#include <fstream>
#include <stdexcept>

#include <boost/filesystem.hpp>
#include <boost/pointer_cast.hpp>

#include "HALaccess.h"
#include "hal/Coordinate/HMFGrid.h"
#include "hal/Coordinate/HMFGeometry.h"
#include "hal/Coordinate/iter_all.h"
#include "hal/HICANN/FGBlock.h"
#include "calibtic/HMF/NeuronCalibration.h"
#include "calibtic/HMF/HICANNCollection.h"
#include "calibtic/HMF/SynapseRowCalibration.h"
#include "calibtic/HMF/SharedCalibration.h"
#include "calibtic/backend/Backend.h"
#include "calibtic/backend/Library.h"
#include "calibtic/trafo/Polynomial.h"
#include "euter/cellparameters.h"

#include <log4cxx/logger.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("ESS.HALAccess");

//auxilliary functions
namespace {

//TODO report error for illegal combination
unsigned int to_id(unsigned int x, unsigned int y)
{
	unsigned int returnval = HMF::Coordinate::HICANNOnWaferEnum.at(x).at(y);
	return returnval;
}

HMF::Coordinate::SynapseDriverOnHICANN toSynapseDriverOnHICANN(
    enum ESS::HICANNBlock block,
    enum ESS::HICANNSide side,
    unsigned int driver_in_quadrant //!< low-level hw counting: from center of chip to top/bottom
    )
{
	using namespace HMF::Coordinate;
	SynapseDriverOnQuadrant const driver_on_quadrant(Enum(
	    block == ESS::HICANNBlock::BL_UP ? driver_in_quadrant
	                                     : SynapseDriverOnQuadrant::max - driver_in_quadrant));
	QuadrantOnHICANN const quadrant{SideVertical(block), SideHorizontal(side)};
	return driver_on_quadrant.toSynapseDriverOnHICANN(quadrant);
}

// returns true if a denmem is active (needs to be simulated).
// This is detected by checking if both activate_firing==true and
// enable_spl1_output==true.
bool is_active(ESS::neuron const& denmem)
{
	return denmem.activate_firing && denmem.enable_spl1_output;
}

} // anonymous namespace

//functions of HALaccess

HALaccess::HALaccess(unsigned int wafer_id, std::string filepath)
    : waf()
    , mGlobalParams()
    , mWaferId(wafer_id)
    , mFilepath(filepath)
    , mDefaultCalib(new calib_type())
{
	// set ESS Defaults
	mDefaultCalib->atNeuronCollection()->setDefaults();
	mDefaultCalib->atBlockCollection()->setDefaults();
	mDefaultCalib->atSynapseRowCollection()->setEssDefaults();
}

HALaccess::~HALaccess()
{}

//returns the x-coordinate of the hicann
unsigned int HALaccess::getHicannXCoordinate(unsigned int hicann_id) const
{
	return wafer().hicanns[hicann_id].mX;
}

//returns the y-coordinate of the hicann
unsigned int HALaccess::getHicannYCoordinate(unsigned int hicann_id) const
{
	return wafer().hicanns[hicann_id].mY;
}

uint8_t HALaccess::getPLLFrequency(unsigned int hicann_id) const
{
    return wafer().hicanns[hicann_id].PLL_freq;
}

//this function returns the number of neurons mapped on this hicann 
unsigned int HALaccess::getNeuronsOnHicann(unsigned int x_coord, unsigned int y_coord )
{
	size_t hic_id = to_id(x_coord, y_coord);
    LOG4CXX_DEBUG(logger, "Getting neurons of hicann " << hic_id);
    //get denmem2neuron configuration
    Denmem2Neuron(hic_id);
    auto const & den2nrn = wafer().hicanns[hic_id].denmem2nrn;
    std::vector<size_t> nrns_on_hic;
    //find how many different logical neurons were mapped on this hicann
    for(auto it : den2nrn)
	{
        size_t log_nrn = it.second;
        auto finder = std::find(nrns_on_hic.begin(),nrns_on_hic.end(),log_nrn);
        if(finder == nrns_on_hic.end() )
        {
            nrns_on_hic.push_back(log_nrn);
            LOG4CXX_DEBUG(logger, "Denmem " << it.first << " builds up logical Neuron " << log_nrn);
        }
    }
	return nrns_on_hic.size();
    //return den2nrn.size();
}

//returns the global parameters
ESS::global_parameter const& HALaccess::getGlobalHWParameters() const
{
	return mGlobalParams;
}
    
ESS::global_parameter & HALaccess::getGlobalHWParameters()
{
	return mGlobalParams;
}

//gets the synapse array from specified hicann
std::vector<std::vector<ESS::Synapse_t> > HALaccess::getSynapses(unsigned int x_coord,unsigned int y_coord) const
{
	size_t hic_id = to_id(x_coord, y_coord);
	return wafer().hicanns[hic_id].syn_array;
}


//returns the syndriver configuration by passing the correspondig data structure of HAL2ESSContainer::hicann
std::vector<ESS::syndriver_cfg> HALaccess::getSyndriverConfig(
		unsigned int x_coord,
		unsigned int y_coord,
		enum ESS::HICANNSide side) const
{
	size_t hic_id = to_id(x_coord, y_coord);

	//determine the block-address
	size_t addr;
	if(side == ESS::HICANNSide::S_LEFT)
	{
        addr = 0;
    }
    else
    {
        addr = 1;
    }

	std::vector<ESS::syndriver_cfg> returnval = wafer().hicanns[hic_id].syndriver_config[addr].syndriver;
	return returnval;
}

//returns the global parameters belonging to the Syndriver and Synapses of the Corresponding block
ESS::SyndriverParameterESS HALaccess::getSyndriverParameters(
    unsigned int x_coord,
    unsigned int y_coord,
    enum ESS::HICANNBlock block,
    enum ESS::HICANNSide side,
    unsigned int driver_in_quadrant) const
{
	using namespace HMF;

	size_t hic_id = to_id(x_coord, y_coord);

	size_t addr;
	if (side == ESS::HICANNSide::S_LEFT) {
		addr = 0;
	} else {
		addr = 1;
	}
	ESS::SyndriverParameterHW params_HW;
	size_t driver_in_side;
	if (block == ESS::HICANNBlock::BL_UP) {
		params_HW = wafer().hicanns[hic_id].syndriver_config[addr].synapse_params_hw[0];
		driver_in_side = driver_in_quadrant;
	} else {
		params_HW = wafer().hicanns[hic_id].syndriver_config[addr].synapse_params_hw[1];
		driver_in_side = 56 + driver_in_quadrant;
	}

	ESS::SyndriverParameterESS returnval;

	ESS::syndriver_cfg raw_config =
	    wafer().hicanns[hic_id].syndriver_config[addr].syndriver.at(driver_in_side);
	if (raw_config.enable) {
		// use synapse calibration
		Coordinate::SynapseDriverOnHICANN driver =
		    toSynapseDriverOnHICANN(block, side, driver_in_quadrant);

		// top row (ESS counting)
		Coordinate::RowOnSynapseDriver top_row_on_driver(
		    block == ESS::HICANNBlock::BL_UP ? Coordinate::top : Coordinate::bottom);
		Coordinate::SynapseRowOnHICANN top_row(driver, top_row_on_driver);
		returnval.g_trafo_up =
		    get_row_wise_synapse_trafo(x_coord, y_coord, top_row, raw_config.top_row_cfg);

		// bottom row (ESS counting)
		Coordinate::RowOnSynapseDriver bottom_row_on_driver(
		    block == ESS::HICANNBlock::BL_UP ? Coordinate::bottom : Coordinate::top);
		Coordinate::SynapseRowOnHICANN bottom_row(driver, bottom_row_on_driver);
		returnval.g_trafo_down =
		    get_row_wise_synapse_trafo(x_coord, y_coord, bottom_row, raw_config.bottom_row_cfg);
	}

	/////////////////////
	// Shared STP Params
	/////////////////////
	// TODO: this trafo should actually be done only once per quadrant
	// get reverse trafo:
	Coordinate::FGBlockOnHICANN const fg_block{Coordinate::X(side), Coordinate::Y(block)};

	HMF::SharedCalibration const& shared_calibration =
	    *boost::dynamic_pointer_cast<SharedCalibration const>(
	        getCalib(x_coord, y_coord)->atBlockCollection()->at(fg_block.id()));
	// set dac values for reverse transformation
	HMF::HWSharedParameter hw_shrd_p;
	hw_shrd_p.setParam(HICANN::shared_parameter::V_dtc, params_HW.V_dtc);
	hw_shrd_p.setParam(HICANN::shared_parameter::V_dep, params_HW.V_dep);
	hw_shrd_p.setParam(HICANN::shared_parameter::V_fac, params_HW.V_fac);
	hw_shrd_p.setParam(HICANN::shared_parameter::V_stdf, params_HW.V_stdf);
	HMF::ModelSharedParameter m_shrd_p = shared_calibration.applySharedReverse(hw_shrd_p);

	returnval.tau_rec = m_shrd_p.tau_rec * 1.e-6; // trafo from us to s
	returnval.lambda = m_shrd_p.lambda;
	returnval.N_dep = m_shrd_p.N_dep;
	returnval.N_fac = m_shrd_p.N_fac;

	return returnval;
}

std::vector<double> HALaccess::get_row_wise_synapse_trafo(
    unsigned int x_coord,
    unsigned int y_coord,
    HMF::Coordinate::SynapseRowOnHICANN row_coord,
    ESS::syndr_row row_cfg) const
{
	using namespace HMF;

	std::vector<double> g_trafo;

	bool row_used = true;
	HMF::GmaxConfig gmax_cfg;
	gmax_cfg.set_sel_Vgmax(row_cfg.sel_Vgmax);
	if (row_cfg.senx && !row_cfg.seni)
		gmax_cfg.set_gmax_div(row_cfg.gmax_div_x);
	else if (!row_cfg.senx && row_cfg.seni)
		gmax_cfg.set_gmax_div(row_cfg.gmax_div_i);
	else if (row_cfg.senx && row_cfg.seni)
		throw std::runtime_error("invalid config config of synapse driver. either left or right "
		                         "synaptic input of neuron must be chosen. not both");
	else // both disabled -> use default
		row_used = false;

	if (row_used) {
		SynapseRowCalibration const& src =
		    *boost::dynamic_pointer_cast<SynapseRowCalibration const>(
		        getCalib(x_coord, y_coord)->atSynapseRowCollection()->at(row_coord));

		LOG4CXX_DEBUG(logger, "searching for synapse calibration with gmax_config: " << gmax_cfg);
		assert(src.exists(gmax_cfg));
		boost::shared_ptr<SynapseCalibration const> synapse_calib = src.at(gmax_cfg);
		assert(synapse_calib->size() > 0);
		// get Polynomial
		boost::shared_ptr<calibtic::trafo::Polynomial const> poly =
		    boost::static_pointer_cast<calibtic::trafo::Polynomial const>(synapse_calib->at(0));
		g_trafo = poly->getData();
		// convert from nanoSiemens to Siemens
		for (auto& coeff : g_trafo)
			coeff *= 1.e-9;
	}

	return g_trafo;
}

//returns a map that matches all denmem to the address of the hw_neuron to which it belongs
std::map<unsigned, unsigned> HALaccess::getDendrites(
		unsigned int x_coord,
		unsigned int y_coord) const
{
	size_t hic_id = to_id(x_coord, y_coord);
    const auto & den2nrn = wafer().hicanns[hic_id].denmem2nrn;
    std::map<unsigned,unsigned> ret;
    ret.insert(den2nrn.begin(),den2nrn.end());
    return ret;
}

// returns a map that holds as pairs the denmem-addresses of all firing denmems
// (i.e. activate_firing == true and enable_spl1_output == true) and their
// L1_addresses
std::map<unsigned int, unsigned int> HALaccess::getFiringDenmemsAndSPL1Addresses(unsigned int x_coord, unsigned int y_coord) const
{
	size_t hic_id = to_id(x_coord, y_coord);
    LOG4CXX_DEBUG(logger, "Getting FiringDenmemsAndSPL1Addresses on HICANN " << hic_id);

	const auto &neurons = wafer().hicanns[hic_id].neurons_on_hicann;
	std::map<unsigned int, unsigned int> returnval;

	for(size_t i = 0; i < neurons.size(); ++i)
	{
		if (is_active(neurons[i])) {
			unsigned int l1 = neurons[i].l1_address;
            LOG4CXX_DEBUG(logger, "Inserting Denmem " << i << " with address " << l1);
			returnval.insert(std::pair<unsigned int, unsigned int>(i, l1));
        }
	}

	return returnval;
}

ESS::BioParameter HALaccess::getNeuronParametersSingleDenmem(
		unsigned int x_coord,
		unsigned int y_coord,
		unsigned int denmem) const
{
    using namespace HMF;
	using namespace HMF::Coordinate;

	size_t hic_id = to_id(x_coord, y_coord);
	const auto &neuron = wafer().hicanns[hic_id].neurons_on_hicann[denmem];

	// reverse trafo:
	// HMF::NeuronCalibration neuron_trafo;
	// neuron_trafo.setDefaults();

	NeuronCalibration const& neuron_trafo = *boost::dynamic_pointer_cast<NeuronCalibration const>(
	    getCalib(x_coord, y_coord)->atNeuronCollection()->at(denmem));

	HMF::NeuronCalibrationParameters nc_params;
	nc_params.shiftV = 0;
	nc_params.alphaV = 1;
	nc_params.bigcap = neuron.cap; // assumes that cap is equal for all connected neurons
	double const speedup = 1;
    const double big_cap   =   NeuronCalibration::big_cap;    //value of the big capacitance in nF
    const double small_cap =   NeuronCalibration::small_cap;	//value of the small capacitance in nF
	double const bio_cap = neuron.cap ? big_cap : small_cap; // bio cap = hw_cap

	auto adex_params = neuron_trafo.applyNeuronReverse(neuron.neuron_parameters, speedup, bio_cap, nc_params);

	// v_reset
	HMF::SharedCalibration shared_calibration;
	shared_calibration.setDefaults();

	HMF::HWSharedParameter hw_shrd_p;
	hw_shrd_p.setParam( HICANN::shared_parameter::V_reset,  neuron.V_reset);
	HMF::ModelSharedParameter m_shrd_p = shared_calibration.applySharedReverse(hw_shrd_p);

	static const double s_to_ms = 1.e3;
	adex_params.v_reset = m_shrd_p.v_reset*s_to_ms;

    ESS::BioParameter returnval;
	returnval.initFromPyNNParameters(adex_params);
    LOG4CXX_DEBUG(logger, "getNeuronParametersSingleDenmem()");
    LOG4CXX_DEBUG(logger, "cm " << returnval.cm );
    LOG4CXX_DEBUG(logger, "v_reset " << returnval.v_reset );
    LOG4CXX_DEBUG(logger, "g_l " << returnval.g_l );
    LOG4CXX_DEBUG(logger, "v_rest " << returnval.v_rest );
    LOG4CXX_DEBUG(logger, "a " << returnval.a );
    LOG4CXX_DEBUG(logger, "b " << returnval.b );
    LOG4CXX_DEBUG(logger, "tau_w " << returnval.tau_w );
    LOG4CXX_DEBUG(logger, "e_rev_E " << returnval.e_rev_E );
    LOG4CXX_DEBUG(logger, "e_rev_I " << returnval.e_rev_I );
    LOG4CXX_DEBUG(logger, "v_thresh " << returnval.v_thresh );
    LOG4CXX_DEBUG(logger, "delta_T " << returnval.delta_T );
    LOG4CXX_DEBUG(logger, "tau_syn_I " << returnval.tau_syn_I );
    LOG4CXX_DEBUG(logger, "tau_syn_E " << returnval.tau_syn_E );
    LOG4CXX_DEBUG(logger, "tau_refrac " << returnval.tau_refrac );
    LOG4CXX_DEBUG(logger, "v_spike " << returnval.v_spike );

    return returnval;
}

HMF::HWNeuronParameter HALaccess::getHWNeuronParameter(ESS::BioParameter const& params, unsigned int const hic_x, unsigned int const hic_y, unsigned int const nrn_id)
{
	using namespace HMF;
	size_t hic_id = to_id(hic_x, hic_y);
    ESS::BioParameter temp = params;
    // calculate the membrane capacity
    double tau = 1000*(params.cm) / (params.g_l); // 1000*nF/nS -> ms
    temp.tau_m     = tau;
    LOG4CXX_DEBUG(logger, "getHWNeuronParameter()");
    LOG4CXX_DEBUG(logger, "tau_m " << temp.tau_m );

	NeuronCalibration const& neuron_trafo = *boost::dynamic_pointer_cast<NeuronCalibration const>(
	    getCalib(hic_x, hic_y)->atNeuronCollection()->at(nrn_id));

	HMF::NeuronCalibrationParameters nc_params;
	nc_params.shiftV = 0;
	nc_params.alphaV = 1;

	double const speedup = 1;
	auto returnval = neuron_trafo.applyNeuronCalibration(temp, speedup, nc_params);
	return returnval;
}

bool HALaccess::getVoltageRecording(unsigned int x_coord, unsigned int y_coord, unsigned int denmem) const
{
	size_t hic_id = to_id(x_coord, y_coord);
	return wafer().hicanns[hic_id].neurons_on_hicann[denmem].enable_output;
}
    
std::string HALaccess::getVoltageFile(unsigned int x_coord, unsigned int y_coord, unsigned int denmem) const
{
	size_t hic_id = to_id(x_coord, y_coord);
	std::ostringstream hic;
	hic << hic_id;
    std::ostringstream den;
	den << denmem;
	std::string file = mFilepath + "/denmem_traces/" + "hicann_" + hic.str() + "denmem_address_" + den.str();
    return file;
}

bool HALaccess::getCurrentInput(unsigned int x_coord, unsigned int y_coord, unsigned int denmem) const
{
	size_t hic_id = to_id(x_coord, y_coord);
    return wafer().hicanns[hic_id].neurons_on_hicann[denmem].enable_curr_input;
}
    
ESS::StimulusContainer HALaccess::getCurrentStimulus(
            unsigned int x_coord,
            unsigned int y_coord,
            enum ESS::HICANNSide side,
            enum ESS::HICANNBlock block) const
{
	size_t hic_id = to_id(x_coord, y_coord);
	//determine the fg_address
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
    return wafer().hicanns[hic_id].stimulus_config[addr];
}


//returns the configuration of the 8 channels of dnc<->hicann-if. enable stores for each channel if it is activated. enable stores if the direction is tohicann (true) or to DNC (false)
void HALaccess::getDNCIFConfig(unsigned int x_coord, unsigned int y_coord, std::vector<ESS::DNCIfDirections> &directions, std::vector<bool> &enable) const
{
	size_t hic_id = to_id(x_coord, y_coord);

	directions.resize(ESS::size::dnc_channels,ESS::DNCIfDirections::OFF);
	enable.resize(ESS::size::dnc_channels,0);
	
	directions = wafer().hicanns[hic_id].dnc_link_directions;
	enable = wafer().hicanns[hic_id].dnc_link_enable;
}

//gets the config of the indicated repeaterblock from the corresponding data structure of HAL2ESS::Container
std::vector<unsigned char> HALaccess::getRepeaterConfig(unsigned int x_coord, unsigned int y_coord, ESS::RepeaterLocation rep_block) const
{
	size_t hic_id = to_id(x_coord, y_coord);

	std::vector<unsigned char> returnval = wafer().hicanns[hic_id].repeater_config[rep_block].repeater;
	return returnval;
}

//gets the CrossbarSwitch config of the indicated side from the corresponding data structure of HAL2ESS::Container
std::vector<std::vector<bool> > HALaccess::getCrossbarSwitchConfig(unsigned int x_coord, unsigned int y_coord, enum ESS::HICANNSide side) const
{
	size_t hic_id = to_id(x_coord, y_coord);

	size_t s = 0;
	if(side == ESS::S_LEFT)
		s = 0;
	else
		s = 1;

	std::vector<std::vector<bool> > returnval = wafer().hicanns[hic_id].crossbar_config[s];
	return returnval;
}

//gets the SyndriverSwitchConfig of the indicated location from the corresponding data structure of HAL2ESS::Container
std::vector<std::vector<bool> > HALaccess::getSyndriverSwitchConfig(
		unsigned int x_coord,
		unsigned int y_coord,
		enum ESS::HICANNBlock block,
		enum ESS::HICANNSide side) const
{
	size_t hic_id = to_id(x_coord, y_coord);

	//detemine the syndriverblock
	size_t synbl;
	if(side == ESS::HICANNSide::S_LEFT)
	{
		if(block == ESS::HICANNBlock::BL_UP)
			synbl = 1;
		else
			synbl = 0;
	}
	else
	{
		if(block == ESS::HICANNBlock::BL_UP)
			synbl = 3;
		else
			synbl = 2;
	}

	std::vector<std::vector<bool> > returnval = wafer().hicanns[hic_id].synswitch_config[synbl];
	return returnval;
}

//gets the MergerTreeConfig from the corresponding data-structure of HAL2ESSContainer::hicann
std::vector<std::vector<bool> > HALaccess::getSPL1OutputMergerConfig(unsigned int x_coord, unsigned int y_coord) const
{
	size_t hic_id = to_id(x_coord, y_coord);

	std::vector<std::vector<bool> > returnval = wafer().hicanns[hic_id].merger_tree_config;
	return returnval;
}
    
//configure the ADC
void HALaccess::configADC(unsigned int adc_coord, uint32_t samples)
{
    auto & config = wafer().adcs.at(adc_coord);
    config.num_samples = samples;
}
    
//prime the ADC
void HALaccess::primeADC(unsigned int adc_coord)
{
    auto & config = wafer().adcs.at(adc_coord);
    config.enable = true;
}

std::vector<uint16_t> HALaccess::getADCTrace(unsigned int adc_coord) const
{
    std::vector<uint16_t> returnval;
    using namespace HMF::Coordinate;
    //chceck if this adc was primed
    const auto & config = wafer().adcs.at(adc_coord);
    if(config.enable == false)
    {
        LOG4CXX_WARN(logger, "Trying to read out trace from ADC " << adc_coord << " which was not primed");
        return std::vector<uint16_t>{};
    }
    //get the number of sample points
    const uint32_t samples = config.num_samples;
    //check if constellation of aout for neurons on this RETICLE ! is valid
    const unsigned int reticle = adc_coord / 2; // address of the reticle this ADC belongs to
    //determine which neurons (on quad) does this ADC read out
    NeuronOnQuad neuron_on_quad;
    if(config.input[0] == true) //the bot_odd neuron is read out
    {
        neuron_on_quad = NeuronOnQuad(X(right),Y(bottom));
    }
    else if(config.input[1] == true) //the top_odd neuron is read out
    {
        neuron_on_quad = NeuronOnQuad(X(right),Y(top));
    }
    else if(config.input[2] == true) //the bot_even neuron is read out
    {
        neuron_on_quad = NeuronOnQuad(X(left),Y(bottom));
    }
    else if(config.input[3] == true) //the top_even neuron is read out
    {
        neuron_on_quad = NeuronOnQuad(X(left),Y(top));
    }
    //vector to store addresses of neurons w/ aout == true
    std::map<unsigned int,unsigned int> record_neurons;
    //loop over hicanns on this reticle 
    //get the reticle aka DNC coordinate
    const DNCOnWafer reticle_halbe{Enum{reticle}};
    for(auto hic_on_reticle : iter_all<HICANNOnDNC>())
    {
        const auto hicann_halbe = hic_on_reticle.toHICANNOnWafer(reticle_halbe);
        const unsigned int hic_id = hicann_halbe.id().value();
        const auto & hicann = wafer().hicanns.at(hic_id);
        //check if hicann is activated
        if(hicann.available == true)
        {
            //loop over quads on this hicann on this hicann
            for( auto quad : iter_all<QuadOnHICANN>())
            {
                NeuronOnHICANN neuron{quad,neuron_on_quad};
                unsigned int denmem = neuron.id().value();
                if(hicann.neurons_on_hicann.at(denmem).enable_output == true)
                {
                    //store neuron address and hicann address
                    record_neurons.insert(
                            std::pair<unsigned int,unsigned int>(hicann.hicann_id,denmem));
                }
            }
        }
    }
    //make sure that exactly 1 neuron is recorded
    if (record_neurons.size() == 0)
    {
        LOG4CXX_WARN(logger, "Trying to read out trace from ADC " << adc_coord << 
                " but aout was not activated for any neuron recorded by this ADC ");
        return returnval;
    }
    else if (record_neurons.size() > 1)
    {
        LOG4CXX_WARN(logger, "Trying to read out trace from ADC " << adc_coord << 
                " but aout was activated for " << record_neurons.size() << " neurons recorded by this ADC.");
        LOG4CXX_WARN(logger, " Aout activted for: " );
        for(auto nrn : record_neurons)
        {
            LOG4CXX_WARN(logger, " Neuron " << nrn.second << " on HICANN " << nrn.first );
        }
        return returnval;
    }
    else
    {
        const unsigned int hic_id = record_neurons.begin()->first;
        const unsigned int nrn_id = record_neurons.begin()->second;
        LOG4CXX_DEBUG(logger, "Recording Trace of ADC " << adc_coord << " belonging to neuron " << nrn_id << " on HICANN " << hic_id);
        //read in the trace
        returnval = read_analog_trace(hic_id,nrn_id,samples);
    }
    return returnval;
}

ESS::BioParameter HALaccess::getScaledBioParameter(ESS::BioParameter const & parameter) const
{
	LOG4CXX_ERROR(logger, "HALaccess::getScaledBioParameter(). Function makes no sense. scaling to bio parameters requires more parametes like speedup ...");
	return parameter;
}

//reads in the analog trace of a neuorn from file
std::vector<uint16_t> HALaccess::read_analog_trace(unsigned int hicann, unsigned int nrn, uint32_t samples) const
{
    std::vector<uint16_t> returnval;
    //get the hicann x,y coordinate
    HMF::Coordinate::HICANNOnWafer hic_halbe{HMF::Coordinate::Enum{hicann}};
    unsigned int hicann_x = hic_halbe.x().value();
    unsigned int hicann_y = hic_halbe.y().value();
    //check if the trace of this neuron was recorded
    if (!getVoltageRecording(hicann_x,hicann_y,nrn) )
    {
        LOG4CXX_WARN(logger, "Recording Trace of Neuron " << nrn << " on HICANN " 
                << hicann << " which trace was NOT recorded during the run of the ESS" );
        return returnval;
    }
    
    //get the filepath
    std::string filepath = getVoltageFile(hicann_x,hicann_y,nrn);
    LOG4CXX_DEBUG(logger, "Recording Trace of Neuron " << nrn << " on hicann " << hicann << " from file " << filepath);
    //initialize the filestream
	std::ifstream read_in{filepath, std::ifstream::in};
	
    //define constants
    //data format: the voltage value has a length of 12 characters
	const size_t len = 12;
	//the maximal voltage value is 1.8 V
    const double v_max	= 1.8;
    //the maximal value of uint16 is 2^16 - 1 = 65535
	const uint16_t uint16_max = 65535;
    
	//read in sample lines
    //skip the first line which contains the header
	std::string _;
	getline(read_in,_);
	//read in
    size_t i = 0;
	while (i < samples && read_in.good())
	{
		std::string line;
		getline(read_in,line);
		//data format: the voltage value begins after the first tab and has a length of len = 12 character
		size_t const tab_pos = line.find("\t");
        std::string val_str;
        try {
            val_str = line.substr(tab_pos, tab_pos + len);
        }
        catch (...) {
            LOG4CXX_WARN(logger, "While recording Trace of Neuron " << nrn << " on HICANN " 
                    << hicann << " a blank line (number " << i << ") was read in, trying to read in  " 
                    << samples << " lines. Recording of trace stopped.");
            break;
        }
		//convert the string value (which corresponds to a floating point number) to a double
		double val_double = std::stof(val_str);
		//convert the double value to a uint16_t
        uint16_t val_int = (val_double / v_max ) * uint16_max;
        //clip the value
        if (val_double < 0)
        {
            val_int = 0;
            LOG4CXX_DEBUG(logger, "Recording Trace of Neuron" << nrn << " on HICANN " 
                    << hicann << " value " << i << " = " << val_double << " clipped to " << 0 );
        }
        if (val_double > v_max)
        {
            val_int = uint16_max;
            LOG4CXX_DEBUG(logger, "Recording Trace of Neuron" << nrn << " on HICANN " 
                    << hicann << " value " << i << " = " << val_double << " clipped to " << uint16_max );
        }
		returnval.push_back(val_int);
		i++;
	}
	read_in.close();
    return returnval;
}

//stuff for the graph used to recollect nrns from the denmem switch config
//datastructure for the graph
struct HALaccess::node {
    node* vert_conn;        //pointer to the vertical node
    node* hori_conn;
    unsigned int denmem_id;
};

//builds the graph of interconnected denmems 
void HALaccess::build_graph(size_t hic_id, std::array<node,512> & nodes) const
{
	const auto & Switches = wafer().hicanns[hic_id].connection_config;
	const auto & Dends = wafer().hicanns[hic_id].neurons_on_hicann;

    //build the graph
    size_t offset = 256;
    node* curr_top = &nodes[0];
    node* curr_bot = &nodes[offset];
    for(size_t i = 0; i < offset-1; ++i)
    {
        node* next_top = &nodes[i+1];
        node* next_bot = &nodes[offset+i+1];
        //set the horizontal connections inside the denmem quad
        //these connections ar not directed so it is sufficient to check every second step if they are set
        if(i%2==0)
        {
            //upper switches
            if(Switches[i/2].hori[0] == true)
            {
                curr_top->hori_conn = next_top;
            }
            //lower switches
            if(Switches[i/2].hori[1] == true)
            {
                curr_bot->hori_conn = next_bot;
            }
        }
        //set the horizontal connections between denmem quads
        //these connections are directed, but this is neglected, because it does not matter in the ESS
        //make sure that there are no connections over neuron block boundaries (i%32 != 0, i%32 != 31)
        if(i%32 !=0 && i%32 != 31)
        {
            //upper switches
            if(Dends[i].enable_fire_input == true) 
            {
                if(i%2==1)
                {
                    curr_top->hori_conn = next_top;
                }
                else if (i%2==0)
                {
                    //check for combinations of enable_fire_input that are not valid
                    if(Dends[i-1].enable_fire_input == true)
                    {
                        LOG4CXX_ERROR(logger, "Getting nrns from denswitches on HICANN " << hic_id << " invalid combination: enable_fire_input = true on denmems: " << i-1 << " and " << i);
                        throw std::runtime_error("HALaccess: Getting nrns from denswitches : invalid combination: enable_fire_input = true ");
                    }
                    else
                    {
                        nodes[i-1].hori_conn = curr_top;
                    }
                }
            }
            //lower switches
            if(Dends[offset+i].enable_fire_input == true) 
            {
                if(i%2==1)
                {
                    curr_bot->hori_conn = next_bot;
                }
                else if (i%2==0)
                {
                    //check for combinations of enable_fire_input that are not valid
                    if(Dends[offset+i-1].enable_fire_input == true)
                    {
                        LOG4CXX_ERROR(logger, "Getting nrns from denswitches on HICANN " << hic_id << " invalid combination: enable_fire_input = true on denmems: " << offset+i-1 << " and " << offset+i);
                        throw std::runtime_error("HALaccess: Getting nrns from denswitches : invalid combination: enable_fire_input = true ");
                    }
                    nodes[offset+i-1].hori_conn = curr_bot;
                }
            }
        }
        //set the vertical connections
        if(i%2==0 && Switches[i/2].vert[0] == true )
        {
            curr_top->vert_conn = curr_bot;
            curr_bot->vert_conn = curr_top;
        }
        else if(i%2==1 && Switches[i/2].vert[1] == true )
        {
            curr_top->vert_conn = curr_bot;
            curr_bot->vert_conn = curr_top;
        }
        
        curr_top = next_top;
        curr_bot = next_bot;
    }
}

void HALaccess::visit(size_t nrn_id, node const * curr_node, std::map<size_t,size_t> & den2nrn, std::vector<ESS::neuron> & denmem, std::array<bool,512> & visited)
{
    size_t den_id = curr_node->denmem_id;
    if(visited[den_id] == false)
    {
        den2nrn.insert(std::pair<size_t,size_t>(den_id,nrn_id));
        denmem[den_id].is_connected = true;
        visited[den_id] = true;
        if(curr_node->hori_conn != NULL)
        {
            visit(nrn_id,curr_node->hori_conn,den2nrn,denmem,visited);
        }
        if(curr_node->vert_conn != NULL)
        {
            visit(nrn_id,curr_node->vert_conn,den2nrn,denmem,visited);
        }
    }
}

//search the graph stored in nodes via depth first algorithm to collect all denmems connected with the den_id-th denmem
//data is stored in den2nrn
void HALaccess::search_graph(size_t hic_id, size_t nrn_id, node const* start_node, std::map<size_t,size_t> & den2nrn)
{
	auto & denmem = wafer().hicanns[hic_id].neurons_on_hicann;
    std::array<bool,512> visited;
    visited.fill(false);
    visit(nrn_id,start_node,den2nrn,denmem,visited);
    //check if the starting node really is connected
    if(start_node->hori_conn == NULL && start_node->vert_conn == NULL)
        denmem[start_node->denmem_id].is_connected = false;
}

//return the next (lowest) denmem address, that is not part of den2nrn (key)
//if all denmem were processed, i.e. all denmem addresses are part of den2nrn return 513 > 512 = # of dennen
size_t HALaccess::next_missing_denmem(std::map<size_t,size_t> const & den2nrn) const
{
    size_t i = 0;
    for(auto it = den2nrn.begin(); it != den2nrn.end(); ++it)
    {
        if(i != it->first)
        {
            break;
        }
        i++;
    }
    return i;
}


void HALaccess::Denmem2Neuron(size_t hic_id)
{
    //initialize all nodes
    std::array<node,512> nodes;
    for(size_t id = 0; id < nodes.size(); id++)
    {
        nodes[id].denmem_id = id;
        nodes[id].vert_conn = NULL;
        nodes[id].hori_conn = NULL;
    }
    
    //build the graph
    build_graph(hic_id,nodes);
    
    //search graph to get connected denmem
    size_t den_id  = 0; //counter for the denmem
    size_t log_nrn = 0; //counter for the logical neuron
    std::map<size_t,size_t> den2nrn_aux;
    while(den_id < nodes.size())
    {
        node* curr_node = &nodes[den_id]; 
        search_graph(hic_id, den_id, curr_node, den2nrn_aux);
        den_id = next_missing_denmem(den2nrn_aux);
        log_nrn++;
    }

    //write the denmem2neuron configuration to data
    auto & den2nrn = wafer().hicanns[hic_id].denmem2nrn;
	const auto & denmem = wafer().hicanns[hic_id].neurons_on_hicann;
    den2nrn.clear();
    //map for logical neurons that were set to logical neurons that will actually be build
    std::map<size_t, int> log_nrn_map;
    //logical nrn count
    int log_nrn_num = -1;
   
    LOG4CXX_DEBUG(logger, "On HICANN " << hic_id << " connecting denmem to logical neurons:" ); 
    for (auto it = den2nrn_aux.begin(); it != den2nrn_aux.end(); it++)
    {
		LOG4CXX_DEBUG(logger, "den2nrn_aux[" << it->first << "] = " << it->second );
        //only write denmem, which shall be activated
		if (is_active(denmem[it->first]) || denmem[it->first].is_connected == true) {
			//check if log_nrn (a.k.a it->second) has already been added
            if (log_nrn_map.find(it->second) == log_nrn_map.end())
            {
                log_nrn_num++;
                log_nrn_map.insert( std::pair<size_t, int>(it->second, log_nrn_num) );
            }
            den2nrn.insert( std::pair<size_t,size_t>(it->first, log_nrn_map[it->second]) );
            LOG4CXX_DEBUG( logger, "Denmem " << it->first << " connected to logical Neuron " << log_nrn_map[it->second] );
        }
        
    }
    
    //check that for all hw_neuron exactly one denmem is activated (fires and sends spl1 output)
    std::map<size_t,size_t> firing_count;
    for(auto it = den2nrn.begin(); it != den2nrn.end(); it++)
    {
        size_t denmem_id = it->first;
        size_t log_nrn_id = it -> second;
        auto it_fire = firing_count.find(log_nrn_id);
        if (it_fire != firing_count.end() && is_active(denmem[denmem_id]))
        {
            firing_count[log_nrn_id]++;
        }
        else if (it_fire == firing_count.end() && is_active(denmem[denmem_id]))
        {
            firing_count.insert(std::pair<size_t,size_t>(log_nrn_id,1));
        }
    }
    for(auto it_fire = firing_count.begin();it_fire != firing_count.end();it_fire++)
    {
        if(it_fire->second > 1)
        {
			LOG4CXX_ERROR(logger, "On HICANN "
			                          << hic_id
			                          << " more than one denmem activated (activate_firing == "
			                             "true and enable_spl1_output == true) for logical neuron "
			                          << it_fire->first);
			throw std::runtime_error(
			    "HALaccess: more than one denmem activated (activate_firing == "
			    "true and enable_spl1_output == true) for a logical neuron");
		}
        if(it_fire->second == 0)
        {
			LOG4CXX_ERROR(logger, "On HICANN "
			                          << hic_id
			                          << " no denmem activated (activate_firing == "
			                             "true and enable_spl1_output == true) for logical neuron "
			                          << it_fire->first);
			throw std::runtime_error("HALaccess: no denmem activated (activate_firing == "
			                         "true and enable_spl1_output == true) for a logical neuron");
		}
    }
}

std::shared_ptr<HALaccess::calib_type> HALaccess::getCalib(
    unsigned int x_coord, unsigned int y_coord) const
{
	size_t hic_id = to_id(x_coord, y_coord);
	auto it_calib = mCalibs.find(hic_id);
	if (it_calib != mCalibs.end()) {
		return it_calib->second;
	} else {
		LOG4CXX_WARN(
		    logger, "HALaccess::getCalib(): No calibration data initialized for HICANN "
		                << hic_id << ".  Using the default calibration instead (Did you call "
		                             "HICANN::init() after starting the simulation?)");
		return mDefaultCalib;
	}
}

void HALaccess::setCalibPath(std::string path)
{
	mCalibPath = path;
	LOG4CXX_INFO(logger, "set calib path to " << mCalibPath);
}

void HALaccess::initCalib()
{
	using namespace calibtic;
	using namespace calibtic::backend;

	if (mCalibPath == "") {
		LOG4CXX_INFO(logger, "Using the default calibration (No calibration path specified)");
		for (auto hicann : wafer().hicanns) {
			if (hicann.available) {
				mCalibs[hicann.hicann_id] = mDefaultCalib;
			}
		}
		return;
	}

	LOG4CXX_INFO(logger, "Loading calibration data from path: " << mCalibPath);

	auto lib = loadLibrary("libcalibtic_xml.so");
	auto backend = loadBackend(lib);

	if (!backend) {
		throw std::runtime_error("unable to load calibtic xml backend");
	}

	backend->config("path", mCalibPath); // search in mCalibPath for calibration xml files
	backend->init();

	calibtic::MetaData md;

	for (auto hicann : wafer().hicanns) {
		if (hicann.available) {
			std::stringstream calib_file;
			calib_file << "w" << mWaferId;
			calib_file << "-h" << hicann.hicann_id;
			const std::string calib_file_string = calib_file.str();
			const std::string calib_file_path = mCalibPath + "/" + calib_file_string + ".xml";

			if (boost::filesystem::exists(boost::filesystem::path(calib_file_path))) {
				LOG4CXX_DEBUG(logger, "Loading calibration file: " << calib_file_path);
				std::shared_ptr<calib_type> calib(new calib_type);
				backend->load(calib_file_string, md, *calib);
				mCalibs[hicann.hicann_id] = calib;
			} else {
				LOG4CXX_WARN(
				    logger, "Calibration file: "
				                << calib_file_path
				                << " does not exist. Using the default calibration for HICANN "
				                << hicann.hicann_id << " instead");
				mCalibs[hicann.hicann_id] = mDefaultCalib;
			}
		}
	}
}
