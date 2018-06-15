//_____________________________________________
// Company      :	kip			      	
// Author       :	agruebl			
// E-Mail   	:	<email>					
//								
// Date         :   03.04.2008				
// Last Change  :   03.04.2008			
// Module Name  :   --					
// Filename     :   types.h
// Project Name	:   p_facets/s_systemsim				
// Description	:   global type definitions for facets system simulation
//								
//_____________________________________________


#ifndef TYPES_H
#define TYPES_H

#ifdef USE_SCTYPES

#ifdef NCSIM   // !!! without this macro the required DPI extensions won't be included !!!
#define NCSC_INCLUDE_TASK_CALLS
#endif  // NCSIM

#include "systemc.h"
#endif  // USE_SCTYPES

#include "sim_def.h"

#include <vector>
#include <string>
#include <sstream>
#include <map>

#ifndef USE_SCTYPES
typedef unsigned long long uint64;
#endif

namespace ess {

/// OCP data and address types
typedef uint64 OCPDataType;
typedef unsigned int OCPAddrType;


/// Data types for communication with control modules
#ifdef USE_SCTYPES
typedef	sc_uint<CIP_WRITE_WIDTH> ci_write_t;
typedef sc_uint<CIP_DATA_WIDTH> ci_data_t;
typedef sc_uint<CIP_ADDR_WIDTH> ci_addr_t;
#else
typedef uint ci_write_t;
typedef uint ci_data_t;
typedef uint ci_addr_t;
#endif


///< type of data payload transported via communication medium between host and Stage 2 hardware.
enum ptype {p_empty=0,p_ci=1,p_event=2,p_layer2=3}; 
///< type of sideband data
enum sbtype {s_empty=0,s_dac=1,s_adc=2};


class TCommObj {
public:

	///< virtual methods for serialization and deserialization.
	///< must be implemented by derived classes
	virtual void serialize(char* buffer, bool verbose=false)=0; ///< give start pointer to char array
	virtual void deserialize(const char* buffer)=0; ///< give start pointer to char array

	virtual void clear(void)=0;
	
	virtual ~TCommObj(){};
};

/**
##########################################################################################
####           start section containing definitions corresponding to:                 ####
####          *  s_hicann1/units/hicann_top/hicann_base.svh                           ####
####          *  s_hicann1/units/hicann_top/hicann_pkg.svh                            ####
##########################################################################################
*/

/// Content of HICANN control interface packet
class ci_packet_t : virtual public TCommObj {	
public:

#ifdef USE_SCTYPES
	sc_uint<CIP_TAGID_WIDTH> TagID;
	sc_uint<CIP_SEQV_WIDTH>  SeqValid;
	sc_uint<CIP_SEQ_WIDTH>   Seq;
	sc_uint<CIP_ACK_WIDTH>   Ack;
#else
	uint TagID;
	uint SeqValid;
	uint Seq;
	uint Ack;
#endif
	ci_write_t Write;
	ci_addr_t Addr;
	ci_data_t Data;

	ci_packet_t() { clear(); };

	ci_packet_t(
#ifdef USE_SCTYPES
		sc_uint<CIP_TAGID_WIDTH> g,
		sc_uint<CIP_SEQV_WIDTH>  v,
		sc_uint<CIP_SEQ_WIDTH>   s,
		sc_uint<CIP_ACK_WIDTH>   k,
		sc_uint<CIP_WRITE_WIDTH> w,
#else
		uint g,
		uint v,
		uint s,
		uint k,
		uint w,
#endif
		ci_addr_t a,
		ci_data_t d)
		: TagID(g), SeqValid(v), Seq(s), Ack(k), Write(w), Addr(a), Data(d){};

	explicit ci_packet_t(
#ifdef USE_SCTYPES
		sc_uint<CIP_PACKET_WIDTH> p
#else
		uint64 p
#endif
  ) {
		*this = p; // use = operator to convert
	}

	virtual ~ci_packet_t();

	inline virtual void clear(void){
		TagID    = 0;
		SeqValid = 0;
		Seq      = 0;
		Ack      = -1;
		Write    = 0;
		Addr     = 0;
		Data     = 0;
	};

	virtual void serialize(char* buffer, bool verbose=false);
	virtual void deserialize(const char* buffer);

	// convert ci_packet_t to uint64
	uint64 to_uint64();

	// convert uint64 to ci_packet
	ci_packet_t & operator=(const uint64 & d);
};


//stream ci_packet_t structs
std::ostream & operator<<(std::ostream &o,const ci_packet_t & d);
std::ostringstream & operator<<(std::ostringstream &o,const ci_packet_t & d);


// Content of DNC/FPGA configuration packets


class l2_packet_t
{
	public:
		// type of payload
		enum l2_type_t {EMPTY,DNC_ROUTING,DNC_DIRECTIONS,FPGA_ROUTING} type;

		// id of element (DNC or FPGA), max. 16bit (when serialized)
		unsigned int id;

		// sub-id of element (for DNC: HICANN channel), max. 16bit (when serialized)
		unsigned int sub_id;

		// vector of data elements
		std::vector<uint64> data;


		l2_packet_t() : type(EMPTY), id(0), sub_id(0)
		{}

		l2_packet_t(l2_type_t set_type, unsigned int set_id, unsigned int set_sub)
			       : type(set_type), id(set_id), sub_id(set_sub)
		{}

		l2_packet_t(l2_type_t set_type, unsigned int set_id, unsigned int set_sub, std::vector<uint64> set_data)
			       : type(set_type), id(set_id), sub_id(set_sub), data(set_data)
		{}

		l2_packet_t(l2_type_t set_type, unsigned int set_id, unsigned int set_sub, uint64 set_entry)
			       : type(set_type), id(set_id), sub_id(set_sub)
		{
			data.push_back(set_entry);
		}

