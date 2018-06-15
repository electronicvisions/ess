#include "systemc.h"
#include "tb_hw_neuron.h"
#include "hw_neuron.h"
#include "hw_neuron_ADEX.h"
#include "hw_neuron_IFSC.h"
#include "IFSC.h"
#include "logger.h"
#include <string>
//#include "neuron_def.h"

tb_hw_neuron::tb_hw_neuron(sc_module_name name):
	sc_module(name)
	,log(Logger::instance())
	,out_port(*this)

{
	/// init IFSC
	//IFSC_Init(0.25);
	//IF_cond_exp variables
	/*
	 params.E_l=-65;
	 params.I_gladapt=0.0; //a
	 params.I_gl=50.0; //g_l
	 params.I_radapt=0.144;//tau_w	
	 params.I_rexp=0.0;//delta_T	
	 params.V_exp=-50.4;
	 params.E_syn_i=-70.;
	 params.E_syn_e=0.0;
	 params.V_syntc_i=5.0;
	 params.V_syntc_e=5.0;
	 params.I_pl=0.0; // tau_refrac
	 params.V_Treset=-56.0; // V_spike
	 params.V_reset=-65;
	 params.Cap=1.;
	 params.I_fire=0.0;
	 */
	 // aEIF standard variables
	 params.E_l=-70.6;
	 params.I_gladapt=4.0; //a
	 params.I_gl=30.0; //g_l
	 params.I_radapt=144.0;//tau_w	
	 params.I_rexp=2.0;//delta_T	
	 params.V_exp=-50.4;
	 params.E_syn_i=-80.;
	 params.E_syn_e=0.0;
	 params.V_syntc_i=5.0;
	 params.V_syntc_e=5.0;
	 params.I_pl=0.5; // tau_refrac
	 params.V_Treset=0.0; // V_spike
	 params.V_reset=-70.6; 
	 params.Cap=0.281;
	 params.I_fire=0.0805;
	 //
	hw_neuron_i = new hw_neuron_ADEX("Neuron0", 0,0,0,&(this->out_port));
	hw_neuron_i->init(params,0,0,1, "voltage.dat",0.001);
	//hw_neuron_i->gen_random_spike();

	log(Logger::DEBUG0) << name << " built successfully!" << endl;

	std::string filename = "spikes.dat";
	pulse_file_rx = fopen(filename.c_str(),"w");

	SC_THREAD(play_tx_event);

}

tb_hw_neuron::~tb_hw_neuron()
{
	delete hw_neuron_i;
	fclose(pulse_file_rx);
}

void tb_hw_neuron::handle_spike(const int& bio_neuron, const sc_uint<9>& logical_neuron, const short& addr,  int wta_id ){

	double spike_time_in_s = sc_time_stamp().to_seconds();
	log(Logger::DEBUG0) << "Recorded Spike at: " << spike_time_in_s << endl;	
	
	if (pulse_file_rx) {	
	    fprintf(pulse_file_rx,"%.9f \n",spike_time_in_s);
	    fflush(pulse_file_rx);
	} else {
		log(Logger::WARNING) << "No open pulse record file found in pulse_rx_neuron";
	}

}


void tb_hw_neuron::play_tx_event()
{
	FILE* pulse_file;
	char buffer_line[1024];
	char* p;
	unsigned int next_spike_time_in_us;	
	unsigned int syn_type;
	unsigned int weight_in_nS;
	sc_time next_spike_time;
	sc_time time_to_next_spike;

	std::stringstream filepath;
    filepath << "pulse_file_neuron";
	pulse_file = fopen(filepath.str().c_str(),"rb");//OPEN Pulseplaybackfile

	if( pulse_file )
	{
		log(Logger::INFO) << "Successfully opened pulse file " << filepath.str() << "@time " <<  sc_time_stamp();
    	while ( fgets(buffer_line, sizeof(buffer_line),pulse_file) )
		{
			p = buffer_line;
			if( !*p )		//leere Zeile
				continue;

			if( p[0]=='/' && p[1]=='/')	//Kommentar
			{
				log(Logger::INFO) << "Comment: " << p;
			}
			else
			{
				// Data Format: spike time(in us), synapse type(0=excitatory, 1=inhibitory), weight(in nS)
				sscanf(p, "%u\t%u\t%u", &next_spike_time_in_us, &syn_type, &weight_in_nS);

				next_spike_time = sc_time((double) next_spike_time_in_us, SC_US);

				time_to_next_spike = (next_spike_time - sc_time_stamp());

               			wait(time_to_next_spike);
				hw_neuron_i->spike_in(weight_in_nS*1.e-9,syn_type);
			}
		}
		fclose(pulse_file);
		log(Logger::INFO) << "Successfully closed file" ;
	} else {
		log(Logger::WARNING) << "Cannot open file " << filepath.str() << "@time " <<  sc_time_stamp();
	}
	
}
