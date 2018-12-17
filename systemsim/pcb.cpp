//_____________________________________________
// Company      :	TU-Dresden
// Author       :	Stefan Scholze
// Refinement	:	Matthias Ehrlich
// E-Mail   	:	*@iee.tu-dresden.de
//
// Date         :   Thu Jan 18 12:55:59 2007
// Last Change  :   mehrlich 03/20/2008
// Module Name  :   pcb
// Filename     :   pcb.cpp
// Project Name	:   p_facets/s_sytemsim
// Description	:   
//
//_____________________________________________

//include own header
#include "pcb.h"
//include functional units
#include "wafer.h"
#include "l2_dnc.h"
#include "l2_fpga.h"
#include "hicann_behav_V2.h"
#include "dnc_if.h"
#include <log4cxx/logger.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("ESS");

using namespace boost::tuples;

pcb::~pcb()
{}

pcb::pcb(
		sc_module_name /*pcb_i*/,       //!< systemc instance name
		int id,                         ///<identification number
		unsigned int hicann_x,      //!< nr of hicanns in X-direction
		unsigned int hicann_y,      //!< nr of hicanns in y-direction
		unsigned int dnc_count_,     //!< nr of DNCs on this PCB
		unsigned int fpga_count_,    //!< nr of FPGAs on this PCB
		std::vector< std::vector<int> >& map_dncs_on_fpga, //!< 2d-array containing the dnc ids for every FPGA, if none is connected at one FPGA-DNC-Channel, id = -1.
		std::vector< std::vector< boost::tuples::tuple<bool, unsigned int, int, int > > >& hicann_config, //!< 2d-array of hicanns: available, configId, parent_dnc, dnc_hicann_channel
		mutexfile& spike_receive_file,  ///<filehandle for received events
		mutexfile& spike_transm_file,   ///<filehandle for transmitted events
		std::string temp_folder,         ///<folder for debug and temporary simulation files
		HALaccess *hala,
		const std::vector<ESS::fpga_config>& fpga_config,
		const std::array<ESS::dnc_config,48>& dnc_config
		)
	: wafer_nr(id)                            ///<identification number
	, hicann_x_count(hicann_x)
	, hicann_y_count(hicann_y)
	, dnc_count(dnc_count_)
	, fpga_count(fpga_count_)
	, map_dncs_on_fpga(map_dncs_on_fpga)
	, spike_rx_file(spike_receive_file)        ///<filehandle for received events
	, spike_tx_file(spike_transm_file)        ///<filehandle for transmitted events
	, sim_folder(temp_folder)                 ///<folder for debug and temporary simulation files
{
	assert(fpga_config.size() == fpga_count);

	///////////////////////
	// Creation
	///////////////////////

	//
	//Crating the simple L2-Classes
	//
	//create the simple DNCs
    LOG4CXX_DEBUG(logger, name() << "::Creating " << dnc_count << " DNCs" );
	char buffer[1000];
	l2_dnc_i.resize(dnc_count);
	for (unsigned int i=0;i<dnc_count;i++){
		snprintf(buffer,sizeof(buffer),"l2_dnc_i%i",i);
		l2_dnc_i[i] = std::shared_ptr<l2_dnc>(new l2_dnc(buffer,id,i,dnc_config[i]));
	}
	//create the simple FPGAs
    LOG4CXX_DEBUG(logger, name() << "::Creating " <<fpga_count << " FPGAs" );
	l2_fpga_i.resize(fpga_count);
	for (unsigned int i=0;i<fpga_count;i++)
	{
		snprintf(buffer,sizeof(buffer),"l2_fpga_i%i",i);
		l2_fpga_i[i] = std::shared_ptr<l2_fpga>(new l2_fpga(buffer,i,wafer_nr, fpga_config[i]));
	}
	
    LOG4CXX_DEBUG(logger, name() << "::Creating WAFER" );

	std::vector< std::vector<int> > hicann_enable(hicann_y_count, std::vector<int>(hicann_x_count,-1)); // -1 means HICANN is disabled
	std::vector< std::vector<int> > hicann_on_dnc(hicann_y_count, std::vector<int>(hicann_x_count,-1));
	for(size_t ny=0;ny< hicann_y_count; ++ny)
	{
		for(size_t nx=0; nx<hicann_x_count; ++nx)
		{
			if (hicann_config.at(ny).at(nx).get<0>()) {
				hicann_enable.at(ny).at(nx) = hicann_config.at(ny).at(nx).get<1>();
				hicann_on_dnc.at(ny).at(nx) = hicann_config.at(ny).at(nx).get<3>();
			}
		}
	}

	wafer_i = std::unique_ptr<wafer>(new wafer( "wafer_i",
                                                hicann_x_count,
                                                hicann_y_count,
                                                hicann_enable, 
											    hicann_on_dnc,
											    spike_rx_file,
                                                spike_tx_file,
                                                sim_folder,
                                                hala));

	///////////////////////
	// Connection
	///////////////////////

    LOG4CXX_DEBUG(logger, name() << "::Start connecting instances " );

	//
	//Connecting the simple L2 Classes
	//
	// FPGA
    LOG4CXX_DEBUG(logger, name() <<  "::Connecting FPGAs and DNCs" );
	for (unsigned int fpga_id =0;fpga_id<map_dncs_on_fpga.size();++fpga_id)
	{
		for (unsigned int channel_id =0;channel_id<map_dncs_on_fpga[fpga_id].size();++channel_id)
		{
			int dnc_id = map_dncs_on_fpga[fpga_id][channel_id];
			if( dnc_id != -1)
			{
				// channel = dnc_on_fpga coordinate!
                LOG4CXX_DEBUG(logger, name() << "::Connecting FPGA " << fpga_id << " with DNC " << dnc_id  << " via channel " << channel_id);
				// DNC -> FPGA
				l2_dnc_i[dnc_id]->dnc_tx_fpga_i->l2_dncfpga_if(*(l2_fpga_i[fpga_id]->dnc_tx_fpga_i[channel_id]));
				// FPGA -> DNC
				l2_fpga_i[fpga_id]->dnc_tx_fpga_i[channel_id]->l2_dncfpga_if(*(l2_dnc_i[dnc_id]->dnc_tx_fpga_i));
			}
		}
	}
	for(size_t ny=0;ny< hicann_y_count; ++ny)
	{
		for(size_t nx=0; nx<hicann_x_count; ++nx)
		{
			if (hicann_config.at(ny).at(nx).get<0>()) 
			{
				int parent_dnc = hicann_config.at(ny).at(nx).get<2>();
				int dnc_hicann_channel = hicann_config.at(ny).at(nx).get<3>();
				auto cur_hicann_dnc_if = wafer_i->hicann_i[ny][nx]->dnc_if_i;
				auto cur_dnc = l2_dnc_i.at(parent_dnc);
                LOG4CXX_DEBUG(logger, name() << "::Connecting HICANN(x="<<nx <<", y="<<ny<<",configID="<< hicann_config.at(ny).at(nx).get<1>() << ") with DNC " << parent_dnc
						<< " via channel :" << dnc_hicann_channel << "( this is not the HICANNOnDNC().id(), but as seen from DNC)");
				// Hicann(DNC_IF) -> DNC
				cur_hicann_dnc_if ->dnc_channel_i->l2_dnc_if( *(cur_dnc->dnc_channel_i[dnc_hicann_channel]) );
				// DNC -> Hicann(DNC_IF)
				cur_dnc -> dnc_channel_i[ dnc_hicann_channel ] -> l2_dnc_if( *(cur_hicann_dnc_if->dnc_channel_i));
			}
		}
	}
}

l2_dnc* pcb::get_dnc(size_t dnc_id)
{
	return l2_dnc_i.at(dnc_id).get();
}	

l2_fpga* pcb::get_fpga(size_t fpga_id)
{
	return l2_fpga_i.at(fpga_id).get();
}

wafer* pcb::get_wafer()
{
	return wafer_i.get();
}
