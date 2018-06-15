//_____________________________________________
// Company      :	tud
// Author       :	Stefan Scholze
// Refinement	:	Matthias Ehrlich
// E-Mail   	:	*@iee.et.tu-dresden.de
//
// Date         :   Thu Dec 14 14:10:13 2006
// Last Change  :   mehrlich 03/20/2008
// Module Name  :   dnc_if
// Filename     :   dnc_if.cpp
// Project Name	:   p_facets/s_systemsim
//
//_____________________________________________

#include "dnc_if.h"
#include "lost_event_logger.h"

#include <log4cxx/logger.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("ESS.Layer2");

///Function to handle received l2 data.
///activated ech clk cycle and checks receive fifo
void dnc_if::rx_l2_ctrl()
{

	if(this->dnc_channel_i->fifo_rx_event.num_available())
	{
		uint value;
		sc_uint<L2_EVENT_WIDTH> out;

		this->dnc_channel_i->fifo_rx_event.nb_read(value);
		//cout << "DNC_IF: ENTER rx_pls_l2_ctrl " << value << endl;
		//printf("ENTER rx_pls_l2_ctrl %.8X, at time: %i\n",value, (unsigned int) sc_simulation_time());fflush(stdout);
		out = value;
		//cout << "@ " << sc_simulation_time() << "From L1: " << hex << value << " from " << id << "\n";
		this->receive_event(out);
		LostEventLogger::count_dnc_if_rx_l2_ctrl();
	} else if(this->dnc_channel_i->fifo_rx_cfg.num_available())
	{
		this->dnc_channel_i->fifo_rx_cfg.nb_read(value_cfg);
		//printf("DNC_IF: ENTER rx_cfg_l2_ctrl %.8X %.8X @ %8X\n",value_cfg>>32,value_cfg,sc_simulation_time());fflush(stdout);
		// cfg_packet.notify(TIME_DES_CONFIG,SC_NS);
        LOG4CXX_WARN(logger, "DNC_IF:: rx_l2_ctrl(): received cfg_packet @ " << sc_simulation_time() << ", value_cfg: " << hex << value_cfg << " WHICH IS DISCARDED");
	}
}

void dnc_if::l1_task_if(const unsigned char& channel, const unsigned char& _nrnid)
{
	if (enable[channel]) {
		//checks for transmission of own neuron busses
        LOG4CXX_DEBUG(logger, "DNC_IF::l1_task_if called with channel = "  <<  (unsigned int) channel << ", nrnid = " << (unsigned int) _nrnid);
		this->sim_time=sc_simulation_time();
		this->nrnid = (channel*(1 << L1_ADDR_WIDTH)) + _nrnid;
        LOG4CXX_DEBUG(logger, "DNC_IF::l1_task_if: new this->nrnid = " << (unsigned int) this->nrnid);

		if (l1direction[channel]==TO_DNC)		//checks for transmission of own neuron busses
		{
			// pulse_event.notify();
            LOG4CXX_DEBUG(logger, "DNC_IF: Start Layer1 Pulse L1->DNC: nrnid=" << (uint) _nrnid << ", channel=" << (uint) channel << ", hicannid=" << hicannid << ", @time=" << sc_time_stamp());
			LostEventLogger::count_dnc_if_l1_task_if();
			start_pulse_dnc();
		} else {
            LOG4CXX_WARN(logger, "DNC_IF: StoppedLayer1 Pulse L1->DNC: nrnid=" << (uint) _nrnid << ", channel=" << (uint) channel << ", hicannid=" << hicannid << ", @time=" << sc_time_stamp() << ". Direction of this Bus is set towards HICANN");
		}
	} else {
		// the following is not a wrong configuration. But happens e.g. if there are background generators or neurons that shall not be recorded.
        LOG4CXX_DEBUG(logger, "DNC_IF: StoppedLayer1 Pulse L1->DNC: nrnid=" << (uint) _nrnid << ", channel=" << (uint) channel << ", hicannid=" << hicannid << ", @time=" << sc_time_stamp() << ". Channel is NOT enabled.");
	}
}

/// Sends a pulse to the DNC.
void dnc_if::start_pulse_dnc()
{
	// divide simtime by 4 to get system clock cycles
    LOG4CXX_DEBUG(logger, "DNC_IF: Pulse send to DNC: nrnid " << (unsigned int) nrnid);
	dnc_channel_i -> start_event(((nrnid & 0xfff)<<TIMESTAMP_WIDTH) + (((uint)(sim_time)>>2) & 0x7fff));
	LostEventLogger::count_dnc_if_start_pulse_dnc();
}


/// external interface for l2 packet receive.
/// upper bits of packets selects layer1 channel
/// @param l1event: l2 packet with [L1_Bus_nr + Neuron_nr + Timestamp]
void dnc_if::receive_event(const sc_uint<L2_EVENT_WIDTH>& l1event)
{
	unsigned int channel = l1event.to_uint() >> (TIMESTAMP_WIDTH+L1_ADDR_WIDTH);
	if (enable[channel]) {
		// check here if l1_bus is configured in direction DNC->L1
		if(l1direction[channel]==TO_HICANN){
			l2tol1_tx_i[channel]->transmit(l1event & 0x3ffffff);
			LostEventLogger::count_dnc_if_receive_event();
			char buffer[256];
			sprintf(buffer, "DNC_IF: %X :\t received DNC event @ HICANN %i DNC_IF %.8X in channel %i\n",(unsigned int)sc_simulation_time(), hicannid, l1event.to_uint(),channel);
            LOG4CXX_DEBUG(logger, buffer);
		}	
		else {
			char buffer[256];
			sprintf(buffer, "DNC_IF: WARNING::%X :\t received invalid DNC event @ HICANN %i DNC_IF %.8X in channel %i: l1direction is set to TOWARDS_DNC!\n",(unsigned int)sc_simulation_time(), hicannid, l1event.to_uint(),channel);
            LOG4CXX_WARN(logger, buffer);
			LostEventLogger::log(/*downwards*/ true, name());
		}
	} else {
		char buffer[256];
		sprintf(buffer, "DNC_IF: WARNING::%X :\t received DNC event @ HICANN %i DNC_IF %.8X in channel %i: which is DISABLED\n",(unsigned int)sc_simulation_time(), hicannid, l1event.to_uint(),channel);
        LOG4CXX_WARN(logger, buffer);
		LostEventLogger::log(/*downwards*/ true, name());
	}
}

/// sets direction for each L2toL1 channel.
void dnc_if::set_directions(const std::vector< enum dnc_if::direction >& directions) {
	assert(directions.size() == DNCL1BUSINCOUNT);
	for (size_t i = 0; i<DNCL1BUSINCOUNT; ++i) {
		l1direction[i] = directions[i];
		// direction is l2tol1_tx:
		if (directions[i] == TO_HICANN)
			l2tol1_tx_i[i]->set_direction(l2tol1_tx::TO_HICANN);
		else if (directions[i] == TO_DNC)
			l2tol1_tx_i[i]->set_direction(l2tol1_tx::TO_DNC);
	}
}
/// sets enable for each L2toL1 channel.
void dnc_if::set_enables(const std::vector<bool>& enables) {
	assert(enables.size() == DNCL1BUSINCOUNT);
	for (size_t i = 0; i<DNCL1BUSINCOUNT; ++i) {
		enable[i] = enables[i];
	}
}
