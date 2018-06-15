#include "systemc.h"
#include "tb_priority_encoder.h"
#include "logger.h"

int sc_main(int argc, char** argv){
	
	Logger& log = Logger::instance(Logger::INFO,"logfile_pe.txt",true);
	log(Logger::INFO) << "Start";
	tb_priority_encoder tb_pe_i("tb_pe_i");
	// Test code goes here.

	sc_start(200, SC_NS);
	sc_stop();
	log(Logger::INFO) << "Stop";
	return(0);
}
