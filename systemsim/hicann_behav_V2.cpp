#include "hicann_behav_V2.h"
#include "sim_def.h"

#include "HALaccess.h"
#include "HMF/SynapseDecoderDisablingSynapse.h"

//include members that were forward declared
#include "l1_behav_V2.h"
#include "anncore_behav.h"
#include "dnc_if.h"
#include "spl1_merger.h"
#include "l2_dnc.h"
#include <log4cxx/logger.h>
#include <stdexcept>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("ESS.HICANN");

hicann_behav_V2::hicann_behav_V2(
			sc_module_name hicann_i,   //!< instance name
			short hicannid,            //!< identification id
			mutexfile& spike_rcx_file, //!< FILE handle for received events
			mutexfile& spike_tcx_file, //!< FILE handle for transmitted events
			std::string temp_folder,   //!< Folder for temporary and debug files during simulation
			HALaccess* hala,           //!< Pointer to the Reference of HALaccess
			short hicann_on_dnc
			) : sc_module(hicann_i)
				,hicannid(hicannid)
				,sim_folder(temp_folder)
				,hal_access(hala)
{
    LOG4CXX_INFO(logger, "Building Hicann " << hicannid);

    //set PLL_frequency
    uint8_t PLL = hal_access->getPLLFrequency(hicannid);
    
    //calculate the PLL_period
    unsigned int PLL_period_ns = 1000 / PLL; //calculate the period of the PLL (in MHz) in ns 
    
    LOG4CXX_DEBUG(logger, "Hicann: " << hicannid << " setting PLLFrequency to " << (int) PLL << " MHz correspoding to Period " << PLL_period_ns  << " ns");

    //check if timed merger or non-timed merger
    bool enable_timed_merger = hal_access->getGlobalHWParameters().enable_timed_merger;

    //check if spike_debugging is activated
    bool enable_spike_debugging = hal_access->getGlobalHWParameters().enable_spike_debugging;

    ////////////////////
	// Create submodules
	////////////////////

	l1_behav_i = std::unique_ptr<l1_behav_V2>(new l1_behav_V2(hicannid, sim_folder, (const char *)hicann_i));
	
	dnc_if_i = std::shared_ptr<dnc_if>(new dnc_if("dnc_if_i",hicann_on_dnc));
	
	anncore_behav_i = std::shared_ptr<anncore_behav>(new anncore_behav("anncore_behav", hicannid, spike_rcx_file, PLL_period_ns,enable_spike_debugging));
	
	spl1_merger_i = std::unique_ptr<spl1_merger>(new spl1_merger("spl1_merger_i",hicannid,spike_tcx_file, PLL_period_ns,enable_timed_merger,enable_spike_debugging));


	////////////////////
	// Connect submodules
	////////////////////

	// Anncore -> SPL1-Merger
	anncore_behav_i->out_port( *spl1_merger_i.get() );

	// DNC_IF  -> SPL1-Merger
	for(int i = 0; i< DNCL1BUSINCOUNT; i++)
		dnc_if_i->l2tol1_tx_i[i]->l1bus_tx_if( *spl1_merger_i.get() );

	// SPL1-Merger -> DNC_IF
	spl1_merger_i->dnc_if_if( *dnc_if_i.get() );

	// SPL1-Merger -> L1
	spl1_merger_i->l1_task_if = l1_behav_i.get();

	// L1 -> Anncore
	l1_behav_i->anncore_if = anncore_behav_i;

    LOG4CXX_INFO(logger, "Hicann " << hicannid << "successfully built!");
    LOG4CXX_DEBUG(logger, "SystemC-Object-Name: " << this->name() );
}

hicann_behav_V2::~hicann_behav_V2()
{}

