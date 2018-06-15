
///////////////////////////
// class l2_fpga
///////////////////////////

// reads event from file
void l2_fpga::play_tx_event() {
	...
	dnc_tx_fpga_i[dnc_id]->start_event(event);
	...
	}

///////////////////////////
// class dnc_tx_fpga
///////////////////////////

class dnc_tx_fpga : 	public sc_module,
                        public l2_hyp_task_if
{
	sc_fifo<uint64> fifo_tx_event; // depth: fifo_tx_event(CHANNEL_HYP_SIZE_OUT)
	start_event(const sc_uint<HYP_EVENT_WIDTH>& event){
		this->fifo_tx_event.nb_write(event.to_uint()); // TODO: FIFO can be full
	}
}

// is called every posistive clock edge
void dnc_tx_fpga::transmit() {
	if(this->fifo_tx_event.num_available() && !transmit_lock)
	{
		transmit_lock = true;
		fifo_tx_event.nb_read(value);
		transmit_event.notify(TIME_HYP_OUT_PULSE,SC_NS); // trigger stransmit_start_event()
	}
}

void dnc_tx_fpga::transmit_start_event() {
	if(transmit_lock)
	{
		l2_dncfpga_if->receive_event(value); // port(IF) to l2_dnc_i[dnc_id]->dnc_tx_fpga_i): SAME Type, but now the receiver side:
		transmit_lock = false;
	}
}

// THIS is now on the DNC SIDE
void dnc_tx_fpga::receive_event (const sc_uint<HYP_EVENT_WIDTH>& rx_event)
{
    event = rx_event.to_uint64();
    receive_sc_event.notify(); // trigger fill_fifo()
}

void dnc_tx_fpga::fill_fifo()
{
 	if(!fill_fifo_lock)
	{
    	fill_fifo_lock = true;
		event_save = event;
		write_fifo_event.notify(TIME_HYP_IN_PULSE,SC_NS); //triggers write_fifo()
	}
}

void dnc_tx_fpga::write_fifo()
{
	if(fill_fifo_lock)
	{
		this->fifo_rx_event.nb_write(event_save); // TODO: FIFO can be full. depth: fifo_rx_event(CHANNEL_HYP_SIZE_IN)
											      // is read by l2_dnc::fifo_from_fpga_ctrl()
    	fill_fifo_lock = false;
	}
}

///////////////////////////
// class l2_dnc
///////////////////////////

/// Function to continously read from fpga receive fifo and init L2 transfer.
void l2_dnc::fifo_from_fpga_ctrl()
{
	uint64 value;
	unsigned char target;
	sc_uint<HYP_EVENT_WIDTH> out;
	uint64 cfg_data;
	unsigned char cfg_target;

		if(dnc_tx_fpga_i->fifo_rx_event.num_available())
		{
			dnc_tx_fpga_i->fifo_rx_event.nb_read(value);
			out = value;
			transmit_from_fpga(out);
		}
}

/// Function to transmit events from FPGA.
void l2_dnc::transmit_from_fpga(const sc_uint<HYP_EVENT_WIDTH>& rx_event)
{
	sc_uint<L2_LABEL_WIDTH> rx_address = (rx_event>>TIMESTAMP_WIDTH) & 0x1ff; // get hicann and neuron address
	sc_uint<TIMESTAMP_WIDTH> time = rx_event & 0x7fff; //get release time 
	sc_uint<3> channel = (rx_event >> (TIMESTAMP_WIDTH+9))& 0x7; //get target channel
	sc_uint<L2_EVENT_WIDTH> event;

	event = (rx_address << TIMESTAMP_WIDTH) + time;
	transmit_to_anc(event, channel);
}

/// Function delay packets for ANC.
void l2_dnc::transmit_to_anc(const sc_uint<L2_EVENT_WIDTH>& rx_event, int channel)
{
	//add to memory of channel
	delay_mem[channel].insert(rx_event.to_uint()); // TODO: can insert fail? e.g. if full?
}

class l2_dnc {
	//Delay memory downto ANC
	heap_mem<uint> delay_mem[DNC_TO_ANC_COUNT]; ///< delay memory for pulse vents
	dnc_ser_channel *dnc_channel_i[DNC_TO_ANC_COUNT];	///< serial communication channels
}


