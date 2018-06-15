#include "stage2_virtual_hardware.h"
#include "common.h"
#include "boost/filesystem.hpp"

#include <string>
#include <boost/progress.hpp>
#include <sstream>
#include <stdexcept>

using namespace boost::tuples;

#include "IFSC.h"
#include <log4cxx/logger.h>
#include "lost_event_logger.h"

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("ESS");

ResetSystemC::~ResetSystemC()
{
	sc_get_curr_simcontext()->reset();
}

Stage2VirtualHardware::Stage2VirtualHardware(
		sc_module_name name,           //!< systemc module name
		unsigned int wafer_count_,      //!< number of wafers(=PCBs) in the system
		std::string simulation_folder_  //!< folder for debug and temporary simulation files
		)
	: sc_module(name)
	, wafer_count(wafer_count_)
	, sim_folder(simulation_folder_)
	, _duration_in_NS(0)
	, res_set(false)
	, _fpga_count(0)
	, _dnc_count(0)
	, _map_dncs_on_fpga()
{
	if (wafer_count != 1) {
        LOG4CXX_ERROR(logger, " You are trying to build up a system with more than 1 wafer, which is currently not supported!" );
        throw std::runtime_error(" stage2virtualhw: You are trying to build up a system with more than 1 wafer, which is currently not supported!" );
	}

	// Initiation of Neuron Model
	IFSC_Init(0.25);


	if (sim_folder == "")
		sim_folder = ".";

	SC_THREAD(progress_bar);
}

Stage2VirtualHardware::~Stage2VirtualHardware()
{
	pcb_i.clear();
	LostEventLogger::reset();
}

void Stage2VirtualHardware::initialize_pcb(
		unsigned int id,             //!< the id of the pcb to be built.
		unsigned int hicann_x,      //!< nr of hicanns in X-direction
		unsigned int hicann_y,      //!< nr of hicanns in y-direction
		unsigned int dnc_count,     //!< nr of DNCs on this PCB
		unsigned int fpga_count,    //!< nr of FPGAs on this PCB
		std::vector< std::vector<int> >& map_dncs_on_fgpa, //!< 2d-array containing the dnc ids for every FPGA, if none is connected at one FPGA-DNC-Channel, id = -1.
		std::vector< std::vector< boost::tuples::tuple<bool, unsigned int, int, int > > >& hicann_config, //!< 2d-array of hicanns: available, configId, parent_dnc, dnc_hicann_channel
		HALaccess *hala,
		const std::vector<ESS::fpga_config> & FPGAConfig,
		const std::array<ESS::dnc_config,48> & DNCConfig
		)
{

	pcb_i.resize(wafer_count);


	if (id >= wafer_count){
            LOG4CXX_ERROR(logger, name() << " You are trying to initialize a pcb with ID " << id << " which you had not specified before. (Wafer count = " << wafer_count << ")" );
            throw std::runtime_error(" stage2virtualhw: You are trying to initialize a pcb with an ID  that was not specified before.");
	}

	if(res_set == false)	//set the systemc time resolution if it wasn t set before (can only be set once, before initializing any sc_module
	{
		//sc_set_time_resolution(10,SC_PS);	//10 = magic_number from Stage2ControlSystemSim
		res_set = true;
	}

	//set member for fpga_count etc.
	assert(FPGAConfig.size() == fpga_count);
	_fpga_count = fpga_count;
	_map_dncs_on_fpga = map_dncs_on_fgpa;
	_dnc_count = dnc_count;

	// generate directory
    LOG4CXX_INFO(logger, name() << "Checking for directory / creating..." << sim_folder );
	boost::filesystem::create_directory( sim_folder.c_str() ); // throws if directory doesn't exist && couldn't be created
	// generate subdirectory for denmem traces
	std::string denmem_folder = sim_folder + "/denmem_traces";
    LOG4CXX_INFO(logger, name() << "Checking for directory / creating..." << denmem_folder );
	boost::filesystem::create_directory( denmem_folder.c_str() ); // throws if directory doesn't exist && couldn't be created
	
	char buffer[1024];

	// TODO: as soon as there are multiple PCBs, make sure that this file handling works!
	snprintf(buffer,sizeof(buffer),"%s/rx_spikes_%i.txt",sim_folder.c_str(),id);
	spike_receive_file.open(buffer,"w");
	snprintf(buffer,sizeof(buffer),"%s/tx_spikes_%i.txt",sim_folder.c_str(),id);
	spike_transm_file.open(buffer,"w");

	// Syntax of spike_files of anncore
	FILE *fpt = spike_receive_file.aquire_fp();
	// FIXME This segfaults if directory cannot be opened - should give an explicit error message
    fprintf(fpt, "// Syntax: \" 8i 3i 3i 2i \",(uint)sc_simulation_time(),anncoreid,(uint)syndr, (unsigned char)addr)\n");
	fprintf(fpt, "// 1.) Simulation Time(usually in ps) \n// 2.) ID of receiving ANNCORE\n// 3.) ID of receiving Synapse Driver 0..127 (left) 128..255(right)\n// 4.) Pulse event addr(6-bit) 0..63\n");
	fflush(fpt);
	spike_receive_file.release_fp(fpt);
	FILE *fpt2 = spike_transm_file.aquire_fp();
	fprintf(fpt2, "// Syntax: \" 8i 3i 4i 2i 5i\",(uint)sc_simulation_time(),anncoreid,(uint)logical_neuron, (unsigned char)addr)\n");
	fprintf(fpt2,"// 1.) Simulation time(usually in ps) \n// 2.) ID of sending ANNCORE\n// 3.) ID of logical Neuron(hardware denmem) 0..511\n// 4.) Pulse event addr(6-bit) 0..63\n // 5.) ID of biological Neuron\n");
	fflush(fpt2);
	spike_transm_file.release_fp(fpt2);

	snprintf(buffer,sizeof(buffer),"pcb_i%i",id);
	pcb_i.at(id).reset(new pcb(
		buffer,
		id,
		hicann_x,
		hicann_y,
		dnc_count,
		fpga_count,
		map_dncs_on_fgpa,
		hicann_config,
		spike_receive_file,
		spike_transm_file,
		sim_folder,
		hala,
		FPGAConfig,
		DNCConfig
	));
}
    

