//_____________________________________________
// Company      :	Tu-Dresden
// Author       :	Stefan Scholze
// E-Mail   	:	scholze@iee.et.tu-dresden.de
//
// Date         :   Wed Jan 17 09:18:19 2007
// Last Change  :   Wed Jan 17 09:18:19 2007
// Module Name  :   l2_dnc
// Filename     :   l2_dnc.cpp
// Project Name	:   p_facets/s_systemsim
// Description	:    
//
//_____________________________________________

#include "l2_dnc.h"
#include "dnc_tx_fpga.h"
#include "lost_event_logger.h"
#include <sstream>
#include <log4cxx/logger.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("ESS.Layer2");

l2_dnc::l2_dnc (sc_module_name /*l2_dnc_i*/,char wafer,short id, const ESS::dnc_config & config)
	: id(id)
	, wafer(wafer)
	, clk("clk",CLK_PER_L2_DNC,SC_NS)
	{
 		int i;
		char buffer[1000];

		for(i=0;i<DNC_TO_ANC_COUNT;i++)
		{
			snprintf(buffer,sizeof(buffer),"dnc_channel_i%i",i);
			dnc_channel_i[i] = new dnc_ser_channel(buffer);
			dnc_channel_i[i]->set_side(dnc_ser_channel::DNC);
  		}
		dnc_tx_fpga_i = new dnc_tx_fpga("dnc_tx_fpga_i",this);
		dnc_tx_fpga_i->set_side(dnc_tx_fpga::DNC);

		SC_METHOD(fifo_from_anc_ctrl);
		dont_initialize();
		sensitive << clk;
		SC_METHOD(fifo_from_fpga_ctrl);
		dont_initialize();
		sensitive << clk;
		SC_METHOD(delay_mem_ctrl);
		dont_initialize();
		sensitive << clk;
        
		// apply dnc_config
		set_hicann_directions(config.hicann_directions);

		// apply other default config that is not yet included into to dnc_config
		// set time limits to default
        for (unsigned int nchannel=0;nchannel<DNC_TO_ANC_COUNT;++nchannel)
			time_limits[nchannel] = DNC_TIME_LIMIT;
        
        // for debugging: fill routing memory with VOID_ENTRY values
        for (unsigned int nchannel=0;nchannel<DNC_TO_ANC_COUNT;++nchannel)
        for (unsigned int naddress=0;naddress<L2_LABEL_COUNT;++naddress)
        	routing_mem[nchannel][naddress] = 0; // no additional delay
	}
l2_dnc::~l2_dnc()
{
	unsigned int i;
	for(i=0;i<DNC_TO_ANC_COUNT;i++)
	{
		delete dnc_channel_i[i];
	}
	delete dnc_tx_fpga_i;
}


void l2_dnc::transmit_from_anc(const sc_uint<L2_EVENT_WIDTH>& rx_event, int channel)
{
    sc_uint<L2_LABEL_WIDTH> rx_address = (rx_event>>TIMESTAMP_WIDTH) & 0x1ff; // get hicann address
    sc_uint<TIMESTAMP_WIDTH> time = rx_event & 0x7fff; //get neuron address
    sc_uint<HYP_EVENT_WIDTH> event;
    uint sub_address;

    sub_address = routing_mem[channel][rx_address.to_uint()] & 0xffff;

	event = (((channel << 9) + rx_address) << TIMESTAMP_WIDTH) + ((time + (sub_address&0x7fff)) & 0x7fff);

	if (l2direction[(channel*8)+(rx_address.to_uint()>>L1_ADDR_WIDTH)] == 0) {
		dnc_tx_fpga_i->start_event(event);
        LOG4CXX_DEBUG(logger, name() << ": Received event from HICANN: " << channel << " l1bus: " << (rx_address.to_uint()>>L1_ADDR_WIDTH) << " send to FPGA");
		LostEventLogger::count_l2_dnc_transmit_from_anc();
	} else {
        LOG4CXX_WARN(logger, name() << ": Received event from HICANN: " << channel << " l1bus: " << (rx_address.to_uint()>>L1_ADDR_WIDTH) << " that is not configured to send to FPGA");
	}
}


