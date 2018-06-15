#include "lost_event_logger.h"
#include "logger.h"
#include "systemc.h"

unsigned int LostEventLogger::count[2] = {0,0};
unsigned int LostEventLogger::count_wafer = 0;
unsigned int LostEventLogger::_count_fpga = 0;
unsigned int LostEventLogger::_count_pre_sim = 0;
unsigned int LostEventLogger::_count_dropped_pre_sim = 0;

unsigned int LostEventLogger::_count_dnc_tx_fpga_start_event[2] = {0,0};
unsigned int LostEventLogger::_count_dnc_tx_fpga_transmit[2] = {0,0};
unsigned int LostEventLogger::_count_dnc_tx_fpga_transmit_start_event[2] = {0,0};
unsigned int LostEventLogger::_count_dnc_tx_fpga_receive_event[2] = {0,0};
unsigned int LostEventLogger::_count_dnc_tx_fpga_fill_fifo[2] = {0,0};
unsigned int LostEventLogger::_count_dnc_tx_fpga_write_fifo[2] = {0,0};


unsigned int LostEventLogger::_count_l2_dnc_fifo_from_fpga_ctrl = 0;
unsigned int LostEventLogger::_count_l2_dnc_transmit_from_fpga = 0;
unsigned int LostEventLogger::_count_l2_dnc_transmit_to_anc = 0;
unsigned int LostEventLogger::_count_l2_dnc_delay_mem_ctrl = 0;

unsigned int LostEventLogger::_count_dnc_ser_channel_start_event[2] = {0,0};
unsigned int LostEventLogger::_count_dnc_ser_channel_transmit[2] = {0,0};
unsigned int LostEventLogger::_count_dnc_ser_channel_transmit_start_event[2] = {0,0};
unsigned int LostEventLogger::_count_dnc_ser_channel_receive_event[2] = {0,0};
unsigned int LostEventLogger::_count_dnc_ser_channel_fill_fifo[2] = {0,0};
unsigned int LostEventLogger::_count_dnc_ser_channel_write_fifo[2] = {0,0};

unsigned int LostEventLogger::_count_dnc_if_rx_l2_ctrl= 0;
unsigned int LostEventLogger::_count_dnc_if_receive_event= 0;
unsigned int LostEventLogger::_count_l2tol1_tx_transmit= 0;
unsigned int LostEventLogger::_count_l2tol1_tx_add_buffer= 0;
unsigned int LostEventLogger::_count_l2tol1_tx_check_time= 0;
unsigned int LostEventLogger::_count_l2tol1_tx_serialize= 0;


unsigned int LostEventLogger::_count_dnc_if_l1_task_if = 0;
unsigned int LostEventLogger::_count_dnc_if_start_pulse_dnc = 0;
unsigned int LostEventLogger::_count_l2_dnc_fifo_from_anc_ctrl = 0;
unsigned int LostEventLogger::_count_l2_dnc_transmit_from_anc = 0;
unsigned int LostEventLogger::_count_l2_fpga_fifo_from_l2_ctrl = 0;
unsigned int LostEventLogger::_count_l2_fpga_transmit_l2_event = 0;
unsigned int LostEventLogger::_count_l2_fpga_record_rx_event = 0;

unsigned int LostEventLogger::_count_neuron_fired = 0;
unsigned int LostEventLogger::_count_priority_encoder_rcv_event = 0;
unsigned int LostEventLogger::_count_priority_encoder_send_event = 0;

Logger::levels LostEventLogger::_loglevel = Logger::DEBUG1;

