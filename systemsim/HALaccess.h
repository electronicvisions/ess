#pragma once

#include <vector>
#include <map>
#include <array>
#include <string>
#include <memory>

#include "HAL2ESSEnum.h"
#include "HAL2ESSContainer.h"


// forward declaration
namespace HMF {
class HICANNCollection;
}

class HALaccess
{
public:
	typedef HMF::HICANNCollection calib_type;

	// Constructor
	HALaccess(unsigned int wafer_id, std::string filepath);

	~HALaccess();

	//method that returns the hicann-x-coordinate on the wafer
	unsigned int getHicannXCoordinate(unsigned int hicann_id) const;

	//method that returns the hicann-y-coordinate on the wafer
	unsigned int getHicannYCoordinate(unsigned int hicann_id) const;

    uint8_t getPLLFrequency(unsigned int hicann_id) const;

	//method that returns number a vector, which size corresponds to the number of neurons to be mapped on this hicann
	unsigned int getNeuronsOnHicann(unsigned int x_coord, unsigned int y_coord );

	//method that returns the global parameters as a map: speedup_factor, timestep, weightdistortion (only standard deviation?)
    ESS::global_parameter const& getGlobalHWParameters() const;
    ESS::global_parameter & getGlobalHWParameters();

	//returns the synapse array
	std::vector<std::vector<ESS::Synapse_t> > getSynapses(unsigned int x_coord, unsigned int y_coord ) const;

	//method that returns the configuration of the Syndrivers as a vector of syndriver_cfg_t
	std::vector<ESS::syndriver_cfg> getSyndriverConfig(
			unsigned int x_coord,
			unsigned int y_coord,
			enum ESS::HICANNSide side) const;

	//method that returns the syndriver parameter as a map, indication of syndriver-address as above
    ESS::SyndriverParameterESS getSyndriverParameters(
			unsigned int x_coord,
			unsigned int y_coord,
			enum ESS::HICANNBlock block,
			enum ESS::HICANNSide side,
			unsigned int driver_in_quadrant //!< low-level hw counting: from center of chip to top/bottom
			) const;

	//method that indicates which dendrites=denmem! form a particular neuron and which WTA-address (whateve...) belongs to which neuron
	//by writing this into two vectors. the dends2neurons has 512 entrys, each representing a denmen. the value of these entries specifies
	// which neuron is formed out of this denmem
	std::map<unsigned, unsigned> getDendrites(
			unsigned int x_coord,
			unsigned int y_coord) const;

	//method that returns map which links Denmem to their SPL1 Addresses
	std::map<unsigned int, unsigned int> getFiringDenmemsAndSPL1Addresses(unsigned int x_coord, unsigned int y_coord) const;

	// returns HW neuron parameters of a neuron
    ESS::HWParameter getHWNeuronParameter(ESS::BioParameter const& params, unsigned int const hic_x, unsigned int const hic_y, unsigned int const nrn_id);

	// returns neuron parameters of a single denmem
    ESS::BioParameter getNeuronParametersSingleDenmem(unsigned int x_coord, unsigned int y_coord, unsigned int denmem) const;

	//methods for file recording and current input of a single denmem 
	bool getVoltageRecording(unsigned int x_coord, unsigned int y_coord, unsigned int denmem) const;
    std::string getVoltageFile(unsigned int x_coord, unsigned int y_coord, unsigned int denmem) const;
	bool getCurrentInput(unsigned int x_coord, unsigned int y_coord, unsigned int denmem) const;

    ESS::StimulusContainer getCurrentStimulus(
            unsigned int x_coord,
            unsigned int y_coord,
            enum ESS::HICANNSide side,
            enum ESS::HICANNBlock block) const;
    
    //method that configures the DNC-IF by working on two vectors. the vectors represent the dnc_if-channels. directions specifies
	void getDNCIFConfig(
			unsigned int x_coord,
			unsigned int y_coord,
			std::vector<ESS::DNCIfDirections> &directions,
			std::vector<bool> &enable) const;