// triggered every clk("clk",CLK_PER_L2_DNC,SC_NS)
void l2_dnc::delay_mem_ctrl()
{
//new function which runs permanently and compares frontelement of heap with
//	current time + tx_max time
	uint value,val;
	uint count;
	sc_uint<L2_EVENT_WIDTH> out;
	int i;
	double sim_time;

	sim_time = sc_simulation_time();
	for(i=0;i<DNC_TO_ANC_COUNT;++i)
	{
		if(!delay_mem[i].empty())
		{
			// check for next entry in heap_memory
			delay_mem[i].view_next(value);
			if(((((value & 0x7fff)-((((uint)( sim_time/SYSTIME_PERIOD_NS ) & 0x7fff) + (uint) (TIME_TO_DNC_IF/SYSTIME_PERIOD_NS))& 0x7fff)))>>(TIMESTAMP_WIDTH-1)) & 0x1)
			{
				delay_mem[i].get(value);
				delay_mem[i].view_next(val);
				out = value;
				dnc_channel_i[i]->start_event(out); // HERE
				count=delay_mem[i].num_available();
				//printf("l2_dnc::delay_mem_ctrl@ %fl started to L1 transfer at DNC%i with data %.8X rest data: %i (next-> %.8X)\n",sc_simulation_time(),id,value,count,val);
			}
		}
	}
}

///////////////////////////
// class dnc_ser_channel
///////////////////////////

/// Access for transmitter for pulse event transmission.
void dnc_ser_channel::start_event(const sc_uint<L2_EVENT_WIDTH>& event)
{
 	this->heap_tx_mem.insert(event.to_uint()); // TODO: this might be full
}


class dnc_ser_channel {
	heap_mem<uint> heap_tx_mem; 	///< transmit heap mem for pulse events
}

/// Function to transmit events to network.
/// If data in tx_fifo and if SERDES time is reached.
/// Checks permanently the status of the transmit memories. 
/// If data is available in config or pulse event FIFO, 
/// it reads FIFO and generates event for transmission delayed by serialization time.
/// TRIGGERED by 
// sensitive_pos << clk : clk("clk",CLK_PER_DNC_SER_CHANNEL,SC_NS)
void dnc_ser_channel::transmit()
{
	uint data;
	if(!this->heap_tx_mem.empty() && !transmit_lock)
	{
		transmit_lock = true;
		heap_tx_mem.get(data);
		value = (uint64)data;
		transmit_event.notify(TIME_SER_PULSE,SC_NS); // triggers transmit_start_event()
	}
}

/// Transmits pulse event by accessing TBV port. 
void dnc_ser_channel::transmit_start_event()
{
	if(transmit_lock)
	{
		l2_dnc_if->receive_event((uint)value); // port do dnc_if of HICANN: l2_dnc_if( *(cur_hicann -> dnc_if_i->dnc_channel_i)
		transmit_lock = false;
		LostEventLogger::count_dnc_tx_fpga_transmit_start_event();
	}
}
// SAME CLASS, but now receiver side on dnc_if

/// Function to get events from network.
/// This function implements l2_dnc_task_if.
/// It is accessed by sc_port of another dnc_ser_channel in a DNC or a DNC_IF.
/// It receives a pulse events and saves it temporarally. 
/// In parallel it generates an event for further event handling.
void dnc_ser_channel::receive_event (const sc_uint<L2_EVENT_WIDTH>& rx_event)
{
    event = rx_event.to_uint(); // TODO: we might overwrite an existing event here!
    receive_sc_event.notify(); // triggers fill_fifo()
}

/// Waits for deserialisation time and generates event for FIFO-write.
/// It handles pulse event FIFO write.
void dnc_ser_channel::fill_fifo()
{
	if(!fill_fifo_lock)
	{
		fill_fifo_lock = true;
    	event_save = event;
		write_fifo_event.notify(TIME_DES_PULSE,SC_NS); // triggers write_fifo()
	}
}