void LostEventLogger::summary() {
	Logger& log = Logger::instance();
	log(Logger::INFO) << "*************************************";
	log(Logger::INFO) << "LostEventLogger::summary";
	unsigned int total_lost_l2 = count[0] + count[1];
	unsigned int total_sent_l2 = _count_fpga + _count_dnc_if_l1_task_if;
	log(Logger::INFO) << "Layer 2 events dropped before sim : " << _count_dropped_pre_sim << "/" << _count_pre_sim << " (" << ( _count_pre_sim > 0 ?(_count_dropped_pre_sim*100./_count_pre_sim): 0 ) << " %)";
	log(Logger::INFO) << "Layer 2 events lost :               " << total_lost_l2 << "/" << total_sent_l2 << " (" << ( total_sent_l2 > 0 ?(total_lost_l2*100./total_sent_l2): 0 ) << " %)";
	log(Logger::INFO) << "Layer 2 events lost downwards :     " << count[1] << "/" << _count_fpga << " (" << ( _count_fpga > 0 ? (count[1]*100./_count_fpga): 0) << " %)";
	log(Logger::INFO) << "Layer 2 events lost upwards   :     " << count[0] << "/" << _count_dnc_if_l1_task_if << " (" << (_count_dnc_if_l1_task_if > 0 ? (count[0]*100./_count_dnc_if_l1_task_if):0) << " %)";
	log(Logger::INFO) << "Layer 1 events lost : " << count_wafer << "/" << _count_neuron_fired<< " (" << (_count_neuron_fired> 0 ? (count_wafer*100./_count_neuron_fired):0) << " %)";
	log(Logger::INFO) << "*************************************";
	if ( log.willBeLogged(_loglevel) ){
		log(_loglevel) << "DOWNSTREAM";
		log(_loglevel) << "*************************************";
		log(_loglevel) << "Events sent l2_fpga::play_tx_event:            " << _count_fpga;
		log(_loglevel) << "Events sent dnc_tx_fpga::start_event:          " << _count_dnc_tx_fpga_start_event[1];
		log(_loglevel) << "Events sent dnc_tx_fpga::transmit:             " << _count_dnc_tx_fpga_transmit[1];
		log(_loglevel) << "Events sent dnc_tx_fpga::transmit_start_event: " << _count_dnc_tx_fpga_transmit_start_event[1];
		log(_loglevel) << "Events sent dnc_tx_fpga::receive_event:        " << _count_dnc_tx_fpga_receive_event[1];
		log(_loglevel) << "Events sent dnc_tx_fpga::fill_fifo:            " << _count_dnc_tx_fpga_fill_fifo[1];
		log(_loglevel) << "Events sent dnc_tx_fpga::write_fifo:           " << _count_dnc_tx_fpga_write_fifo[1];
		log(_loglevel) << "Events sent l2_dnc::fifo_from_fpga_ctrl:       " << _count_l2_dnc_fifo_from_fpga_ctrl;
		log(_loglevel) << "Events sent l2_dnc::transmit_from_fpga:        " << _count_l2_dnc_transmit_from_fpga;
		log(_loglevel) << "Events sent l2_dnc::transmit_to_anc:           " << _count_l2_dnc_transmit_to_anc;
		log(_loglevel) << "Events sent l2_dnc::delay_mem_ctrl:            " << _count_l2_dnc_delay_mem_ctrl;
		log(_loglevel) << "Events sent dnc_ser_channel::start_event:      " << _count_dnc_ser_channel_start_event[1];
		log(_loglevel) << "Events sent dnc_ser_channel::transmit:         " << _count_dnc_ser_channel_transmit[1];
		log(_loglevel) << "Events sent dnc_ser_channel::transmit_start_event: " << _count_dnc_ser_channel_transmit_start_event[1];
		log(_loglevel) << "Events sent dnc_ser_channel::receive_event:    " << _count_dnc_ser_channel_receive_event[1];
		log(_loglevel) << "Events sent dnc_ser_channel::fill_fifo:        " << _count_dnc_ser_channel_fill_fifo[1];
		log(_loglevel) << "Events sent dnc_ser_channel::write_fifo:       " << _count_dnc_ser_channel_write_fifo[1];
		log(_loglevel) << "Events sent dnc_if::rx_l2_ctrl:                " << _count_dnc_if_rx_l2_ctrl;
		log(_loglevel) << "Events sent dnc_if::receive_event:             " << _count_dnc_if_receive_event;
		log(_loglevel) << "Events sent l2tol1_tx::transmit:               " << _count_l2tol1_tx_transmit;
		log(_loglevel) << "Events sent l2tol1_tx::add_buffer:             " << _count_l2tol1_tx_add_buffer;
		log(_loglevel) << "Events sent l2tol1_tx::check_time:             " << _count_l2tol1_tx_check_time;
		log(_loglevel) << "Events sent l2tol1_tx::serialize:              " << _count_l2tol1_tx_serialize;
		log(_loglevel) << "*************************************";
		log(_loglevel) << "*************************************";
		log(_loglevel) << "UPSTREAM";
		log(_loglevel) << "*************************************";
		log(_loglevel) << "Events send dnc_if::l1_task_if:                " << _count_dnc_if_l1_task_if;
		log(_loglevel) << "Events send dnc_if::start_pulse_dnc:           " << _count_dnc_if_start_pulse_dnc;
		log(_loglevel) << "Events sent dnc_ser_channel::start_event:      " << _count_dnc_ser_channel_start_event[0];
		log(_loglevel) << "Events sent dnc_ser_channel::transmit:         " << _count_dnc_ser_channel_transmit[0];
		log(_loglevel) << "Events sent dnc_ser_channel::transmit_start_event: " << _count_dnc_ser_channel_transmit_start_event[0];
		log(_loglevel) << "Events sent dnc_ser_channel::receive_event:    " << _count_dnc_ser_channel_receive_event[0];
		log(_loglevel) << "Events sent dnc_ser_channel::fill_fifo:        " << _count_dnc_ser_channel_fill_fifo[0];
		log(_loglevel) << "Events sent dnc_ser_channel::write_fifo:       " << _count_dnc_ser_channel_write_fifo[0];
		log(_loglevel) << "Events send l2_dnc::fifo_from_anc_ctrl:        " << _count_l2_dnc_fifo_from_anc_ctrl;
		log(_loglevel) << "Events send l2_dnc::transmit_from_anc:         " << _count_l2_dnc_transmit_from_anc;
		log(_loglevel) << "Events sent dnc_tx_fpga::start_event:          " << _count_dnc_tx_fpga_start_event[0];
		log(_loglevel) << "Events sent dnc_tx_fpga::transmit:             " << _count_dnc_tx_fpga_transmit[0];
		log(_loglevel) << "Events sent dnc_tx_fpga::transmit_start_event: " << _count_dnc_tx_fpga_transmit_start_event[0];
		log(_loglevel) << "Events sent dnc_tx_fpga::receive_event:        " << _count_dnc_tx_fpga_receive_event[0];
		log(_loglevel) << "Events sent dnc_tx_fpga::fill_fifo:            " << _count_dnc_tx_fpga_fill_fifo[0];
		log(_loglevel) << "Events sent dnc_tx_fpga::write_fifo:           " << _count_dnc_tx_fpga_write_fifo[0];
		log(_loglevel) << "Events send l2_fpga::fifo_from_l2_ctrl:        " << _count_l2_fpga_fifo_from_l2_ctrl;
		log(_loglevel) << "Events send l2_fpga::transmit_l2_event:        " << _count_l2_fpga_transmit_l2_event;
		log(_loglevel) << "Events send l2_fpga::record_rx_event:          " << _count_l2_fpga_record_rx_event;
		log(_loglevel) << "*************************************";
		log(_loglevel) << "*************************************";
		log(_loglevel) << "ON WAFER";
		log(_loglevel) << "*************************************";
		log(_loglevel) << "Events sent by neurons:                " << _count_neuron_fired;
		log(_loglevel) << "Events registered by priority encoder: " << _count_priority_encoder_rcv_event;
		log(_loglevel) << "Events handleld by merger tree:        " << _count_priority_encoder_send_event;
		log(_loglevel) << "*************************************";
	}
}