/// function to get access to GraphModel for config of Hicann and Submodules
void hicann_behav_V2::config_hicann(unsigned int id)
{
	x_coord = hal_access->getHicannXCoordinate(id);
	y_coord = hal_access->getHicannYCoordinate(id);

    LOG4CXX_INFO(logger, "Configuring Hicann with ID: " << hicannid << ", X=" << x_coord << ", Y=" << y_coord);
	
	// assign pointer to graph model access class to the layer1net
	config_l1net();

	config_spl1_merger();

	// Configure Anncore
	config_anncore();
	
	// Configure Dnc_if channels
	config_dncif();
    LOG4CXX_INFO(logger, "Building Hicann Nr "<< hicannid << "(X_coord=" << x_coord << ", Y_coord=" << y_coord << ")  configured." );
}

void hicann_behav_V2::config_anncore()
{
    unsigned int nrns_on_hic = hal_access->getNeuronsOnHicann(x_coord, y_coord);
	if(nrns_on_hic>0)
	{
	// Get Global HW Parameters
	auto global_parameters = hal_access->getGlobalHWParameters();	
	double speedup = 10000;
	double bio_timestep = 0.01;

	speedup = global_parameters.speedup;
    LOG4CXX_DEBUG(logger,  "GlobalParameter: speedupFactor \t" << speedup);

	bio_timestep = global_parameters.timestep;
    LOG4CXX_DEBUG(logger, "GlobalParameter: timestep(bio)\t" << bio_timestep << "\t in ms");

	double hw_timestep = bio_timestep/speedup;
	double max_hw_timestep =  0.01/speedup;
	if (hw_timestep > max_hw_timestep) {
		hw_timestep = max_hw_timestep;
        LOG4CXX_DEBUG(logger, "Set hardware timestep to \t" << hw_timestep << "\t in ms for a higher precisions in the neurons");
	}
	
    double weight_distortion_stddev = 0.;
	double weight_distortion_mean = 1.;

    if(global_parameters.enable_weight_distortion == true)
    {
	    weight_distortion_stddev = global_parameters.weight_distortion;
        LOG4CXX_DEBUG(logger, "GlobalParameter: weight_distortion_stddev \t" << weight_distortion_stddev );
	    anncore_behav_i->set_weight_distortion(weight_distortion_mean , weight_distortion_stddev );
	}
    else
    {
        LOG4CXX_DEBUG(logger, "Global Parameter: Weight distortion disabled" );
        anncore_behav_i->disable_weight_distortion();
    }
	unsigned int i,j;
	// Configure Addresses
	// first create vector with addresses and weights
	std::vector<char> addresses;
	std::vector<char> weights;
	std::vector<char> syndr_mirrors;

	////////////////////////////////////
	//	Configuration of SYNAPSES
	////////////////////////////////////

	// initialize all addresses with the 4-bit address, that was chosen to disable a synapse and all weights to 0.
	// FIXME: PM: refactor so that both mapping frameworks are compatible at once -> set disabled address in mapping, not here
    const int tot_synapses = SYN_PER_ROW*ROWS_PER_SYNDRIVER*SYNDRIVERS*2;
    addresses.resize(tot_synapses,HMF::HICANN::four_bit_address_disabling_synapse);
	weights.resize(tot_synapses,0);		// init with 0 (inactive)

	// Synapse Access: get Configuration from HALbe
	std::vector<std::vector<ESS::Synapse_t> > synapses = hal_access->getSynapses(x_coord, y_coord);
    //make sure the sizes match
    const int size_read = synapses.size()*synapses[0].size();
    if (tot_synapses != size_read)
    {
        LOG4CXX_ERROR(logger, "On HICANN: " << hicannid << " Size of SynapseArrays does not match: Size expected = " << tot_synapses << " != Size read = " << size_read );
        throw std::runtime_error("hicann_behav:  Size of SynapseArrays does not match");
    }
    LOG4CXX_INFO(logger, "Synapses read on Hicann " << hicannid << ".");

	int syn_count = 0;
    LOG4CXX_DEBUG(logger, "outer vector size: " << synapses.size() << " inner vector size: " << synapses[0].size() );

	// Be careful here: structure of synapses vector vector: synapses[i=column][j=row]
	for(i=0; i<synapses.size();i++){
		for(j=0; j<synapses[i].size(); j++){
			ESS::Synapse_t  current_synapse = synapses[i][j];
			addresses[j*SYN_PER_ROW + i]=current_synapse.address;
			weights[j*SYN_PER_ROW + i]=current_synapse.weight;
			syn_count++;
            //only log if the synapse is active (weight != 0)
            if (current_synapse.weight != 0)
            {
                LOG4CXX_TRACE(logger, "Synapse[" << i << "][" << j << "]: address(" << (size_t)synapses[i][j].address << "), weight(" << (size_t)synapses[i][j].weight << ")" );
		    }
        }
	}
	
	// Send CONFIG to ANNCORE	
	anncore_behav_i->configAddressDecoders(addresses);
	anncore_behav_i->configWeights(weights);

	/////////////////////////////////////////////////////////
	//<	Configuration of SYNDRIVERS	
    ///////////////////////////////////////////////////////////
	std::vector< ESS::syndriver_cfg > syndr_config_left  = hal_access->getSyndriverConfig(x_coord, y_coord, ESS::HICANNSide::S_LEFT);
	std::vector< ESS::syndriver_cfg > syndr_config_right = hal_access->getSyndriverConfig(x_coord, y_coord, ESS::HICANNSide::S_RIGHT);
	std::vector< ESS::syndriver_cfg > syndr_config = syndr_config_left;
	syndr_config.insert(syndr_config.end(), syndr_config_right.begin(), syndr_config_right.end());
	// Get Syndriver Parameters
	for(i=0;i<2*SYNDRIVERS;i++){

        ESS::SyndriverParameterESS syndrv_params;
		if(i < SYNDRIVERS/2)
			syndrv_params = hal_access->getSyndriverParameters(x_coord, y_coord, ESS::HICANNBlock::BL_UP, ESS::HICANNSide::S_LEFT, i%(SYNDRIVERS/2));
		else if(i >= SYNDRIVERS/2 && i<SYNDRIVERS)
			syndrv_params = hal_access->getSyndriverParameters(x_coord, y_coord, ESS::HICANNBlock::BL_DOWN, ESS::HICANNSide::S_LEFT, i%(SYNDRIVERS/2));
		else if(i >= SYNDRIVERS && i<SYNDRIVERS*3/2)
			syndrv_params = hal_access->getSyndriverParameters(x_coord, y_coord, ESS::HICANNBlock::BL_UP, ESS::HICANNSide::S_RIGHT, i%(SYNDRIVERS/2));
		else if(i >= SYNDRIVERS*3/2 && i<SYNDRIVERS*2)
			syndrv_params = hal_access->getSyndriverParameters(x_coord, y_coord, ESS::HICANNBlock::BL_DOWN, ESS::HICANNSide::S_RIGHT, i%(SYNDRIVERS/2));

        //log the syndriver params for each block
        if(i == 0 || i == SYNDRIVERS/2 || i == SYNDRIVERS || i == 3*SYNDRIVERS/2)
        {
            std::string block;
            if (i == 0)
                block = "TopLeft(syndrivers0-55)";
            if (i == SYNDRIVERS/2)
                block = "BottomLeft(syndrivers56-111)";
            if (i == SYNDRIVERS)
                block = "TopRight(syndrivers112-167)";
            if (i == 3*SYNDRIVERS/2)
                block = "BottomRight(syndrivers167-223)";
            LOG4CXX_DEBUG(logger, "Parameters for Syndrivers in block " << block << "on hicann " << hicannid );

			std::stringstream ss_g_trafo_up;
			for (auto coeff : syndrv_params.g_trafo_up)
				ss_g_trafo_up << coeff << " ";
            LOG4CXX_DEBUG(logger, "Syndrv Parameter: g_trafo_up: \t" << ss_g_trafo_up.str() << "\t in Siemens " );

			std::stringstream ss_g_trafo_down;
			for (auto coeff : syndrv_params.g_trafo_down)
				ss_g_trafo_down << coeff << " ";
            LOG4CXX_DEBUG(logger, "Syndrv Parameter: g_trafo_down: \t" << ss_g_trafo_down.str() << "\t in Siemens " );

            LOG4CXX_DEBUG(logger, "Syndrv Parameter: N_dep: \t" << syndrv_params.N_dep );
            LOG4CXX_DEBUG(logger, "Syndrv Parameter: N_fac: \t" << syndrv_params.N_fac );
            LOG4CXX_DEBUG(logger, "Syndrv Parameter: tau_rec: \t" << syndrv_params.tau_rec << "\t in s" );
            LOG4CXX_DEBUG(logger, "Syndrv Parameter: lambda: \t" << syndrv_params.lambda );
        }

		// using data from config_struct:
        ESS::syndriver_cfg& config = syndr_config.at(i);
        anncore_behav_i->configSyndriver(i, config, syndrv_params.g_trafo_up, syndrv_params.g_trafo_down,
				syndrv_params.tau_rec, syndrv_params.lambda, syndrv_params.N_dep, syndrv_params.N_fac);
	}

    ////////////////////////////////////
    //< Configuration of current stimuli
    ////////////////////////////////////
    
    ESS::StimulusContainer stim0 = hal_access->getCurrentStimulus(x_coord, y_coord, ESS::HICANNSide::S_LEFT, ESS::HICANNBlock::BL_UP); 
    ESS::StimulusContainer stim1 = hal_access->getCurrentStimulus(x_coord, y_coord, ESS::HICANNSide::S_LEFT, ESS::HICANNBlock::BL_DOWN); 
    ESS::StimulusContainer stim2 = hal_access->getCurrentStimulus(x_coord, y_coord, ESS::HICANNSide::S_RIGHT, ESS::HICANNBlock::BL_UP); 
    ESS::StimulusContainer stim3 = hal_access->getCurrentStimulus(x_coord, y_coord, ESS::HICANNSide::S_RIGHT, ESS::HICANNBlock::BL_DOWN); 
    anncore_behav_i->configCurrentStimulus(stim0, ESS::HICANNSide::S_LEFT, ESS::HICANNBlock::BL_UP); 
    anncore_behav_i->configCurrentStimulus(stim1, ESS::HICANNSide::S_LEFT, ESS::HICANNBlock::BL_DOWN); 
    anncore_behav_i->configCurrentStimulus(stim2, ESS::HICANNSide::S_RIGHT, ESS::HICANNBlock::BL_UP); 
    anncore_behav_i->configCurrentStimulus(stim3, ESS::HICANNSide::S_RIGHT, ESS::HICANNBlock::BL_DOWN); 
	
    ///////////////////////////////////////////////////
	//<	Configuration of hw_neurons
	///////////////////////////////////////////////////

	// Get Dendrites2Neuron
	std::map<unsigned int, unsigned int> dendrites2neurons =
		hal_access->getDendrites(x_coord, y_coord);

	const std::map< unsigned int, unsigned int > firing_denmems_and_addresses = hal_access->getFiringDenmemsAndSPL1Addresses(x_coord, y_coord);

	// Configure Connection of Dendrites to hw_neurons and get Nr of used hw_neurons
	unsigned int hw_neuron_count = anncore_behav_i->initializeNeurons(firing_denmems_and_addresses,dendrites2neurons);

	// Check if nr of biological neurons is equal to nr of hw_neurons
	if(nrns_on_hic != hw_neuron_count)
    {
        LOG4CXX_ERROR(logger,name() << "Number of Neurons to build: " << nrns_on_hic << " but anncore initialized " << hw_neuron_count << " neurons!" );
        throw std::runtime_error("hicann_behav_V2: Number of Neurons to build !=  Number of neurons initialized in anncore");
    }
    else
    {
        LOG4CXX_INFO(logger, "Nr. of used hw_neurons on this anncore: " << hw_neuron_count );
    }

	// For every bio-neuron: get Neuron parameters from GraphModell and built the neuron_param struct.
	// Then configure hw_neuron with parameters, 6-bit neuron2wta- addr and ID of bio_neuron
	std::map< unsigned int, unsigned int >::const_iterator it_fdna =  firing_denmems_and_addresses.begin();
	std::map< unsigned int, unsigned int >::const_iterator it_fdna_end =  firing_denmems_and_addresses.end();

	for(;it_fdna != it_fdna_end; ++it_fdna) {
		// we need the id of the "configured" hw_neuron
		// biological params are mapped to the first dendrite belonging to a logical Neuron on hicann.
		// for example: if there are 64 logical neurons mapped on a hicann, i.e. 8 denmems building one neuron,
		// 		params of logical neuron 0 are stored in dendrite 0, 1->4, 2->8 etc.
		// 		(just upper denmem row have params)
		unsigned int dendrite =  it_fdna->first;
		unsigned int logical_neuron = dendrites2neurons.at(dendrite);
        LOG4CXX_DEBUG(logger, "Neuron parameters for logical Neuron " << logical_neuron << " on hicann " << hicannid << ", firing dendrite " << dendrite);

		//Look for Voltage Recording
        std::string rec_file = "";
        bool rec = hal_access->getVoltageRecording(x_coord, y_coord, dendrite);
        if(rec) {
            rec_file = hal_access->getVoltageFile(x_coord, y_coord, dendrite);
        }
		LOG4CXX_DEBUG(logger, "Neuron Parameter: record_membrane: \t" << rec );
		LOG4CXX_DEBUG(logger, "Neuron Parameter: record_membrane_filename: \t" << rec_file );

		// First configure params of all connected denmems
		std::set< unsigned int > connected_denmems;
		anncore_behav_i->getConnectedDenmems(connected_denmems, dendrites2neurons, logical_neuron, dendrite);
		for (auto connected : connected_denmems) {
			//if (connected != dendrite ) {
			ESS::BioParameter neuron_parameter = hal_access->getNeuronParametersSingleDenmem(x_coord, y_coord, connected);

			LOG4CXX_DEBUG(logger, "NeuronParameters for dendrite " << dendrite << " on hicann " << hicannid);
			//Logging all params
			LOG4CXX_DEBUG(logger, "Parameter: g_l:        " << neuron_parameter.g_l         << " nS" );
			LOG4CXX_DEBUG(logger, "Parameter: E_l:        " << neuron_parameter.v_rest      << " mV" );
			LOG4CXX_DEBUG(logger, "Parameter: a:          " << neuron_parameter.a           << " nS" );
			LOG4CXX_DEBUG(logger, "Parameter: tau_w:      " << neuron_parameter.tau_w       << " ms" );
			LOG4CXX_DEBUG(logger, "Parameter: b:          " << neuron_parameter.b           << " nA" );
			LOG4CXX_DEBUG(logger, "Parameter: V_exp:      " << neuron_parameter.v_thresh    << " mV" );
			LOG4CXX_DEBUG(logger, "Parameter: delta_t:    " << neuron_parameter.delta_T     << " mV" );
			LOG4CXX_DEBUG(logger, "Parameter: E_syn_i:    " << neuron_parameter.e_rev_I     << " mV" );
			LOG4CXX_DEBUG(logger, "Parameter: E_syn_e:    " << neuron_parameter.e_rev_E     << " mV" );
			LOG4CXX_DEBUG(logger, "Parameter: tau_syn_I:  " << neuron_parameter.tau_syn_I   << " ms" );
			LOG4CXX_DEBUG(logger, "Parameter: tau_syn_E:  " << neuron_parameter.tau_syn_E   << " ms" );
			LOG4CXX_DEBUG(logger, "Parameter: tau_refrac: " << neuron_parameter.tau_refrac  << " ms" );
			LOG4CXX_DEBUG(logger, "Parameter: v_spike:    " << neuron_parameter.v_spike     << " mV" );
			LOG4CXX_DEBUG(logger, "Parameter: V_reset:    " << neuron_parameter.v_reset     << " mV" );
			LOG4CXX_DEBUG(logger, "Parameter: Cap:        " << neuron_parameter.cm          << " nF");

			anncore_behav_i->configSingleDenmem(connected, neuron_parameter);
		}
		// Then configure and initialize compound neuron
		anncore_behav_i->configCompoundNeuron(dendrite, it_fdna->second, rec, rec_file, hw_timestep);
	}

	// current input
	for (size_t nd = 0; nd < 2*SYN_PER_ROW; ++nd) {
        if ( hal_access->getCurrentInput(x_coord, y_coord, nd) ) {
            anncore_behav_i->setCurrentInput(nd);
			LOG4CXX_DEBUG(logger, "Current_input enabled for denmem: \t" << nd);
		}
	}

    }
	else{
		LOG4CXX_INFO(logger, "No Neurons mapped on Hicann " << hicannid );
	}
	/// print Config of Anncore!
	std::stringstream file_path;
	file_path << sim_folder << "/debug_cfg_anncore_behav";
	anncore_behav_i->print_cfg(file_path.str());
}


