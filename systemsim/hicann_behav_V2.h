//_____________________________________________
// Company      :	TU-Dresden
// Author       :	Bernhard Vogginger
// Refinement	:	Matthias Ehrlich
// E-Mail   	:	Bernhard.Vogginer@tu-dresden.de
//_____________________________________________

#ifndef _HICANN_BEHAV_V2_H_
#define _HICANN_BEHAV_V2_H_

// always needed
#include "systemc.h"
#include <string>
#include "mutexfile.h"

using namespace ess; // for mutexfile

// forward declarations
class HALaccess;
class l1_behav_V2;
class anncore_behav;
class dnc_if;
class spl1_merger;

/// The High Input Count Analog Neural Network (hicann) class.
/// It instantiates: \n
/// 	- Level1 static network (layer1net) \n
/// 	- Level1/2 DNC interfaces (dnc_if) \n
/// 	- Analogue Neural Network core (anncore) \n
///     - SPL1 Merger (also called neuron control) \n
class hicann_behav_V2 :
	public sc_module
{
public:
	/** constructor for class hicann_behav_V2.*/
	hicann_behav_V2(
			sc_module_name hicann_i,   	//!< instance name
			short hicannid,            	//!< identification id
			mutexfile& spike_rcx_file, 	//!< FILE handle for received events
			mutexfile& spike_tcx_file, 	//!< FILE handle for transmitted events
			std::string temp_folder,   	//!< Folder for temporary and debug files during simulation
			HALaccess* hala,			//!< pointer to HALaccess
			short hicann_on_dnc
			);

	/// function to config Hicann and Submodules
	void config_hicann(unsigned int id);

	/** destructor*/
	virtual ~hicann_behav_V2();

	////////////////
	// Submodules //
	////////////////

	std::shared_ptr<dnc_if> dnc_if_i; ///< instance of a dnc_interface 

	std::shared_ptr<anncore_behav> anncore_behav_i; ///< inserted this anncore_behav instance for debugging!

	std::unique_ptr<l1_behav_V2> l1_behav_i; ///< instance of behavioral layer 1 network

	std::unique_ptr<spl1_merger> spl1_merger_i;  //!< instance of SPL1 merger tree

	unsigned int get_id() const {return hicannid;}
	unsigned int get_x() const {return x_coord;}
	unsigned int get_y() const {return y_coord;}
private:

	///////////////////
	// class members //
	///////////////////

	// need hicann id & nrnumbers
	short hicannid; ///< identification number
	unsigned int x_coord;	///< x-coordinate of hicann on wafer(needed for GMAccess)
	unsigned int y_coord;	///< y-coordinate of hicann on wafer(needed for GMAccess)
	std::string sim_folder; ///< Folder for temporary and debug files during simulation
	HALaccess * hal_access; ///< instance of HALaccess for configuration

	/////////////////////////////////////////////
	// Methods for configuration of submodules //
	/////////////////////////////////////////////

	/// function to configure anncore
	void config_anncore();

	/// function to configure layer 1 routing module 
	void config_l1net();

	/// function to configure dnc_if
	void config_dncif();

	/// configures the spl1-merger tree via access to the graphmodel
	void config_spl1_merger();
};

#endif // _HICANN_BEHAV_V2_H_