	//method that configures the repeaters by returnig a vector, normal repeaters as well as spL1-repeaters are configuered, but i dont 
	//know what happens exactly,currently has some repeater class, which indicates the repeater location as input
	std::vector<unsigned char> getRepeaterConfig(
			unsigned int x_coord,
			unsigned int y_coord,
			ESS::RepeaterLocation rep_block) const; //input needs to be reworked

	//method that configures the crossbar switches, returns a vector of boolean vectors, representing the configuration of the crossbar-
	//switch-matrix. gets input which indicates the position of the crossbar switch
	std::vector<std::vector<bool> > getCrossbarSwitchConfig(unsigned int x_coord, unsigned int y_coord, enum ESS::HICANNSide side) const; //input needs to be reworked  

	//method that configures the Syndriver-Switches by returning a vector of boolean vectors, which represents the configuration of the 
	//SyndriverSwitch, gets input which indicates the position of the syndriver
	std::vector<std::vector<bool> > getSyndriverSwitchConfig(
			unsigned int x_coord,
			unsigned int y_coord,
			enum ESS::HICANNBlock block,
			enum ESS::HICANNSide side) const; //input needs to be reworked

	//method that configures the spL1-OutputMerger-Config, returns a vector of boolean vectors
	std::vector<std::vector<bool> > getSPL1OutputMergerConfig(unsigned int x_coord, unsigned int y_coord) const;

    //configure the ADC
    void configADC(unsigned int adc_coord, uint32_t samples);
    
    //prime the ADC
    void primeADC(unsigned int adc_coord);

    //get the recorded Neuron Trace from virtual ESS ADC
    std::vector<uint16_t> getADCTrace(unsigned int adc_coord) const;

    // returns the ADEX-model parameter of the specified neuron
    ESS::BioParameter getScaledBioParameter(ESS::BioParameter const & parameter) const;

	ESS::wafer & wafer()
	{ return waf; }
	const ESS::wafer & wafer() const
	{ return waf; }

	void setCalibPath(std::string path);

	/// Initialize the calibration data used in the simulation.
	/// Loads the calibration data from the directory specified via
	/// setCalibPath for all used HICANNs.
	/// The calibration is looked up per HICANN in xml files with pattern
	/// "w<W>-h<H>.xml", where <W> is the wafer ID and <H> the HICANN Id, e.g.
	/// w0-h276.xml
	/// If no directory was specified, the default calibration is used.
	void initCalib();

private:
    std::vector<uint16_t> read_analog_trace(unsigned int const hicann, unsigned int const nrn, uint32_t const samples) const;
    //stuff for Denmem2NeuronV2
    struct node;
    void build_graph(size_t hic_id, std::array<node,512> & nodes) const;
    void visit(size_t nrn_id, node const* curr_node, std::map<size_t,size_t> & den2nrn, std::vector<ESS::neuron> & denmem, std::array<bool,512> & visited);
    void search_graph(size_t hic_id, size_t nrn_id, node const * start_node, std::map<size_t, size_t> & den2nrn);
    size_t next_missing_denmem(std::map<size_t, size_t> const &  den2nrn) const;
    void Denmem2Neuron(size_t hic_id);
	// get row-wise synapse trafo from calibration data
	// returns vector of coefficients of a polynomial trafo (index equals degree) from digital to
	// analog weights in Siemens.
	// returns empty vector if synapse row is not used.
	std::vector<double> get_row_wise_synapse_trafo(
	    unsigned int x_coord,
	    unsigned int y_coord,
	    HMF::Coordinate::SynapseRowOnHICANN row_coord,
	    ESS::syndr_row row_cfg) const;

	std::shared_ptr<calib_type> getCalib(unsigned int x_coord, unsigned int y_coord) const;

	ESS::wafer waf;
    ESS::global_parameter mGlobalParams;
	unsigned int mWaferId; ///< wafer id needed for calibration data
	std::string mFilepath;
	std::string mCalibPath;
	std::shared_ptr<calib_type> mDefaultCalib;
	std::map<unsigned int, std::shared_ptr<calib_type> > mCalibs;
};
