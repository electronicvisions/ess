#ifndef _LOST_EVENT_LOGGER_H_
#define _LOST_EVENT_LOGGER_H_

#include <string>
#include "logger.h"

struct LostEventLogger {
private:
	static unsigned int count[2];
	static unsigned int count_wafer;
	static unsigned int _count_fpga;

	// before simulation
	static unsigned int _count_pre_sim; // dropped events before simulation
	static unsigned int _count_dropped_pre_sim; // all events before simulation
	// fpga side
	static unsigned int _count_dnc_tx_fpga_start_event[2];
	static unsigned int _count_dnc_tx_fpga_transmit[2];
	static unsigned int _count_dnc_tx_fpga_transmit_start_event[2];
	// dnc side
	static unsigned int _count_dnc_tx_fpga_receive_event[2];
	static unsigned int _count_dnc_tx_fpga_fill_fifo[2];
	static unsigned int _count_dnc_tx_fpga_write_fifo[2];
	// dnc
	static unsigned int _count_l2_dnc_fifo_from_fpga_ctrl;
	static unsigned int _count_l2_dnc_transmit_from_fpga;
	static unsigned int _count_l2_dnc_transmit_to_anc;
	static unsigned int _count_l2_dnc_delay_mem_ctrl;
	// dnc_ser_channel
	// dnc side
	static unsigned int _count_dnc_ser_channel_start_event[2];
	static unsigned int _count_dnc_ser_channel_transmit[2];
	static unsigned int _count_dnc_ser_channel_transmit_start_event[2];
	// dnc_if side
	static unsigned int _count_dnc_ser_channel_receive_event[2];
	static unsigned int _count_dnc_ser_channel_fill_fifo[2];
	static unsigned int _count_dnc_ser_channel_write_fifo[2];

	// dnc _if
	static unsigned int _count_dnc_if_rx_l2_ctrl;
	static unsigned int _count_dnc_if_receive_event;
	// l2tol1_tx
	static unsigned int _count_l2tol1_tx_transmit;
	static unsigned int _count_l2tol1_tx_add_buffer;
	static unsigned int _count_l2tol1_tx_check_time;
	static unsigned int _count_l2tol1_tx_serialize;


	// upstream
	// dnc_if
	static unsigned int _count_dnc_if_l1_task_if;
	static unsigned int _count_dnc_if_start_pulse_dnc;
	// dnc_ser_channel
	// l2_dnc
	static unsigned int _count_l2_dnc_fifo_from_anc_ctrl;
	static unsigned int _count_l2_dnc_transmit_from_anc;
	// dnc_tx_fpga
	// l2_fpga
	static unsigned int _count_l2_fpga_fifo_from_l2_ctrl;
	static unsigned int _count_l2_fpga_transmit_l2_event;
	static unsigned int _count_l2_fpga_record_rx_event;


	// SPL1 Merger-Tree and Priority-Encoder
	static unsigned int _count_neuron_fired;
	static unsigned int _count_priority_encoder_rcv_event;
	static unsigned int _count_priority_encoder_send_event;

	static Logger::levels _loglevel; // Loglevel for detailed summary and logging of individual lost events. default: DEBUG1

public:
	static void log(bool downwards) {
		count[downwards]++;
	}
	static void log_pre_sim() {
		_count_dropped_pre_sim++;
	}
	static void log(bool downwards, std::string location);
	static void log_wafer(std::string location); /// logs a lost event on the wafer(i.e. between neurons and spl1 merger)
	static void count_pre_sim() { _count_pre_sim++; }
	static void count_fpga(){ _count_fpga++;}

	static void count_dnc_tx_fpga_start_event(bool downwards){_count_dnc_tx_fpga_start_event[downwards]++;}
	static void count_dnc_tx_fpga_transmit(bool downwards){_count_dnc_tx_fpga_transmit[downwards]++;}
	static void count_dnc_tx_fpga_transmit_start_event(bool downwards){_count_dnc_tx_fpga_transmit_start_event[downwards]++;}
	static void count_dnc_tx_fpga_receive_event(bool downwards){_count_dnc_tx_fpga_receive_event[downwards]++;}
	static void count_dnc_tx_fpga_fill_fifo(bool downwards){_count_dnc_tx_fpga_fill_fifo[downwards]++;}
	static void count_dnc_tx_fpga_write_fifo(bool downwards){_count_dnc_tx_fpga_write_fifo[downwards]++;}

	static void count_l2_dnc_fifo_from_fpga_ctrl(){_count_l2_dnc_fifo_from_fpga_ctrl++;}
	static void count_l2_dnc_transmit_from_fpga(){_count_l2_dnc_transmit_from_fpga++;}
	// static void xxx(){xxx++;}
	static void count_l2_dnc_transmit_to_anc(){_count_l2_dnc_transmit_to_anc++;}
	static void count_l2_dnc_delay_mem_ctrl(){_count_l2_dnc_delay_mem_ctrl++;}

	static void count_dnc_ser_channel_start_event(bool downwards){ _count_dnc_ser_channel_start_event[downwards]++;}
	static void count_dnc_ser_channel_transmit(bool downwards){_count_dnc_ser_channel_transmit[downwards]++;}
	static void count_dnc_ser_channel_transmit_start_event(bool downwards) {_count_dnc_ser_channel_transmit_start_event[downwards]++;}
	static void count_dnc_ser_channel_receive_event(bool downwards) {_count_dnc_ser_channel_receive_event[downwards]++;}
	static void count_dnc_ser_channel_fill_fifo(bool downwards){_count_dnc_ser_channel_fill_fifo[downwards]++;}
	static void count_dnc_ser_channel_write_fifo(bool downwards){_count_dnc_ser_channel_write_fifo[downwards]++;}

	static void count_dnc_if_rx_l2_ctrl() {_count_dnc_if_rx_l2_ctrl++;}
	static void count_dnc_if_receive_event() {_count_dnc_if_receive_event++;}
	static void count_l2tol1_tx_transmit(){ _count_l2tol1_tx_transmit++;}
	static void count_l2tol1_tx_add_buffer(){ _count_l2tol1_tx_add_buffer++;}
	static void count_l2tol1_tx_check_time(){ _count_l2tol1_tx_check_time++;}
	static void count_l2tol1_tx_serialize(){ _count_l2tol1_tx_serialize++;}



	static void count_dnc_if_l1_task_if() { _count_dnc_if_l1_task_if++;}
	static void count_dnc_if_start_pulse_dnc() { _count_dnc_if_start_pulse_dnc++;}
	static void count_l2_dnc_fifo_from_anc_ctrl() { _count_l2_dnc_fifo_from_anc_ctrl++;}
	static void count_l2_dnc_transmit_from_anc() { _count_l2_dnc_transmit_from_anc++;}
	static void count_l2_fpga_fifo_from_l2_ctrl() { _count_l2_fpga_fifo_from_l2_ctrl++;}
	static void count_l2_fpga_transmit_l2_event() { _count_l2_fpga_transmit_l2_event++;}
	static void count_l2_fpga_record_rx_event() { _count_l2_fpga_record_rx_event++;}

	static void count_neuron_fired() {_count_neuron_fired++;}
	static void count_priority_encoder_rcv_event() {_count_priority_encoder_rcv_event++;}
	static void count_priority_encoder_send_event() {_count_priority_encoder_send_event++;}

	static void set_loglevel(Logger::levels loglevel) {_loglevel=loglevel;}
	static void summary();
	static void print_summary_to_file(std::string file);
	static void reset();
};
#endif //_LOST_EVENT_LOGGER_H_
