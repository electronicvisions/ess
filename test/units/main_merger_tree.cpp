#include "systemc.h"
#include "tb_merger_tree.h"
#include "logger.h"

int sc_main(int argc, char** argv){
	
	Logger& log = Logger::instance(Logger::DEBUG1,"logfile_merger.txt",true);
	log(Logger::INFO) << "Start";
	tb_merger_tree tb_merger_tree_i("tb_merger_tree_i");
	// Test code goes here.

	sc_start(200, SC_NS);
	sc_stop();
	log(Logger::INFO) << "Stop";
	return(0);
}