void l2_dnc::transmit_from_fpga(const sc_uint<HYP_EVENT_WIDTH>& rx_event)
{
	sc_uint<L2_LABEL_WIDTH> rx_address = (rx_event>>TIMESTAMP_WIDTH) & 0x1ff; // get hicann and neuron address
	sc_uint<TIMESTAMP_WIDTH> time = rx_event & 0x7fff; //get release time 
	sc_uint<3> channel = (rx_event >> (TIMESTAMP_WIDTH+9))& 0x7; //get target channel
	sc_uint<L2_EVENT_WIDTH> event;

	// check direction
	sc_uint<3> dncif_channel =  rx_address >> 6;
	if (l2direction[ (channel*8)+ dncif_channel.to_uint() ]) {
		event = (rx_address << TIMESTAMP_WIDTH) + time;
		transmit_to_anc(event, channel);
		LostEventLogger::count_l2_dnc_transmit_from_fpga();
	} else {
        LOG4CXX_WARN(logger, name() << ": CANNOT send event to HICANN: " << channel << " l1bus: " << dncif_channel.to_uint() << ", that is not configured to send to HICANN");
	}
}



void l2_dnc::fifo_from_anc_ctrl()
{
	uint value = 0;
	uint64 value_cfg = 0;
	int i;
	sc_uint<L2_EVENT_WIDTH> out;

//		wait(100,SC_PS);
        for(i=0;i<DNC_TO_ANC_COUNT;++i)
		{
			if(dnc_channel_i[i]->fifo_rx_event.num_available())
			{
				dnc_channel_i[i]->fifo_rx_event.nb_read(value);
    			//printf("DNC FROM HICANN ENTER fifo_from_anc_ctrl @ %i: %i %.8X\n",id,i,value);fflush(stdout);
				out = value;
				//cout << "@ " << sc_simulation_time() << "From L1: " << hex << value << " from " << id << "\n";
				transmit_from_anc(out,i);
				LostEventLogger::count_l2_dnc_fifo_from_anc_ctrl();
			} else if(dnc_channel_i[i]->fifo_rx_cfg.num_available())
			{
				dnc_channel_i[i]->fifo_rx_cfg.nb_read(value_cfg);
//    			printf("ENTER fifo_from_anc_ctrl @ %i: %i %.8X\n",id,i,value);fflush(stdout);
				//cout << "@ " << sc_simulation_time() << "From L1: " << hex << value << " from " << id << "\n";
				config_from_anc(i,value_cfg);
			}
		}
}

void l2_dnc::fifo_from_fpga_ctrl()
{
	uint64 value = 0;
	unsigned char target = 0;
	sc_uint<HYP_EVENT_WIDTH> out;
	uint64 cfg_data = 0;
	unsigned char cfg_target = 0;

//		wait(100,SC_PS);
		if(dnc_tx_fpga_i->fifo_rx_event.num_available())
		{
			dnc_tx_fpga_i->fifo_rx_event.nb_read(value);
	   		//printf("ENTER fifo_from_fpga_ctrl @ %i: %.8X\n",id,(uint)value);fflush(stdout);
			out = value;
			//cout << "@ " << sc_simulation_time() << "From FPGA: " << hex << value << "\n";
			transmit_from_fpga(out);
			LostEventLogger::count_l2_dnc_fifo_from_fpga_ctrl();
		} else if(dnc_tx_fpga_i->fifo_rx_cfg.num_available())
		{
			dnc_tx_fpga_i->fifo_rx_cfg.nb_read(value);
			dnc_tx_fpga_i->fifo_rx_cfg_target.nb_read(target);
    		//printf("ENTER fifo_from_fpga_ctrl cfg @ %i: target = %i pkt = %.8X %.8X\n",id,target,(uint)(value>>32),(uint)value);fflush(stdout);
			cfg_data = value;
			cfg_target = target;
			//cout << "@ " << sc_simulation_time() << "From FPGA: " << hex << value << "\n";
			config_from_fpga(cfg_target,cfg_data);
		}
}

