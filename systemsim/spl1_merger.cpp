#include "spl1_merger.h"
#include "merger_config_if.h"
#include "bg_event_generator.h"

#include "merger_timed.h"
#include "dnc_merger_timed.h"
#include "priority_encoder.h"
#include "merger.h"
#include "dnc_merger.h"
#include "lost_event_logger.h"

#include <boost/assign/list_of.hpp>
#include <log4cxx/logger.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("ESS.HICANN.Layer1");


// helper for merger tree ascii art
void set_config_graph(std::string & graph, const std::string & item, merger * m) 
{
	std::string symbol_left      = " \\ ";
	std::string symbol_right     = " / ";
	std::string symbol_merge     = " V ";
	std::string symbol;

	if (!m) {
		return;
	}

	if ( m -> get_enable() ) {
		symbol = symbol_merge;
	}
	else {
		if ( m -> get_select() ) {
			symbol = symbol_left;
		}
		else {
			symbol = symbol_right;
		}
	}
	
	size_t i = graph.find(item);
	if (i != std::string::npos) {
		graph.replace(i, symbol.size(), symbol);
	}
}



const std::vector < unsigned int > spl1_merger::output_register_to_spl1_repeater =
	boost::assign::list_of(7)(6)(5)(4)(3)(2)(1)(0);

const std::vector < unsigned int > spl1_merger::wta_to_bg_merger =
	boost::assign::list_of(7)(6)(5)(4)(3)(2)(1)(0);

spl1_merger::~spl1_merger()
{
	for (size_t n_m=0; n_m < ANNCORE_WTA; ++n_m) {
		delete bg_merger_i[n_m];
		delete level_merger_i[n_m];
		delete dnc_merger_i[n_m];
		delete bg_event_generator_i[n_m];
		delete priority_encoder_i[n_m];
	}

}

spl1_merger::spl1_merger(
			sc_module_name name, //!< instance name
			short hicann_id,
			ess::mutexfile& spike_tcx_file,
			uint8_t PLL_period_ns,
            bool enable_timed,
            bool enable_spike_debugging
            ) : sc_module(name)
				, hicannid(hicann_id)
                , timed(enable_timed)
                , _spike_debugging(enable_spike_debugging)
                , spike_tx_file(spike_tcx_file)
				, bg_generator_seed(1u)
{
	bg_and_level_mergers.reserve(2*ANNCORE_WTA);

	char buffer[32];
	for (unsigned int n_m=0; n_m < ANNCORE_WTA; ++n_m) {
		snprintf(buffer,sizeof(buffer),"bg_merger_i%u",n_m);
		if(timed)
            bg_merger_i[n_m] =  new merger_timed(buffer, PLL_period_ns);
		else
            bg_merger_i[n_m] =  new merger(buffer);
		
        bg_and_level_mergers.push_back(bg_merger_i[n_m]);
	}
	for (unsigned int n_m=0; n_m < ANNCORE_WTA; ++n_m) {
		snprintf(buffer,sizeof(buffer),"level_merger_i%u",n_m);
		if(timed)
            level_merger_i[n_m] =  new merger_timed(buffer, PLL_period_ns);
        else
            level_merger_i[n_m] =  new merger(buffer);
		bg_and_level_mergers.push_back(level_merger_i[n_m]);
	}

	for (unsigned int n_m=0; n_m < ANNCORE_WTA; ++n_m) {
		snprintf(buffer,sizeof(buffer),"dnc_merger_i%u",n_m);
		if(timed)
            dnc_merger_i[n_m] =  new dnc_merger_timed(buffer,this,n_m,PLL_period_ns);
        else
            dnc_merger_i[n_m] =  new dnc_merger(buffer,this,n_m);
	}

	// Now create Background event generators
	for (unsigned int n_m=0; n_m < ANNCORE_WTA; ++n_m) {
		snprintf(buffer,sizeof(buffer),"bg_event_generator_i%u",n_m);
		bg_event_generator_i[n_m] = new bg_event_generator(buffer, bg_merger_i[n_m], PLL_period_ns);
	}
	// and priority encoders
	if(timed)
    {    
        for (unsigned int n_m=0; n_m < ANNCORE_WTA; ++n_m) {
    		snprintf(buffer,sizeof(buffer),"priority_encoder_i%u",n_m);
    		priority_encoder_i[n_m] = new priority_encoder(buffer, bg_merger_i[ wta_to_bg_merger.at(n_m) ], PLL_period_ns);
    	}
    }

	dnc_if_directions = std::vector< ESS::DNCIfDirections >(ANNCORE_WTA,ESS::DNCIfDirections::OFF);

	///////////////////////////////
	// Now connect the merger tree!
	///////////////////////////////
	connect_mergers();

}