/// It writes pulse events to receive FIFO.
void dnc_ser_channel::write_fifo()
{
	if(fill_fifo_lock)
	{
		this->fifo_rx_event.nb_write(event_save); // TODO: pulse might get lost, this FIFO is checked by dnc_if::rx_l2_ctrl()
		fill_fifo_lock = false;
	}
}

//////////////////////////////7
// class dnc_if
//////////////////////////////7

class dnc_if {
	dnc_ser_channel *dnc_channel_i; ///< layer2 communication channel
	l2tol1_tx *l2tol1_tx_i[DNCL1BUSINCOUNT]; ///< l2tol1_tx instance
}

///Function to handle received l2 data.
///activated ech clk cycle and checks receive fifo
void dnc_if::rx_l2_ctrl()
{

	if(this->dnc_channel_i->fifo_rx_event.num_available())
	{
		uint value;
		sc_uint<L2_EVENT_WIDTH> out;

		this->dnc_channel_i->fifo_rx_event.nb_read(value);
		out = value;
		this->receive_event(out);
	}
}

/// external interface for l2 packet receive.
/// upper bits of packets selects layer1 channel
/// @param l1event: l2 packet with [L1_Bus_nr + Neuron_nr + Timestamp]
void dnc_if::receive_event(const sc_uint<L2_EVENT_WIDTH>& l1event)
{
	// check here if l1_bus is configured in direction DNC->L1
    if(l1direction[l1event.to_uint() >> (TIMESTAMP_WIDTH+L1_ADDR_WIDTH)]==0){
		l2tol1_tx_i[l1event.to_uint() >> (TIMESTAMP_WIDTH+L1_ADDR_WIDTH)]->transmit(l1event & 0x3ffffff); // Here it continues
	}
	else {
		char buffer[256];
		sprintf(buffer, "DNC_IF: WARNING::%X :\t received invalid DNC event @ HICANN %i DNC_IF %.8X in channel %i: l1direction is set to TOWARDS_DNC!\n",(unsigned int)sc_simulation_time(), hicannid, l1event.to_uint(),(l1event.to_uint() >> (TIMESTAMP_WIDTH+L1_ADDR_WIDTH)));
		log(Logger::WARNING) << buffer;
	}
}


//////////////////////////////7
// class l2tol1_tx
//////////////////////////////7

/// Gets received data from dnc_if and adds it into buffers.
/// External interface!
/// @param data_in: event containing timestamp and neuron number
void l2tol1_tx::transmit(sc_uint <TIMESTAMP_WIDTH+L1_ADDR_WIDTH> data_in)
{
	buffer = data_in;
	rx_data.notify(TIME_DES_PULSE,SC_NS); // triggers add_buffer()
}

/// Adds received data into buffer. 
void l2tol1_tx::add_buffer()
{
    for(i=0;i<DNC_IF_2_L2_BUFFERSIZE;i++)
	{
		if(memory[i].valid == false)
		{
            memory[i].valid = true;
			memory[i].value = buffer; // memory checked by check_time()
			return;
		}
		// TODO: PULSE can get lost if nothing free
	}
}

/// Compare recv time stamp with current sim time.
/// continously running process that compares 
/// received timestamps with current simulation time
/// reinvoked with help of clock
//  sensitive << clk("clk",CLK_PER_L2TOL1_TX,SC_NS)
void l2tol1_tx::check_time()
{
	sim_time = sc_simulation_time();	// sc_simulation_time returns time as a double in SC_NS(per default, comp. sc_get_default_time_unit
	uint current_clock_cycle = (uint)(sim_time/SYSTIME_PERIOD_NS) & 0x7fff;
	for(j=0;j<DNC_IF_2_L2_BUFFERSIZE;++j)
	{
		if(memory[j].valid == true)
		{
			if((memory[j].value & 0x7fff) <= current_clock_cycle )
			{
				memory[j].valid = false;
				serialize(memory[j].value >> TIMESTAMP_WIDTH);
			}
		}
	}
}

/// Serializes event at release time.
/// @param neuron: Neuron number of event
void l2tol1_tx::serialize(sc_uint <L1_ADDR_WIDTH> neuron)
{
	l1bus_tx_if->rcv_pulse_from_dnc_if_channel(neuron, channel_id);
}