void l2_dnc::transmit_to_anc(const sc_uint<L2_EVENT_WIDTH>& rx_event, int channel)
{
//add to memory of channel
	unsigned int value;
	if(delay_mem[channel].insert(rx_event.to_uint())) {
		LostEventLogger::count_l2_dnc_transmit_to_anc();
		// _log(Logger::INFO) << name() << "::transmit_to_anc(" << rx_event.to_uint() << ", " << channel << ") successful";
	}
	else
		LostEventLogger::log(/*downwards*/ true, name());
	delay_mem[channel].view_next(value);
	//printf("l2_dnc::transmit_to_anc at:%i::Add L1 data %.8X at DNC%i to channel: %i  (-> next: %.8X) count: %i\n",(int) sc_simulation_time(),rx_event.to_uint(),id,channel,value,delay_mem[channel].num_available());
	//fflush(stdout);
}

void l2_dnc::delay_mem_ctrl()
{
//new function which runs permanently and compares frontelement of heap with
//	current time + tx_max time
	uint value,val;
	// uint count;
	sc_uint<L2_EVENT_WIDTH> out;
	int i;

	double sim_time = sc_simulation_time();
	unsigned int clock_cycle = (uint)( sim_time/SYSTIME_PERIOD_NS ) & 0x7fff;
	unsigned int clock_cycle_big = clock_cycle >> 5;

	for(i=0;i<DNC_TO_ANC_COUNT;++i)
	{
		if(dnc_channel_i[i]->can_transmit() && !delay_mem[i].empty())
		{
			// check for next entry in heap_memory
			delay_mem[i].view_next(value);

			// OLD
			// if(((((value & 0x7fff)-((((uint)( sim_time/SYSTIME_PERIOD_NS ) & 0x7fff) + (uint) (TIME_TO_DNC_IF/SYSTIME_PERIOD_NS))& 0x7fff)))>>(TIMESTAMP_WIDTH-1)) & 0x1)
			// NEW:
			// uint release_time = (value & 0x7fff);
			// if ( release_time > clock_cycle ? (release_time - clock_cycle < CYCLES_TO_DNC_IF) : 
			//		( release_time + (1 << TIMESTAMP_WIDTH)  - clock_cycle <  CYCLES_TO_DNC_IF) )
			// the line below does exactly the same as the two lines above(BV).
			// if(((((value & 0x7fff)-(clock_cycle + CYCLES_TO_DNC_IF)))>>(TIMESTAMP_WIDTH-1)) & 0x1)
			unsigned int rel_time = (value & 0x7fff);
			unsigned int rel_time_big = (rel_time>>5);
			int diff = rel_time_big - clock_cycle_big;
			if ( rel_time_big < clock_cycle_big )
				diff += (1 << (TIMESTAMP_WIDTH-5));

			/*
			_log(Logger::INFO) << "l2_dnc: delay_mem_ctrl()"
				<< "\t, sim_time=" << sim_time
				<< "\t, clock_cycle=" << clock_cycle
				<< "\t, clock_cycle_big=" << clock_cycle_big
				<< "\t, rel_time=" << rel_time
				<< "\t, rel_time_big=" << rel_time_big
				<< "\t, diff =" << diff;
			*/

			if ( diff == (int) time_limits[i] )
			{
				delay_mem[i].get(value);
				delay_mem[i].view_next(val);
				out = value;
				// We directly trigger the sending of an event, by first starting and then sending it within one cycle
				dnc_channel_i[i]->start_event(out);
				dnc_channel_i[i]->transmit();
				// count=delay_mem[i].num_available();
				LostEventLogger::count_l2_dnc_delay_mem_ctrl();
				//printf("l2_dnc::delay_mem_ctrl@ %fl started to L1 transfer at DNC%i with data %.8X rest data: %i (next-> %.8X)\n",sc_simulation_time(),id,value,count,val);
			}
			else if ( diff < (int) time_limits[i] ) {
			// else if ( diff > (1<<(TIMESTAMP_WIDTH-6)) ) {
				delay_mem[i].get(value);
                LOG4CXX_DEBUG(logger, name() << "expired event dropped"
					<< "\t, sim_time=" << sim_time
					<< "\t, clock_cycle=" << clock_cycle
					<< "\t, clock_cycle_big=" << clock_cycle_big
					<< "\t, rel_time=" << rel_time
					<< "\t, rel_time_big=" << rel_time_big
					<< "\t, diff =" << diff);

				std::stringstream ss;
				ss << name() << ".delay_mem_ctrl(): dropped expired event";
				LostEventLogger::log( /*downwards*/true, ss.str());
			}
		}
	}
}