void
spl1_merger::connect_mergers()
{
	// BG to level 0
	for (size_t n_m=0; n_m < ANNCORE_WTA; ++n_m) {
		bg_merger_i[n_m]->connect_output_to( level_merger_i[n_m/2], (n_m%2) );
	}
	// level 0 to level 1
	level_merger_i[0]->connect_output_to( level_merger_i[4], 0);
	level_merger_i[1]->connect_output_to( level_merger_i[4], 1);
	level_merger_i[2]->connect_output_to( level_merger_i[5], 0);
	level_merger_i[3]->connect_output_to( level_merger_i[5], 1);
	// level 1 to level 2
	level_merger_i[4]->connect_output_to( level_merger_i[6], 0);
	level_merger_i[5]->connect_output_to( level_merger_i[6], 1);

	// inputs to dnc from bgs and merger tree
	bg_merger_i[0]->connect_output_to( dnc_merger_i[0], 0);     // bg 0
	level_merger_i[0]->connect_output_to( dnc_merger_i[1], 0);  // L0 0
	level_merger_i[4]->connect_output_to( dnc_merger_i[2], 0);  // L1 0
	bg_merger_i[3]->connect_output_to( dnc_merger_i[3], 0);     // bg 3
	level_merger_i[6]->connect_output_to( dnc_merger_i[4], 0);  // L2 0
	bg_merger_i[5]->connect_output_to( dnc_merger_i[5], 0);     // bg 5
	level_merger_i[3]->connect_output_to( dnc_merger_i[6], 0);  // L0 3
	bg_merger_i[7]->connect_output_to( dnc_merger_i[7], 0);     // bg 7
}

bg_event_generator const& spl1_merger::get_background(size_t i) const
{ 
    return *(bg_event_generator_i[i]); 
}
	
merger const& spl1_merger::get_bg_merger(size_t i) const
{
	return *(bg_merger_i[i]);
}

void
spl1_merger::handle_spike(
		const sc_uint<LD_NRN_MAX>& logical_neuron, //!< ID of sending logical neuron(0..511) for debug information
		const short& addr,                         //!< 6-bit L1 address
		int wta_id                                 //!< the Priority Encoder(=WTA) receiving the spike (0..7)
	   )
{
    LOG4CXX_TRACE(logger, name() << "::handle_spike(logical_neuron" << ", " << addr << ", " << wta_id << ") called" );
	LostEventLogger::count_neuron_fired();
	if(_spike_debugging)
	{
		FILE *fpt2 = spike_tx_file.aquire_fp();
		fprintf(fpt2,"%.8i %.3i %.3i %.2i\n", (int)sc_simulation_time(), (int) hicannid, (int)logical_neuron, (int) addr);
		fflush(fpt2);
		spike_tx_file.release_fp(fpt2);
	}

	if(timed)
        priority_encoder_i[wta_id]->rcv_event(addr);
	else
        bg_merger_i[ wta_to_bg_merger[wta_id] ]->rcv_pulse(0,addr);
}

bool
spl1_merger::rcv_pulse_from_dnc_if_channel(
			short addr,  //!< 6-bit neuron address
			int  channel_id //!< channel ID (0..7)
			)
{
    LOG4CXX_TRACE(logger, name() <<  "::rcv_pulse_from_dnc_if_channel(" << addr << ", " << channel_id << ") called." );
    return dnc_merger_i[channel_id]->nb_write_pulse(1,addr); // dnc_if is connected to input[1] of dnc_merger. An existing event might be overwritten here!
}