//////////////////////////////////////////////////
//////////////////////////////////////////////////
//////////////////////////////////////////////////
//  UPSTREAM 
//    UPSTREAM 
//      UPSTREAM 
//        UPSTREAM 
//          UPSTREAM 
//            UPSTREAM 
//              UPSTREAM 
//                UPSTREAM 
//                  UPSTREAM 
//                    UPSTREAM 
//                      UPSTREAM 
//                        UPSTREAM 
//                          UPSTREAM 
//                            UPSTREAM 
//                              UPSTREAM 
//                                UPSTREAM 
//                                  UPSTREAM 
//                                    UPSTREAM 
//                                      UPSTREAM 
//////////////////////////////////////////////////
//////////////////////////////////////////////////
//////////////////////////////////////////////////
//////////////////////////////////////////////////



void dnc_if::l1_task_if(const unsigned char& channel, const unsigned char& nrnid)
{
    this->sim_time=sc_simulation_time();
	this->nrnid = (channel*(1 << L1_ADDR_WIDTH)) + nrnid;
	if (l1direction[channel])		//checks for transmission of own neuron busses
	{
		pulse_event.notify(); // triggers start_pulse_dnc
		log(Logger::DEBUG1) << "DNC_IF: Start Layer1 Pulse L1->DNC: " << (uint) nrnid << " with " << (uint)sim_time << " " << hicannid << " @ " << transmit_l1bus  << "(" << (((nrnid >> 6) & 0xf) < 8) << ")." << endl;
	} else {
		log(Logger::WARNING) << "DNC_IF: StoppedLayer1 Pulse L1->DNC: " << (uint) nrnid << " with " << (uint)sim_time << " " << hicannid << " @ " << transmit_l1bus  << "(" << (((nrnid >> 6) & 0xf) < 8) << "). Direction of this Bus is set towards HICANN" << endl;
	}
}

/// Sends a pulse to the DNC.
void dnc_if::start_pulse_dnc()
{
	// divide simtime by 4 to get system clock cycles
	dnc_channel_i -> start_event(((nrnid & 0xfff)<<TIMESTAMP_WIDTH) + (((uint)(sim_time)>>2) & 0x7fff));
	pulse2dnc_finish.notify(); // notify potentially waiting OCPResponseProcess
}

/// dnc_ser_channel
//
/// DNC_IF side:
/// start_event()
/// transmit()
/// transmit_start_event()
//
/// DNC side
/// receive_event()
/// fill_fifo()
/// write_fifo()

///////////////////
////DNC
///////////////////

/// Function to continously read from anc receive fifo and init L2 transfer.
void l2_dnc::fifo_from_anc_ctrl()
{
	uint value;
	uint64 value_cfg;
	int i;
	sc_uint<L2_EVENT_WIDTH> out;
        for(i=0;i<DNC_TO_ANC_COUNT;++i)
		{
			if(dnc_channel_i[i]->fifo_rx_event.num_available())
			{
				dnc_channel_i[i]->fifo_rx_event.nb_read(value);
				out = value;
				transmit_from_anc(out,i);
			}
		}
}

/// Function to transmit events from ANC.
void l2_dnc::transmit_from_anc(const sc_uint<L2_EVENT_WIDTH>& rx_event, int channel)
{
    //printf("ENTER transmit_from_anc @ %i\n",id);      fflush(stdout);
    sc_uint<L2_LABEL_WIDTH> rx_address = (rx_event>>TIMESTAMP_WIDTH) & 0x1ff; // get hicann address
    sc_uint<TIMESTAMP_WIDTH> time = rx_event & 0x7fff; //get neuron address
    sc_uint<HYP_EVENT_WIDTH> event;
    uint sub_address;
  
    sub_address = routing_mem[channel][rx_address.to_uint()] & 0xffff;
	//printf("  -> accessing routing mem at channel 0x%X, address 0x%X -> result: 0x%04X\n",channel,rx_address.to_uint(),sub_address);

	//printf("  -> @ %.X send at DNC %i to Address 0x%04X\n",(short)sc_simulation_time(),id,sub_address);fflush(stdout);
	event = (((channel << 9) + rx_address) << TIMESTAMP_WIDTH) + ((time + (sub_address&0x7fff)) & 0x7fff);

	if (l2direction[(channel*8)+(rx_address.to_uint()>>L1_ADDR_WIDTH)] == 0) {
		dnc_tx_fpga_i->start_event(event);
	} else {
		//printf("L2_DNC %i: Received event from HICANN: %i l1bus: %i that is not configured to send to l2\n",id,channel,(rx_address.to_uint()>>L1_ADDR_WIDTH));fflush(stdout);
		_log(Logger::ERROR) << "L2_DNC " << id << ": Received event from HICANN: " << channel << " l1bus: " << (rx_address.to_uint()>>L1_ADDR_WIDTH) << " that is not configured to send to l2";
	}
}


