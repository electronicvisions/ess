#include "systemc.h"
#include "tb_merger_tree.h"
#include "logger.h"
#include "merger_timed.h"
#include <string>

tb_merger_tree::tb_merger_tree(sc_module_name name):
	sc_module(name)
	, clock("clock", 4, SC_NS)
	, _log(Logger::instance())
{
	bg_merger_i[0] = new merger_timed("bg_merger_i0");
	bg_merger_i[1] = new merger_timed("bg_merger_i1");
	l0_merger_i = new merger_timed("l0_merger_i");

	l0_merger_i->connect_input_to(bg_merger_i[0],0);
	l0_merger_i->connect_input_to(bg_merger_i[1],1);
	
	//bg_merger_i[0]->set_select(0);
	//bg_merger_i[1]->set_select(1);
	//l0_merger_i->set_select(0);
	bg_merger_i[0]->set_enable(1);
	bg_merger_i[1]->set_enable(1);
	l0_merger_i->set_enable(1);
	l0_merger_i->set_slow(1);

	std::string filename = "spikes.dat";
	pulse_file_rx = fopen(filename.c_str(),"w");

	SC_THREAD(play_tx_event);

	SC_METHOD(check_for_events);
	dont_initialize();
	sensitive << clock.posedge_event();
}

tb_merger_tree::~tb_merger_tree()
{
	delete bg_merger_i[0];
	delete bg_merger_i[1];
	delete  l0_merger_i;
	fclose(pulse_file_rx);
}

void tb_merger_tree::play_tx_event()
{
	FILE* pulse_file;
	char buffer_line[1024];
	char* p;
	unsigned int wait_time;
	unsigned int next_spike_time_in_ns;
	int nrn_adr;
	unsigned int merger_id = 0;
	unsigned int merger_input = 0;
	sc_time next_spike_time;
	sc_time time_to_next_spike;

	std::stringstream filepath;
    filepath << "pulse_file_merger";
	pulse_file = fopen(filepath.str().c_str(),"rb");//OPEN Pulseplaybackfile

	if( pulse_file )
	{
		_log(Logger::INFO) << "Successfully opened pulse file " << filepath.str() << "@time " <<  sc_time_stamp();
    	while ( fgets(buffer_line, sizeof(buffer_line),pulse_file) )
		{
			p = buffer_line;
			if( !*p )		//leere Zeile
				continue;

			if( p[0]=='/' && p[1]=='/')	//Kommentar
			{
				_log(Logger::INFO) << "Comment: " << p;
			}
			else
			{
				// Data Format: spike time(in us), synapse type(0=excitatory, 1=inhibitory), weight(in nS)
				//_log(Logger::INFO) << "Read: " << p;
				//sscanf(p, "%u %d %u %u", &next_spike_time_in_ns, &nrn_adr, &merger_id, &merger_input);
				sscanf(p, "%u\t%d\t%d\t%d", &next_spike_time_in_ns, &nrn_adr, &merger_id, &merger_input);

				next_spike_time = sc_time((double) next_spike_time_in_ns, SC_NS);
				time_to_next_spike = (next_spike_time - sc_time_stamp());

				wait(time_to_next_spike);
				bool ack = bg_merger_i[merger_id]->nb_write_pulse(merger_input,nrn_adr);
				_log(Logger::INFO) << "Send spike at " << sc_time_stamp() << " with adr " << nrn_adr << " to merger " << (int) merger_id << " input " << (int) merger_input;
				_log << " ack= " << (int)ack << Logger::flush;
			}
		}
		fclose(pulse_file);
		_log(Logger::INFO) << "Successfully closed file" ;
	} else {
		_log(Logger::WARNING) << "Cannot open file " << filepath.str() << "@time " <<  sc_time_stamp();
	}
}
void tb_merger_tree::check_for_events()
{
	short addr;
	if(l0_merger_i->nb_read_pulse(addr)){
		_log(Logger::INFO) << "tb_merger_tree: received event with addr " << addr << " at time " << sc_time_stamp() << Logger::flush;
	}
}