void
spl1_merger::send_pulse_to_output_register(
			short addr,  //!< 6-bit neuron address
			unsigned int  channel_id //!< channel ID (0..7)
			)
{
    LOG4CXX_TRACE(logger, name() <<  "::send_pulse_to_output_register (addr: " << addr << ", channel_id: " << channel_id << " dnc_if_direction==TO_DNC: "
						 << (dnc_if_directions[channel_id] == ESS::DNCIfDirections::TO_DNC) << ") called" );

	if (dnc_if_directions[channel_id] == ESS::DNCIfDirections::TO_DNC) {
        LOG4CXX_TRACE(logger, " ... forwarded to dnc_if" );
		dnc_if_if->l1_task_if(channel_id,addr);
	}
	l1_task_if->rcv_pulse_from_this_hicann(output_register_to_spl1_repeater[channel_id], addr);
}

void spl1_merger::print_config(std::ostream & out)
{
	std::string graph =
	"(hicann doc./halbe notation)\n"
	" input convention: [1 0] (i.e., select==true is left)\n"
    "\n"
	"   0       1       2       3       4       5       6       7  \n"
    "  B N     B N     B N     B N     B N     B N     B N     B N \n"
	"  | |     | |     | |     | |     | |     | |     | |     | | \n"
	" [bg7]   [bg6]   [bg5]   [bg4]   [bg3]   [bg2]   [bg1]   [bg0]\n"
	"   |       |       |       |       |       |       |       |  \n"
	"   +--+ +--+       +--+ +--+       +--+ +--+       +--+ +--+  \n"
	"   |  | |          |  | |          |  | |             | |  |  \n"
	"   | [lm3]         | [lm2]         | [lm1]           [lm0] |  \n"
	"   |   |           |   |           |   |               |   |  \n"
	"   |   +---+--+ +--|---+           |   +------+ +------+   |  \n"
	"   |       |  | |  |               |          | |      |   |  \n"
	"   |       | [lm5] |               |         [lm4]     |   |  \n"
	"   |       |   |   |               |           |       |   |  \n"
	"   |       |   +---|---------+ +---|-------+---+   +---+   |  \n"
	"   |       |       |         | |   |       |       |       |  \n"
	"   |       |       |        [lm6]  |       |       |       |  \n"
	"   |       |       |          |    |       |       |       |  \n"
	" D |     D |     D |     D +--+  D |     D |     D |     D |  \n"
	" | |     | |     | |     | |     | |     | |     | |     | |  \n"
	"[dm7]   [dm6]   [dm5]   [dm4]   [dm3]   [dm2]   [dm1]   [dm0]\n"
    "                                                             \n"
    " dnc interface directions: \n"
	"[di7]   [di6]   [di5]   [di4]   [di3]   [di2]   [di1]   [di0]\n";

	char item[4];
	item[0] = 'b';
	item[1] = 'g';
	item[3] = '\0';

	// BG to level 0
	for (size_t n_m=0; n_m < ANNCORE_WTA; ++n_m) {
		assert(n_m < 10);
		item[2] = '0' + n_m;
		set_config_graph(graph, item, bg_merger_i[n_m]);
	}
	item[0] = 'l';
	item[1] = 'm';
	for (size_t n_m=0; n_m < ANNCORE_WTA; ++n_m) {
		assert(n_m < 10);
		item[2] = '0' + n_m;
		set_config_graph(graph, item, level_merger_i[n_m]);
	}

	item[0] = 'd';
	item[1] = 'm';
	for (size_t n_m=0; n_m < ANNCORE_WTA; ++n_m) {
		assert(n_m < 10);
		item[2] = '0' + n_m;
		set_config_graph(graph, item, dnc_merger_i[n_m]);
	}

	item[0] = 'd';
	item[1] = 'i';

	std::string todnc = "->D";
	std::string tohic = "->H";
	std::string off = "OFF";
	std::string inv = "???";

	for (size_t n_m=0; n_m < dnc_if_directions.size(); ++n_m) {
		assert(n_m < 10);
		item[2] = '0' + n_m;
		size_t i = graph.find(item);
		if (i != std::string::npos) {
			switch (dnc_if_directions[n_m])
			{
			case ESS::DNCIfDirections::OFF:
				graph.replace(i, todnc.size(), off);
				break;
			case ESS::DNCIfDirections::TO_HICANN:
				graph.replace(i, tohic.size(), tohic);
				break;
			case ESS::DNCIfDirections::TO_DNC:
				graph.replace(i, todnc.size(), todnc);
				break;
			default:
				graph.replace(i, inv.size(), inv);
				break;
			}
		}
	}
    out << graph;
}