void hicann_behav_V2::config_dncif()
{
	unsigned int i;
	std::vector<ESS::DNCIfDirections> dncif_directions;
	std::vector<bool> dncif_enables;

	hal_access->getDNCIFConfig(x_coord, y_coord, dncif_directions, dncif_enables);

	std::vector< enum dnc_if::direction > l1direction(DNCL1BUSINCOUNT, dnc_if::TO_HICANN);
	if(dncif_directions.size()!=DNCL1BUSINCOUNT){
		LOG4CXX_ERROR(logger, "hicann_behav_V2::config_dncif(): wrong number of horizontal buses connected to dnc_if!" );
        throw std::runtime_error("hicann_behav_V2::config_dncif(): wrong number of horizontal buses connected to dnc_if!" );
	}
	for(i=0;i<dncif_directions.size();i++) {
		if(dncif_directions[i] == ESS::DNCIfDirections::TO_HICANN)
			l1direction[i]=dnc_if::TO_HICANN;
		else
			l1direction[i]=dnc_if::TO_DNC;
	}


	// Config dnc_if
	dnc_if_i->set_directions(l1direction);
	dnc_if_i->set_enables(dncif_enables);

    LOG4CXX_DEBUG(logger, "Config of dnc_if channels to horizontal l1 bus: 1 means from DNC to L1" );
	for(unsigned int i=0;i<dncif_directions.size();++i){
        LOG4CXX_DEBUG(logger, dncif_directions[i] );
	}
}