/// dnc_tx_fpga
///
/// DNC side
/// start_event()
/// transmit()
/// transmit_start_event()
///
/// FPGA side
/// receive_event()
/// fill_fifo()
/// write_fifo()



/// checks l2 channel for received data.
void l2_fpga::fifo_from_l2_ctrl()
{
	uint64 value;
	sc_uint<HYP_EVENT_WIDTH> out;
	unsigned char target;
	int i;

	for(i=0;i<DNC_FPGA;++i)
	{
		if(dnc_tx_fpga_i[i]->fifo_rx_event.num_available())
		{
			dnc_tx_fpga_i[i]->fifo_rx_event.nb_read(value);
			out = value;
            //printf("FPGA: fifo_from_l2_ctrl: read packet 0x%016llX\n",value);
			transmit_l2_event(i,out);
		}
	}
}

/// Routing from L2 Bus to other wafer.
void l2_fpga::transmit_l2_event(const int dnc_id, const sc_uint<HYP_EVENT_WIDTH>& rx_event)
{
	sc_uint<FPGA_LABEL_WIDTH> rx_address = (dnc_id << L2_LABEL_WIDTH) | ((rx_event>>TIMESTAMP_WIDTH) & 0xfff); // get hicann address
	sc_uint<TIMESTAMP_WIDTH> time = rx_event & 0x7fff; //get neuron address
	sc_uint<HYP_EVENT_WIDTH> event;
	uint64 sub_address;
	bool valid;
	bool trace;
	bool route;
	unsigned char channel;
	uint64 new_label;
	uint64 time_delta;

    for(int i=0;i<4;i++)
	{
		sub_address = routing_mem[WAFER_COUNT-1][rx_address.to_uint()][i] & 0x1fffffffffULL;
		
		//printf("FPGA: RX_L2_EVENT: %.4X  time = %.4X  routing data = 0x%.016llX\n",rx_address.to_uint(),time.to_uint(),sub_address);

		valid = (sub_address >> 34) & 0x1;
		route = (sub_address >> 33) & 0x1;
		trace = (sub_address >> 32) & 0x1;
		channel = (sub_address >> 28) & 0xf;
		new_label = (sub_address >> 14) & 0x3fff;
		time_delta = (sub_address) & 0x3fff;
		
		if (valid) {
		    if(trace) {
				this->record_rx_event(dnc_id,rx_event);
			}
		}
	}
}

///Trace memory for received pulses
void l2_fpga::record_rx_event(const int dnc_id, const sc_uint<HYP_EVENT_WIDTH>& event) 
{
	if (pulse_file_rx)
	{	
		uint pulse_label = (dnc_id << L2_LABEL_WIDTH) | ((event.to_uint() >> TIMESTAMP_WIDTH) & 0xfff);

		//printf("l2_fpga::record_rx_event: pulse from DNC 0x%X, event 0x%08X -> label: 0x%04X (=%d)\n",dnc_id,event.to_uint(),pulse_label,pulse_label);

	    fprintf(pulse_file_rx,"%.8X %.4X %.4X\n",(uint)sc_simulation_time(), pulse_label, event.to_uint() & 0x7fff);
	    fflush(pulse_file_rx);
	}
	else
	{
		_log(Logger::DEBUG1) << "No open pulse record file found in fpga_" << wafer_id <<"_"<<id;
	}

}
