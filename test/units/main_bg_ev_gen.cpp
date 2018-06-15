#include "systemc.h"
#include "bg_event_generator.h"
#include "tb_bg_event_generator.h"
#include "statistics.h"
#include <string>
#include <fstream>

#include "logger.h"
#include <vector>

int main(int argc, char** argv){
	unsigned int period;
	if (argc > 1)
		period = atoi(argv[1]);
	else
		period = 1;

	Logger& log = Logger::instance(Logger::INFO,"logfile_bg.txt",true);

	//log(Logger::INFO) << "Period set to: " << period << std::endl;
	sc_set_time_resolution(1, SC_PS);

	tb_bg_event_generator tb_bg;
	tb_bg.bg_event_generator_i->set_random(true);
	tb_bg.bg_event_generator_i->set_addr(42);
	tb_bg.bg_event_generator_i->set_period(period);
	tb_bg.bg_event_generator_i->reset_n(true, 43409);


	// Test code goes here.

	sc_start(2*65536*5, SC_NS);
	std::vector<double>& st = tb_bg.spiketimes();
	std::vector<double> ISIs;
	if(st.size() > 1) {
		double first = st[0];
		//log(Logger::INFO) << "Spiketimes: " << first;
		for(size_t i = 1; i < st.size(); ++i) {
			double second = st[i];
			ISIs.push_back( second - first );
			first = second;
		//	log(Logger::INFO) << " " << first;
		}
		//log(Logger::INFO)  << std::endl;
		//log(Logger::INFO) << "ISIs: ";
		//for(size_t i = 0; i < ISIs.size(); ++i)
			//log(Logger::INFO) << " " << ISIs[i];
		//log(Logger::INFO)  << std::endl;
		std::pair<double,double> m_d = mean_and_dev<double>(ISIs);
		//log(Logger::INFO) << "Period set: " << bg_i.get_period()*4 << std::endl;
		//log(Logger::INFO) << "Mean ISI: " << m_d.first << " +- " << m_d.second << std::endl;
		//log(Logger::INFO) << "Spike count: " << st.size() << std::endl;
		//log(Logger::INFO) << "CV: " << m_d.second/m_d.first << std::endl;

		// write out results (Period,CV)
		std::string filename("random_3b.dat");
		std::ofstream myfile;
		myfile.open(filename.c_str(), std::ios::out | std::ios::app);
		myfile <<  period << "\t" << m_d.second/m_d.first << "\t" <<  m_d.first << "\n";
		myfile.close();
	}

	return(0);
}
