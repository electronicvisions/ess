#include "systemc.h"
#include "logger.h"
#include "tb_hw_neuron.h"


int sc_main(int argc, char* argv[]){
	Logger& log = Logger::instance(Logger::DEBUG2,"");
	sc_set_time_resolution(1, SC_PS);
	tb_hw_neuron tb_hw("TB");

	
	
	// Test code goes here.
	
	sc_start(1000, SC_MS);
	
	return(0);
}
