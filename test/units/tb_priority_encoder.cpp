#include "systemc.h"
#include "tb_priority_encoder.h"
#include "priority_encoder.h"
#include "logger.h"
#include <string>

tb_priority_encoder::tb_priority_encoder(sc_module_name name):
	sc_module(name)
	, _log(Logger::instance())
{
	priority_encoder_i = new priority_encoder("priority_encoder_i", this);
	std::string filename = "spikes.dat";
	pulse_file_rx = fopen(filename.c_str(),"w");

	SC_THREAD(play_tx_event);

}

tb_priority_encoder::~tb_priority_encoder()
{
	delete priority_encoder_i;
	fclose(pulse_file_rx);
}

void tb_priority_encoder::play_tx_event()
{
	FILE* pulse_file;
	char buffer_line[1024];
	char* p;
	unsigned int wait_time;
	unsigned int next_spike_time_in_ns;
	int denmem;
	sc_time next_spike_time;
	sc_time time_to_next_spike;

	std::stringstream filepath;
    filepath << "pulse_file_pe";
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
				sscanf(p, "%u\t%d", &next_spike_time_in_ns, &denmem);

				_log(Logger::INFO) << "Send spike at " << next_spike_time_in_ns << " to " << denmem;
				next_spike_time = sc_time((double) next_spike_time_in_ns, SC_NS);
				time_to_next_spike = (next_spike_time - sc_time_stamp());

				wait(time_to_next_spike);
				priority_encoder_i->rcv_event(denmem);
			}
		}
		fclose(pulse_file);
		_log(Logger::INFO) << "Successfully closed file" ;
	} else {
		_log(Logger::WARNING) << "Cannot open file " << filepath.str() << "@time " <<  sc_time_stamp();
	}
}
bool tb_priority_encoder::nb_write_pulse(
	bool input, //!< input, either 0 or 1
	short nrn_adr //!< 6-bit neuron address
	) {

	_log(Logger::INFO) << "tb_priority_encoder::nb_write_pulse("<< (int) input << ", " << nrn_adr << ") called at " << sc_time_stamp();
	return true;
}

bool tb_priority_encoder::write_pulse(
	bool input, //!< input, either 0 or 1
	short nrn_adr //!< 6-bit neuron address
	) {

	_log(Logger::INFO) << "tb_priority_encoder::write_pulse("<< (int) input << ", " << nrn_adr << ") called at " << sc_time_stamp();
	return true;
}
