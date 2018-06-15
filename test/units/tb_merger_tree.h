#ifndef _TB_MERGER_TREE_H_
#define _TB_MERGER_TREE_H_

#include "systemc.h"

class Logger;
class merger;
/** class tb_merger_tree
 * serves as a testbench of a merger_tree
 */ 
class tb_merger_tree: public sc_module {

	public:
		void play_tx_event();	///< function to read input spikes from a file an inject them into the neuron

		SC_HAS_PROCESS(tb_merger_tree);
		/** Constructor.*/
		tb_merger_tree(sc_module_name name);

		~tb_merger_tree();
	protected:
		void check_for_events();
		sc_clock clock;
		merger * bg_merger_i[2]; ///< 2 bg mergers 
		merger * l0_merger_i; ///< 1 l0 merger
		FILE * pulse_file_rx; ///<
		Logger& _log;
};
#endif//_TB_MERGER_TREE_H_