void LostEventLogger::log(bool downwards, std::string location){
	count[downwards]++;
	Logger& Log = Logger::instance();
	if (Log.willBeLogged(_loglevel))
		Log(_loglevel) << "Lost L2 Event @ " << sc_simulation_time() << " ns in " << location;
}

void LostEventLogger::log_wafer(std::string location){
	count_wafer++;
	Logger& Log = Logger::instance();
	if (Log.willBeLogged(_loglevel))
		Log(_loglevel) << "Lost L1 Event @ " << sc_simulation_time() << " ns in " << location;
}

void LostEventLogger::print_summary_to_file(std::string file)
{
	std::ofstream fout;
	fout.open(file.c_str());
	fout << "pulse_statistics = {\n";
	fout << "'l2_down_before_sim' :" << _count_pre_sim << ",\n";
	fout << "'l2_down_dropped_before_sim' :" << _count_dropped_pre_sim << ",\n";
	fout << "'l2_down_sent' :" << _count_fpga << ",\n";
	fout << "'l2_down_lost' :" << count[1] << ",\n";
	fout << "'l2_up_sent' :" << _count_dnc_if_l1_task_if << ",\n";
	fout << "'l2_up_lost' :" << count[0] << ",\n";
	fout << "'l1_neuron_sent' :" << _count_neuron_fired << ",\n";
	fout << "'l1_neuron_lost' :" << count_wafer << ",\n";
	fout << "}\n";

	fout.close();
}
void LostEventLogger::reset()
{
	count[0] = 0;
	count[1] = 0;
	count_wafer = 0;
	_count_fpga = 0;
	_count_pre_sim = 0;
	_count_dropped_pre_sim = 0;

	_count_dnc_tx_fpga_start_event[0] = 0;
	_count_dnc_tx_fpga_transmit[0] = 0;
	_count_dnc_tx_fpga_transmit_start_event[0] = 0;
	_count_dnc_tx_fpga_receive_event[0] = 0;
	_count_dnc_tx_fpga_fill_fifo[0] = 0;
	_count_dnc_tx_fpga_write_fifo[0] = 0;

	_count_dnc_tx_fpga_start_event[1] = 0;
	_count_dnc_tx_fpga_transmit[1] = 0;
	_count_dnc_tx_fpga_transmit_start_event[1] = 0;
	_count_dnc_tx_fpga_receive_event[1] = 0;
	_count_dnc_tx_fpga_fill_fifo[1] = 0;
	_count_dnc_tx_fpga_write_fifo[1] = 0;


	_count_l2_dnc_fifo_from_fpga_ctrl = 0;
	_count_l2_dnc_transmit_from_fpga = 0;
	_count_l2_dnc_transmit_to_anc = 0;
	_count_l2_dnc_delay_mem_ctrl = 0;

	_count_dnc_ser_channel_start_event[0] = 0;
	_count_dnc_ser_channel_transmit[0] = 0;
	_count_dnc_ser_channel_transmit_start_event[0] = 0;
	_count_dnc_ser_channel_receive_event[0] = 0;
	_count_dnc_ser_channel_fill_fifo[0] = 0;
	_count_dnc_ser_channel_write_fifo[0] = 0;

	_count_dnc_ser_channel_start_event[1] = 0;
	_count_dnc_ser_channel_transmit[1] = 0;
	_count_dnc_ser_channel_transmit_start_event[1] = 0;
	_count_dnc_ser_channel_receive_event[1] = 0;
	_count_dnc_ser_channel_fill_fifo[1] = 0;
	_count_dnc_ser_channel_write_fifo[1] = 0;

	_count_dnc_if_rx_l2_ctrl= 0;
	_count_dnc_if_receive_event= 0;
	_count_l2tol1_tx_transmit= 0;
	_count_l2tol1_tx_add_buffer= 0;
	_count_l2tol1_tx_check_time= 0;
	_count_l2tol1_tx_serialize= 0;


	_count_dnc_if_l1_task_if = 0;
	_count_dnc_if_start_pulse_dnc = 0;
	_count_l2_dnc_fifo_from_anc_ctrl = 0;
	_count_l2_dnc_transmit_from_anc = 0;
	_count_l2_fpga_fifo_from_l2_ctrl = 0;
	_count_l2_fpga_transmit_l2_event = 0;
	_count_l2_fpga_record_rx_event = 0;

	_count_neuron_fired = 0;
	_count_priority_encoder_rcv_event = 0;
	_count_priority_encoder_send_event = 0;
}