void hicann_behav_V2::config_l1net()
{
    // start repeater config
    std::vector<unsigned char> rep_config;

	rep_config = hal_access->getRepeaterConfig(x_coord, y_coord, ESS::REP_L);
	for(size_t rep_id = 0; rep_id < rep_config.size();++rep_id){
		l1_behav_i->set_repeater_config( ESS::REP_L, rep_id, rep_config[rep_id] );
	}

	rep_config = hal_access->getRepeaterConfig(x_coord, y_coord, ESS::REP_R);
	for(size_t rep_id = 0; rep_id < rep_config.size();++rep_id){
		l1_behav_i->set_repeater_config( ESS::REP_R, rep_id, rep_config[rep_id] );
	}

	rep_config = hal_access->getRepeaterConfig(x_coord, y_coord, ESS::REP_UL);
	for(size_t rep_id = 0; rep_id < rep_config.size();++rep_id){
		l1_behav_i->set_repeater_config( ESS::REP_UL, rep_id, rep_config[rep_id] );
	}

	rep_config = hal_access->getRepeaterConfig(x_coord, y_coord, ESS::REP_UR);
	for(size_t rep_id = 0; rep_id < rep_config.size();++rep_id){
		l1_behav_i->set_repeater_config( ESS::REP_UR, rep_id, rep_config[rep_id] );
	}

	rep_config = hal_access->getRepeaterConfig(x_coord, y_coord, ESS::REP_DL);
	for(size_t rep_id = 0; rep_id < rep_config.size();++rep_id){
		l1_behav_i->set_repeater_config( ESS::REP_DL, rep_id, rep_config[rep_id] );
	}

	rep_config = hal_access->getRepeaterConfig(x_coord, y_coord, ESS::REP_DR);
	for(size_t rep_id = 0; rep_id < rep_config.size();++rep_id){
		l1_behav_i->set_repeater_config( ESS::REP_DR, rep_id, rep_config[rep_id] );
	}
	// end of repeater config

	// switch config

	// enum switch_loc {CBL=0, CBR=1, SYNTL=2, SYNTR=3 ,SYNBL=4, SYNBR=5};
	std::vector< std::vector<bool> > switch_config;
	switch_config = hal_access->getCrossbarSwitchConfig(x_coord, y_coord, ESS::HICANNSide::S_LEFT);
	for(size_t row_id = 0; row_id < switch_config.size(); ++row_id) {
		l1_behav_i->set_switch_row_config(CBL, row_id, switch_config[row_id]);
	}

	switch_config = hal_access->getCrossbarSwitchConfig(x_coord, y_coord, ESS::HICANNSide::S_RIGHT);
	for(size_t row_id = 0; row_id < switch_config.size(); ++row_id) {
		l1_behav_i->set_switch_row_config(CBR, row_id, switch_config[row_id]);
	}

	switch_config = hal_access->getSyndriverSwitchConfig(x_coord, y_coord, ESS::HICANNBlock::BL_UP, ESS::HICANNSide::S_LEFT);
	for(size_t row_id = 0; row_id < switch_config.size(); ++row_id) {
		l1_behav_i->set_switch_row_config(SYNTL, row_id, switch_config[row_id]);
	}
	switch_config = hal_access->getSyndriverSwitchConfig(x_coord, y_coord, ESS::HICANNBlock::BL_UP, ESS::HICANNSide::S_RIGHT);
	for(size_t row_id = 0; row_id < switch_config.size(); ++row_id) {
		l1_behav_i->set_switch_row_config(SYNTR, row_id, switch_config[row_id]);
	}

	switch_config = hal_access->getSyndriverSwitchConfig(x_coord, y_coord, ESS::HICANNBlock::BL_DOWN, ESS::HICANNSide::S_LEFT);
	for(size_t row_id = 0; row_id < switch_config.size(); ++row_id) {
		l1_behav_i->set_switch_row_config(SYNBL, row_id, switch_config[row_id]);
	}
	switch_config = hal_access->getSyndriverSwitchConfig(x_coord, y_coord, ESS::HICANNBlock::BL_DOWN, ESS::HICANNSide::S_RIGHT);
	for(size_t row_id = 0; row_id < switch_config.size(); ++row_id) {
		l1_behav_i->set_switch_row_config(SYNBR, row_id, switch_config[row_id]);
	}
	// end of switch config
    
    std::stringstream file_path_1;
	file_path_1 << sim_folder << "/debug_cfg_layer1net";
	l1_behav_i->print_cfg(file_path_1.str());
}