void Stage2VirtualHardware::connect_fpgas_from_different_pcbs()
{
	if (wafer_count > 1) {
		// Connect FPGAs of different wafers.
		// Note: The following assumes that all pcbs have the same number of fpgas.
		for (unsigned int k=0;k<wafer_count;k++)
		{
			unsigned int fpgas_per_pcb = pcb_i[k]->fpga_count;
			for (unsigned int i=0;i<fpgas_per_pcb;i++)
			{
				int fb_corr = 0;
				for (unsigned int j=0;j<wafer_count;j++)
				if (k != j)
				{
					printf("connection of pcb_i[%i]->l2_fpga_i[%i][%i]@pcb_i[%i]->l2_fpga_i[%i][%i] \n",k,i,j+fb_corr,j+fb_corr,i,j+fb_corr);
					//pcb_i[k]->get_fpga(i)->fpga_channel_i[j+fb_corr]->l2_fpga_if(*(pcb_i[j+fb_corr]->get_fpga(i)->fpga_channel_i[j+fb_corr]));    //needs to insert additional wafer
				}
				else
				{
					fb_corr = -1;
				}
			}
		}
	}
}

void
Stage2VirtualHardware::set_sim_duration(long duration_in_NS) {
	_duration_in_NS = duration_in_NS;
}


void
Stage2VirtualHardware::run() {
    //logging the systemc object tree
    std::ostringstream oss;
    print_systemc_object_tree(oss);
    std::string tree = oss.str(); 
    LOG4CXX_DEBUG(logger, tree );
	sc_report_handler::set_actions("/IEEE_Std_1666/deprecated", SC_DO_NOTHING);
	//running the simulation
    sc_start(_duration_in_NS,SC_NS);
	sc_stop();
}


void Stage2VirtualHardware::progress_bar()
{
	if (_duration_in_NS > 0) {
		size_t num_steps = 100;
		long wait_time_in_NS = (_duration_in_NS -1)/num_steps;
		boost::progress_display show_progress(num_steps);
		for(size_t i = 0; i < num_steps; ++i) {
			wait(wait_time_in_NS, SC_NS);
			++show_progress;
		}
	}
}

//returns the filepath of the simfolder as string
std::string Stage2VirtualHardware::get_filepath() const
{
    return sim_folder;
}