void l2_dnc::config_from_fpga(unsigned char target, uint64 cfg_data)
{
//    printf("ENTER config_from_fpga @ %i: %.8X %.8X\n",id,(uint)(cfg_data>>32),(uint)cfg_data);fflush(stdout);
	unsigned short address = (cfg_data >> CIP_DATA_WIDTH) & 0xffff;

	if(target < DNC_TO_ANC_COUNT)
	{
		this->routing_mem[target][address] = cfg_data & 0xffffffff;
		//printf("l2_dnc: configuring routing mem: channel 0x%X, address 0x%X = %.4X\n",target, address, routing_mem[target][address]);
	} else
	{
		//printf("ENTER l1route config %llX @ %i\n",cfg_data & 0xffffffffffffffff ,target-DNC_TO_ANC_COUNT);
		if(((cfg_data>>48) & 0xffff) != 0xffff)
		{
			//printf("l1route config %llX @ %i\n",cfg_data & 0xffffffffffffffff ,target-DNC_TO_ANC_COUNT);
			dnc_channel_i[target-DNC_TO_ANC_COUNT]->start_config(cfg_data);
		} else
		{
            LOG4CXX_DEBUG(logger, name() << "ERROR in l1 cfg route " << target << " " << ((uint)cfg_data & 0xffff)); 
		}
	}
//		cout << "Config Done! DNC: " << id << endl;
		//Here: configuration for this DNC
}

void l2_dnc::config_from_anc (int i, uint64 cfg_data)
{
	dnc_tx_fpga_i->start_cfg(i,cfg_data);
//	printf("DNC %i %.4X %.8X \n",id,address,cfg_data & 0xffffffff);
}


void l2_dnc::instant_config(const ess::l2_packet_t& cfg_packet)
{
    LOG4CXX_DEBUG(logger, name()<< "Instant DNC config received:");
    LOG4CXX_DEBUG(logger, "id: " << cfg_packet.id << ", sub_id: " << cfg_packet.sub_id << ", type: " << (int)cfg_packet.type << ", entries: " << (int)cfg_packet.data.size() << " -> ");

	uint64 rt_entry;
	switch(cfg_packet.type)
	{
        case ess::l2_packet_t::DNC_ROUTING:
			for (unsigned int ndata=0;ndata<cfg_packet.data.size();++ndata) {
				rt_entry = cfg_packet.data[ndata];
                unsigned int channel = cfg_packet.sub_id;
                unsigned int address = (rt_entry>>15) & 0x1ff;
				this->routing_mem[channel][address] = rt_entry & 0x7fff;
                LOG4CXX_DEBUG(logger, std::hex << std::uppercase << cfg_packet.data[ndata] << "  -> routing mem: channel 0x" << channel << ", address 0x" << address << " = " << routing_mem[channel][address]);
			}
		break;

        case ess::l2_packet_t::DNC_DIRECTIONS:
			this->l2direction = cfg_packet.data[0];
			for (unsigned int ndata=0;ndata<cfg_packet.data.size();++ndata)
                LOG4CXX_DEBUG(logger, std::hex << cfg_packet.data[ndata] << " " );
		break;

        case ess::l2_packet_t::EMPTY:
		break;


		default:
		break;
	}
}

void l2_dnc::set_time_limits(
		const std::array< sc_uint<10>, DNC_TO_ANC_COUNT > & limits
		)
{
	time_limits = limits;
}

void l2_dnc::set_hicann_directions(const std::bitset<64> & hicann_directions)
{
	l2direction = hicann_directions.to_ullong();
}