void hicann_behav_V2::config_spl1_merger()
{
	std::vector< std::vector<bool> > neuron_control_config = hal_access->getSPL1OutputMergerConfig(x_coord, y_coord);
    
    LOG4CXX_DEBUG(logger, "SPL1 Merger config of Hicann " << x_coord << ", " << y_coord );
	
    for(size_t row_id = 0; row_id < neuron_control_config.size(); ++row_id) {
		spl1_merger_i->set_config(row_id, neuron_control_config[row_id]);
        LOG4CXX_DEBUG(logger, "Row " << row_id << ":\t" );
		for(size_t n = 0; n < neuron_control_config[row_id].size(); ++n) {
            LOG4CXX_DEBUG(logger, neuron_control_config[row_id][n] );
        }
	}

	std::vector<ESS::DNCIfDirections> dncif_directions;
	std::vector<bool> dncif_enables;

	hal_access->getDNCIFConfig(x_coord, y_coord, dncif_directions, dncif_enables);

	spl1_merger_i->set_dnc_if_directions(dncif_directions);

	// we re-run the reset_n config, to configure with the correct seed:
	if( neuron_control_config.size() >= 5 )
		spl1_merger_i->set_config(4, neuron_control_config[4]);

    std::stringstream file_path;
	file_path << sim_folder << "/debug_cfg_spl1merger";
    spl1_merger_i->print_cfg(file_path.str());
}