		~l2_packet_t()
		{}
	public:
		std::string serialize();
		bool deserialize(std::string instr);

};



/**
##########################################################################################
####             end section containing definitions corresponding to:  ...            ####
##########################################################################################
*/


/** Layer1 pulse packet with debug information
    The minimum content of an L1 pulse packet is the destination bus id and the transferred L1 address. */
class l1_pulse {
public:
#ifdef USE_SCTYPES
	sc_uint<L1_TARGET_BUS_ID_WIDTH> bus_id;  /// target L1 bus_id before crossbar switch
	sc_uint<L1_ADDR_WIDTH> addr;             /// L1 address transferred in L1 packet.
#else
	uint bus_id;
	uint addr;
#endif
	
	uint  source_nrn;   /// logical (global no.) source neuron (origin of packet), for debugging

	l1_pulse():bus_id(0),addr(0){};
	~l1_pulse(){};
};


/** Layer2 pulse packet with debug information.
    The minimum content of an L2 pulse packet is an address and a time stamp. If the packet is
		sent to HICANN (target event), the address denotes the  */
class l2_pulse : virtual public TCommObj {
public:
#ifdef USE_SCTYPES
	sc_uint<FPGA_LABEL_WIDTH> addr;  /// target/source address within HICANN. Address may be specified in more detail, later.
	sc_uint<32> time;  /// target/source time stamp -> make it big enough to avoid timestamp overflow
#else
	uint addr;
	uint time;
#endif
	
	l2_pulse() : addr(0),time(0){};
	l2_pulse(uint addr, uint time) : addr(addr),time(time){};
	virtual ~l2_pulse();
	
	virtual void serialize(char* buffer, bool verbose=false);
	virtual void deserialize(const char* buffer);

	inline virtual void clear(void) {
		addr = 0;
		time = 0;
	};
	
	// functions for sorting by time
	bool operator==(const l2_pulse &p) const {return ((this->addr==p.addr)&&(this->time==p.time)); }
	bool operator<(const l2_pulse &p) const
	{
		if (this->time == p.time)
			return (this->addr < p.addr);
			
		return (this->time < p.time);
	}
};



/**  #### encapsulation of the information exchange between host software and stage 2 hardware  ####

 two classes of data: IData:  anything that is transported to/via FPGA / DNC / HICANN
											SBData:	sideband data (data like transported parallel to the lvds bus between host and spikey)
 
*/

///< Internal representation of the data exchanged with the Stage 2 hardware. This data structure
///< is to be transported over the communication medium (e.g. TCP/IP socket)
class IData : virtual public TCommObj {
private:
	ci_packet_t    _data;   ///< control interface data payload
	l2_pulse       _event;  ///< Layer 2 pulse event payload
	l2_packet_t    _l2data; ///< Layer 2 configuration data payload
	/* add other payload structs when implemented */
	
	uint           _addr;   /** defines target address depending on type: p_ci:    global HICANN address
	                                                                      p_event: global HICANN address
																																				p_dnc:   global DNC address
																																				p_fpga:  global FPGA address*/
	ptype _payload; ///< type of payload

public:

	/// Constructor and initializer functions
	IData();
	IData(ci_packet_t cip);
	IData(l2_pulse l2p);
	IData(l2_packet_t l2data);
	virtual ~IData();
	
	bool isEmpty() const {return payload()==p_empty;};

	///< access to private members
	uint addr(void)			const {return _addr;};
	uint & setAddr(void)			{return _addr;};
	ptype payload(void)			const {return _payload;};
	ptype & setPayload(void)  		{return _payload;};

	///< access to ci command packet
	ci_packet_t cmd(void)			const {return _data;};
	ci_packet_t & setCmd(void)			  {_payload=p_ci; return _data;};
	bool isCI(void)								const {return payload()==p_ci;};

	///< access to layer2 configuration packet
	l2_packet_t l2data(void)			const {return _l2data;};
	l2_packet_t & setL2data(void)			  {_payload=p_layer2; return _l2data;};
	bool isL2data(void)								const {return payload()==p_layer2;};

	///< access to event daty payload
	l2_pulse event(void)			const {return _event;};
	l2_pulse & setEvent(void)				{_payload=p_event; return _event;};
	bool isEvent(void)				const {return payload()==p_event;};

	virtual void serialize(char* buffer, bool verbose=false);
	virtual void deserialize(const char* buffer);

	inline virtual void clear(void) {
		_payload=p_empty;
		_addr=0;
	};
};

///< sideband data -> implements external control voltages etc.
class SBData {
public:
	enum ADtype {irefdac};//vout0/4 adc readback of out

private:	
	sbtype _payload;
	ADtype _adtype;
	float _advalue;
	
public:
	SBData():_payload(s_empty){};	//default constructor

	sbtype payload(void)			const {return _payload;};
	sbtype & setPayload(void)	  		{return _payload;};

	//dacs
	bool isDac(void) const {return payload()==s_dac;};
	bool isAdc(void) const {return payload()==s_adc;};

	void setDac(ADtype type,float value){
		setPayload()=s_dac;
		_advalue=value;
		_adtype=type;
	};
			
	void setAdc(ADtype type){
		setPayload()=s_adc;
		_adtype=type;
	};
	
	void setADvalue(float v){_advalue=v;};
	float ADvalue() const {return _advalue;};

	ADtype getADtype() const {return _adtype;};
};

//stream idata structs
std::ostream & operator<<(std::ostream &o,const SBData & d);
//stream sbdata structs
std::ostream & operator<<(std::ostream &o,const IData & d);

}	// namespace ess

#endif // TYPES_H

