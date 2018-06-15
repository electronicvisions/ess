//_____________________________________________
// Company      :    TU-Dresden
// Author       :    Stefan Scholze
// Refinement    :    Matthias Ehrlich
// E-Mail       :    *@iee.et.tu-dresden.de
//
// Date         :   Thu Jan 18 12:55:59 2007
// Last Change  :   mehrlich 03/20/2008
// Module Name  :   pcb
// Filename     :   pcb.h
// Project Name    :   p_facets/s_systemsim
// Description    :   Description of the printed circuit board
//
//_____________________________________________

#ifndef _PCB_H_
#define _PCB_H_

// always needed
#include <sys/time.h>
#include "systemc.h"

#include "boost/tuple/tuple.hpp"

// defines and helpers
#include "sim_def.h"
#include "mutexfile.h"
#include "HAL2ESSContainer.h"


// forward declaration
class HALaccess;
class l2_dnc;
class l2_fpga;
class wafer;

using namespace ess;

/// The printed circuit board class - pcb instantiates all its components.
/// That is : \n
/// - Level2 Digital Network Cores - DNCs (l2_dnc) \n
/// - Level2 FPGAs (l2_fpga) \n
/// - a wafer \n
/// and connects their interfaces according to the connection shemes: \n
/// - dnc_con[HICANN_Y][HICANN_X] \n
/// - hicann_conx[DNC_COUNT] \n
/// - hicann_cony[DNC_COUNT] \n
class pcb : public sc_module
{
public:
    // instantiations
    int wafer_nr;///<identification number

private:
	std::vector< std::shared_ptr<l2_dnc> > l2_dnc_i; 	///<all DNCs of a pcb system, simple implementation
	std::vector< std::shared_ptr<l2_fpga> > l2_fpga_i; 	///<all FPGAs of a pcb system, simple implementation
	
public:
	
	std::unique_ptr<wafer> wafer_i;    ///<the wafer with the neuronal elements
	const unsigned int hicann_x_count; //!< nr of hicanns in X-direction
	const unsigned int hicann_y_count; //!< nr of hicanns in y-direction
	const unsigned int dnc_count; //!< nr of DNCs on this PCB
	const unsigned int fpga_count; //!< nr of FPGAs on this PCB
	const std::vector< std::vector<int> >& map_dncs_on_fpga; //!< 2d-array containing the dnc ids for every FPGA(4 per FPGA), if none is connected at one FPGA-DNC-Channel, id = -1.

    mutexfile &spike_rx_file;    ///<filehandle to record received events on the wafer
    mutexfile &spike_tx_file;    ///<filehandle to record transmitted events on the wafer
	std::string sim_folder;      ///<Folder where all temporary files during simulation and debug goes to. For parallel simulation.

    /**
    Constructor of the PCB system generates all required components and creates the layer2 communication connections.
    It also provide the handle to different files and configuration to the instantiated components.
    */
    pcb (
		sc_module_name pcb_i,           //!< systemc instance name
		int id,                         ///<identification number
		unsigned int hicann_x,      //!< nr of hicanns in X-direction
		unsigned int hicann_y,      //!< nr of hicanns in y-direction
		unsigned int dnc_count_,     //!< nr of DNCs on this PCB
		unsigned int fpga_count_,    //!< nr of FPGAs on this PCB
		std::vector< std::vector<int> >& map_dncs_on_fpga, //!< 2d-array containing the dnc ids for every FPGA, if none is connected at one FPGA-DNC-Channel, id = -1.
		std::vector< std::vector< boost::tuples::tuple<bool, unsigned int, int, int > > >& hicann_config, //!< 2d-array of hicanns: available, configId, parent_dnc, dnc_hicann_channel
		mutexfile& spike_receive_file,  ///<filehandle for received events
		mutexfile& spike_transm_file,   ///<filehandle for transmitted events
		std::string temp_folder,        ///<folder for debug and temporary simulation files
		HALaccess *hala,				///<pointer to HALaccess
		const std::vector<ESS::fpga_config>& fpga_config,
		const std::array<ESS::dnc_config,48>& dnc_config
	);

	/** destructor.*/
	virtual ~pcb();

	//getter for pointer to dnc/fpga_simple
	l2_dnc* get_dnc(size_t dnc_id);
	l2_fpga* get_fpga(size_t fpga_id);
	wafer*	get_wafer();
};

#endif  // _PCB_H_
