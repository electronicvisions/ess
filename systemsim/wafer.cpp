//_____________________________________________
// Company      :	TU-Dresden	      	
/// Author       :	Stefan Scholze
// Refinement	:	Matthias Ehrlich
// E-Mail   	:	*@iee.et.tu-dresden.de					
//								
// Date         :   Wed Jan 24 08:56:09 2007				
// Last Change  :   mehrlich 03/20/2008			
// Module Name  :   wafer					
// Filename     :   wafer.cpp				
// Project Name	:   p_facets/s_systemsim				
// Description	:  		
//								
//_____________________________________________

#include "hicann_behav_V2.h"
#include "wafer.h"
#include "l1_behav_V2.h"
#include "anncore_behav.h"
#include <log4cxx/logger.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("ESS");

wafer::~wafer()
{}


wafer::wafer(
		sc_module_name wafer,        //!< systemc instance name
		unsigned int hicann_x,      //!< nr of hicanns in X-direction
		unsigned int hicann_y,      //!< nr of hicanns in y-direction
		std::vector< std::vector<int> >& enabled_hicanns, //!< 2d-array containing information whether hicann are enabled or not.
		std::vector< std::vector<int> >& hicann_on_dnc,
		mutexfile& spike_rcx_file,   ///<filehandle for received events
		mutexfile& spike_tcx_file,   ///<filehandle for transmitted events
		std::string temp_folder,      ///<folder for debug and temporary simulation files
		HALaccess *hala
	)
	: sc_module(wafer)
	, hicann_x_count(hicann_x)
	, hicann_y_count(hicann_y)
	, hicann_enable(enabled_hicanns)
	, spike_rx_file(spike_rcx_file)
	, spike_tx_file(spike_tcx_file)
	, sim_folder(temp_folder)
	, mHala(hala)
{
	char buffer[1000];

    LOG4CXX_INFO(logger, "Start building Wafer (i.e. Array of HICANNs)" );
	assert(hicann_enable.size() == hicann_y_count);
	hicann_i.resize(hicann_y_count);
	for(size_t ny=0;ny< hicann_y_count; ++ny)
	{
        assert(hicann_enable[ny].size() == hicann_x_count);
		hicann_i[ny].resize(hicann_x_count);
		for(size_t nx=0; nx<hicann_x_count; ++nx)
		{
            if (hicann_enable[ny][nx] >= 0) {
                snprintf(buffer,sizeof(buffer),"hicann_behav_V2_i_X%i_Y%i",(int)nx,(int)ny);
				unsigned int hicann_id = hicann_enable[ny][nx];
                LOG4CXX_INFO(logger, "Creating HICANN with coordinates x = " << nx << ", y = " << ny << ", configId = " << hicann_id );
				hicann_i[ny][nx] = std::unique_ptr<hicann_behav_V2>(new hicann_behav_V2(buffer, hicann_id,spike_rx_file,spike_tx_file, sim_folder, hala, hicann_on_dnc[ny][nx]));
				hicann_i[ny][nx]->config_hicann(hicann_id);
			}
		}
	}
	/////////////////////////////////
	//// Connect Hicanns on wafer  //
	/////////////////////////////////

	// 1) connect to horizontal neighbour 
	for(size_t ny=0;ny< hicann_y_count; ++ny)
	{
		for(size_t nx=0; nx<hicann_x_count-1; ++nx)
		{
			// if both are enabled, connect!
			if ( (hicann_i[ny][nx]!= NULL) && (hicann_i[ny][nx+1]!=NULL) ) {
				hicann_i[ny][nx]->l1_behav_i->l1_nb_right = (hicann_i[ny][nx+1]->l1_behav_i.get());	// connect to layer1net of right neighbour
				hicann_i[ny][nx+1]->l1_behav_i->l1_nb_left = (hicann_i[ny][nx]->l1_behav_i.get());	// connect to layer1net of left neighbour
				hicann_i[ny][nx]->l1_behav_i->anncore_if_right = (hicann_i[ny][nx+1]->anncore_behav_i);	// connect to anncore of right neighbour
				hicann_i[ny][nx+1]->l1_behav_i->anncore_if_left = (hicann_i[ny][nx]->anncore_behav_i);	// connect to anncore of right neighbour
			}
		}
	}

	// 2) connect to vertical neighbour
	for(size_t ny=0;ny< hicann_y_count-1; ++ny)
	{
		for(size_t nx=0; nx<hicann_x_count; ++nx)
		{
			if ( (hicann_i[ny][nx]!= NULL) && (hicann_i[ny+1][nx]!=NULL) ) {
				hicann_i[ny][nx]->l1_behav_i->l1_nb_bottom = (hicann_i[ny+1][nx]->l1_behav_i.get());	// connect to layer1net of bottom neighbour
				hicann_i[ny+1][nx]->l1_behav_i->l1_nb_top = (hicann_i[ny][nx]->l1_behav_i.get());	// connect to layer1net of top neighbour
			}
		}
	}

    LOG4CXX_INFO(logger, "Completed building Wafer" );

}
