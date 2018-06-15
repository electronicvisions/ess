#include "systemc.h"
#include "logger.h"
#include "test_hw_neuron.h"


int sc_main(int argc, char* argv[]){
	Logger& log = Logger::instance(Logger::DEBUG2,"logfile_test_hw_neuron.txt");
	sc_set_time_resolution(1, SC_PS);
	test_hw_neuron tb_hw("TB");

	
	
	// Test code goes here.
	
	sc_start(1000, SC_MS);
	
	return(0);
}