void
spl1_merger::set_config(
	unsigned int adr,  //!< the adr of the register row, that is configured (0..19)
	const std::vector<bool>& config  //!< vector of size 16 containing the config of one address row.
			)
{
	//  TODO:
	//  3: loopback (0..7)
	//  5: phase (0..7) (not needed here)
	if(config.size() != 16)
        LOG4CXX_WARN(logger, "spl1_merger::set_config(): received vector with wrong size" );

	if (adr == 0) { // enable
		for (size_t n_mg = 0; n_mg < config.size(); ++n_mg)
			bg_and_level_mergers[n_mg]->set_enable(config[n_mg]);
	}
	else if ( adr == 1) { // select
		for (size_t n_mg = 0; n_mg < config.size(); ++n_mg)
			bg_and_level_mergers[n_mg]->set_select(config[n_mg]);
	}
	else if ( adr == 2) { // select
		for (size_t n_mg = 0; n_mg < config.size(); ++n_mg)
			bg_and_level_mergers[n_mg]->set_slow(config[n_mg]);
	}
	else if ( adr == 3) { // dncloopb and select
		for (size_t n_mg = 0; n_mg < 8; ++n_mg){
			dnc_merger_i[n_mg]->set_select(config[n_mg+8]);
		}
		// TODO loopback
	}
	else if ( adr == 4) { // random and reset_n of background event generators
		for (size_t n_mg = 0; n_mg < 8; ++n_mg){
			bg_event_generator_i[n_mg]->set_random(config[n_mg+8]);
			//bg_event_generator_i[n_mg]->reset_n(config[n_mg], bg_generator_seed);
			// setting seed from rand() in order to have different seeds for different generators.
			uint16_t beg_seed = rand();
			bg_event_generator_i[n_mg]->reset_n(config[n_mg], beg_seed);
		}
	}
	else if ( adr >= 6 && adr <= 14) { // common seed (6) and individual period (7-14) of the bg_event_generatos
		uint16_t new_value = 0;
		for (size_t n_mg = 0; n_mg < 16; ++n_mg){
			new_value ^= (config[n_mg] << n_mg) ;
		}
		if ( adr == 6 )  // common seed
			bg_generator_seed = new_value;
		else  // (7-14) individual periods
			bg_event_generator_i[ adr-7 ]->set_period(new_value);

	}
	else if ( adr == 15) { // slow and enable for dnc_merger_i
		for (size_t n_mg = 0; n_mg < 8; ++n_mg) {
			dnc_merger_i[n_mg]->set_enable(config[n_mg]);
			dnc_merger_i[n_mg]->set_slow(config[n_mg+8]);
		}
	}
	else if ( adr >= 16 && adr <= 19) { // neuron addr for bg_event_generators
		short adr_of_even_bg = 0;
		short adr_of_odd_bg = 0;
		for (size_t n_mg = 0; n_mg < 6; ++n_mg){
			adr_of_even_bg ^= (config[n_mg] << n_mg) ;
			adr_of_odd_bg ^= (config[n_mg+8] << n_mg) ;
		}
		bg_event_generator_i[ (adr-16)*2 ]->set_addr(adr_of_even_bg);
		bg_event_generator_i[ (adr-16)*2 + 1]->set_addr(adr_of_odd_bg);
	}
}

void spl1_merger::print_cfg(
	std::string fn  //!< name of file, to which the config will be printed
    )
{
    LOG4CXX_DEBUG(logger, name() << "dnc if directions:\t" );
	for (size_t n = 0; n<dnc_if_directions.size();++n)
        LOG4CXX_DEBUG(logger, dnc_if_directions[n] );

    LOG4CXX_DEBUG(logger, name() << hicannid << " print_cfg: " << fn );
	assert(fn.size() > 0 );
	std::ostringstream oss;
	oss << "_" << hicannid;
	fn.append(oss.str());
	std::ofstream fs(fn.c_str());

	assert (fs.is_open());

	print_config(fs);
}

void spl1_merger::set_dnc_if_directions(
		const std::vector<ESS::DNCIfDirections>& directions //!< vector of size 8 containing the config for each dnc if channel (1 means from Hicann to DNC )
		)
{
	if (directions.size() != dnc_if_directions.size()) {
        LOG4CXX_ERROR(logger, name() << "spl1_merger::set_dnc_if_direction(): received vector with wrong size" );
		return;
	}
	dnc_if_directions = directions;
}
