#ifndef _STAGE2_VIRTUAL_HARDWARE_H_
#define _STAGE2_VIRTUAL_HARDWARE_H_

// always needed
#include <ctime>
#include <systemc.h>
#include <iostream>
#include <memory>

// defines and helpers
#include "mutexfile.h"

// functional units
#include "pcb.h"

// forward declaration
class HALaccess;

namespace ess
{
    class S2C_SystemSim;
    class FPGAControl;
    class DNCControl;
}

using namespace ess;

struct ResetSystemC
{
	~ResetSystemC();
};

/// This class provides the FACETS stage 2 system setup.
/// As testbench that instantiates as much printed circuit 
/// boards as wafers are defined
class Stage2VirtualHardware : public ResetSystemC, sc_module
{

public:

    // File stuff
	const unsigned int wafer_count;    //!< number of wafers in the system
	std::string sim_folder;      ///<Folder where all temporary files during simulation and debug goes to. For parallel simulation.
    mutexfile spike_receive_file; ///<filehandle to record received events on the wafer
    mutexfile spike_transm_file;  ///<filehandle to record transmitted events on the wafer

	int _duration_in_NS;  //!< duration of the systemc simulation in nano-seconds, needed for the progress_bar.

	std::vector< std::unique_ptr<pcb> > pcb_i; //!< array containing all pcb instances

	// the following is needed as we don't use the CTOR constructor
	// and the module has processes.
	SC_HAS_PROCESS(Stage2VirtualHardware);

	/** constructs the Stage2 Virtual Hardware, i.e. the ESS.*/
    Stage2VirtualHardware(
		sc_module_name name,           //!< systemc module name
		unsigned int wafer_count,      //!< number of wafers(=PCBs) in the system
		std::string simulation_folder  //!< folder for debug and temporary simulation files
	);

	/** destructor.*/
	virtual ~Stage2VirtualHardware();

	/** sets the simulation duration ins nano seconds.
	 * This information is needed by the method progress_bar()
	 */
	void set_sim_duration(long duration_in_NS);

	void run();

	/** provides a progress bar for the execution of the systemc simulation.*/
	void progress_bar();

	/** initializes and instantiates the PCB with ID id.
	 * has to be called before end of elaboration.
	 */
	void initialize_pcb(
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
		);

    //returns the filepath of the simfolder as string
    std::string get_filepath() const;

private:
	/** when all pcbs are built, this connects the fpgas before the end_of_elaboration phase.*/
	void connect_fpgas_from_different_pcbs();
	
private:
	bool res_set;	                           	 			//has sc_time_resolution been set?
	unsigned int _fpga_count;								//nr of fpgas
	unsigned int _dnc_count;								//nr of dncs
	std::vector< std::vector<int> > _map_dncs_on_fpga;		//mapping of dncs to fpgas
};

#endif //_STAGE2_VIRTUAL_HARDWARE_H_
