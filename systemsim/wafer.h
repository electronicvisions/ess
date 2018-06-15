//_____________________________________________
// Company      :	TU-Dresden
/// Author       :	Stefan Scholze
// Refinement	:	Matthias Ehrlich
// E-Mail   	:	*@iee.et.tu-dresden.de
//
// Date         :   Wed Jan 24 08:56:09 2007
// Last Change  :   mehrlich 03/20/2008
// Module Name  :   wafer
// Filename     :   wafer.h
// Project Name	:   p_facets/s_systemsim
// Description	:   The wafer assembly
//
//_____________________________________________

#ifndef _WAFER_H_
#define _WAFER_H_

// always needed
#include <sys/time.h>
#include "systemc.h"

// defines and helpers
#include "sim_def.h"
#include "mutexfile.h"

// forward declarations
class hicann_behav_V2;
class HALaccess;

using namespace ess;

/// The wafer class.
/// - instantiates the HICANNs and \n
/// - provides objects for mimicing HICANN beahviour
class wafer :
	public sc_module
{
	public:
	const unsigned int hicann_x_count; //!< nr of hicanns in X-direction
	const unsigned int hicann_y_count; //!< nr of hicanns in y-direction
	std::vector< std::vector< std::unique_ptr<hicann_behav_V2> > > hicann_i;
	const std::vector< std::vector<int> > hicann_enable; //!< 2d-array containing information containg the configId of Hicann. -1 if hicann is disabled
	mutexfile& spike_rx_file; ///< filehandle for received events
	mutexfile& spike_tx_file; ///< filehandle for transmitted events
	std::string sim_folder;   ///< folder for debug and temporary simulation files
	HALaccess *mHala;

	/** constructor.*/
	wafer(
		sc_module_name wafer,        //!< systemc instance name
		unsigned int hicann_x,      //!< nr of hicanns in X-direction
		unsigned int hicann_y,      //!< nr of hicanns in y-direction
		std::vector< std::vector<int> >& enabled_hicanns, //!< 2d-array containing information containg the configId of Hicann. -1 if hicann is disabled
		std::vector< std::vector<int> >& hicann_on_dnc, 
		mutexfile& spike_rcx_file,  ///<filehandle for received events
		mutexfile& spike_tcx_file,  ///<filehandle for transmitted events
		std::string temp_folder,    ///<folder for debug and temporary simulation files
		HALaccess *hala 			///<pointer to HALaccess	
	);

	/** desctructor.*/
	virtual ~wafer();
};

#endif  // _WAFER_H_

