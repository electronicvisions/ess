//_____________________________________________
// Company      :	kip			      	
// Author       :	agruebl			
// E-Mail   	:	<email>					
//								
// Date         :   03.04.2008				
// Last Change  :   03.04.2008			
// Module Name  :   --					
// Filename     :   types.cpp
// Project Name	:   p_facets/s_systemsim				
// Description	:   global type definitions for facets system simulation
//								
//_____________________________________________

#include <string>
#include <iostream>

#include "types.h"

using namespace std;
using namespace ess;

namespace ess {

//_____________________________________________
// ci_packet_t
//_____________________________________________


///< ci_packet_t destructor
ci_packet_t::~ci_packet_t(){}

/// stream ci_packet_t structs
std::ostream & operator<<(std::ostream &o,const ci_packet_t & d){
	return o<<"TagID: "<<d.TagID<<", SeqValid: "<<d.SeqValid<<", Seq: "<<d.Seq<<", Ack: "<<d.Ack<<", Write: "<<d.Write<<", Addr: "<<d.Addr<<", Data: "<<d.Data;
}

/** Output string without special formatting. Is used for TCP/IP transfer */
std::ostringstream & operator<<(std::ostringstream &o,const ci_packet_t & d){
	o << d.TagID << d.SeqValid << d.Seq << d.Ack << d.Write << d.Addr << d.Data;
	return o;
}

uint64 ci_packet_t::to_uint64(){
	uint64 left=
#ifdef USE_SCTYPES
		  (((uint64)this->TagID.to_uint()    & (((uint64)1 << CIP_TAGID_WIDTH) -1)) << CIP_TAGID_POS)
		| (((uint64)this->SeqValid.to_uint() & (((uint64)1 << CIP_SEQV_WIDTH) -1))  << CIP_SEQV_POS)
	  | (((uint64)this->Seq.to_uint()      & (((uint64)1 << CIP_SEQ_WIDTH) -1))   << CIP_SEQ_POS)
		| (((uint64)this->Ack.to_uint()      & (((uint64)1 << CIP_ACK_WIDTH) -1))   << CIP_ACK_POS)
		| (((uint64)this->Write.to_uint()    & (((uint64)1 << CIP_WRITE_WIDTH) -1)) << CIP_WRITE_POS)
		| (((uint64)this->Addr.to_uint()     & (((uint64)1 << CIP_ADDR_WIDTH) -1))  << CIP_ADDR_POS)
		| (((uint64)this->Data.to_uint()     & (((uint64)1 << CIP_DATA_WIDTH) -1))  << CIP_DATA_POS);
#else
		  (((uint64)this->TagID    & (((uint64)1 << CIP_TAGID_WIDTH) -1)) << CIP_TAGID_POS)
		| (((uint64)this->SeqValid & (((uint64)1 << CIP_SEQV_WIDTH) -1))  << CIP_SEQV_POS)
	  | (((uint64)this->Seq      & (((uint64)1 << CIP_SEQ_WIDTH) -1))   << CIP_SEQ_POS)
		| (((uint64)this->Ack      & (((uint64)1 << CIP_ACK_WIDTH) -1))   << CIP_ACK_POS)
		| (((uint64)this->Write    & (((uint64)1 << CIP_WRITE_WIDTH) -1)) << CIP_WRITE_POS)
		| (((uint64)this->Addr     & (((uint64)1 << CIP_ADDR_WIDTH) -1))  << CIP_ADDR_POS)
		| (((uint64)this->Data     & (((uint64)1 << CIP_DATA_WIDTH) -1))  << CIP_DATA_POS);
#endif
	return left;
}

ci_packet_t & ci_packet_t::operator = (const uint64 & d) {
	(*this).TagID    = (d >> CIP_TAGID_POS) & (((uint64)1 << CIP_TAGID_WIDTH)-1);
	(*this).SeqValid = (d >> CIP_SEQV_POS)  & (((uint64)1 << CIP_SEQV_WIDTH)-1);
	(*this).Seq      = (d >> CIP_SEQ_POS)   & (((uint64)1 << CIP_SEQ_WIDTH)-1);
	(*this).Ack      = (d >> CIP_ACK_POS)   & (((uint64)1 << CIP_ACK_WIDTH)-1);
	(*this).Write    = (d >> CIP_WRITE_POS) & (((uint64)1 << CIP_WRITE_WIDTH)-1);
	(*this).Addr     = (d >> CIP_ADDR_POS)  & (((uint64)1 << CIP_ADDR_WIDTH)-1);
	(*this).Data     = (d >> CIP_DATA_POS)  & (((uint64)1 << CIP_DATA_WIDTH)-1);
	return *this;
}

/** Serialize ci_packet_t. This could also be done by directly shifting the data
into an uint64, which is omitted for seamless extensibility.
The total size of the character array is stored at its beginning. The size of the size 
identifier is defined by */
void ci_packet_t::serialize(char* buffer, bool verbose){
	//ci_packet_t tmp = *this;
	uint size;
	uint i,pos=0;
	uint64 tu = to_uint64();

	if(verbose){
		size = CIP_SIZE_VERB;
		std::cout << "Verbose serialization of ci_packet_t not yet implemented!" << std::endl;
		//assert(false);
	}else{
		size = CIP_SIZE_REG;
		for(i=0;   i<IDATA_PAYL_SIZE;         i++, size>>= 8, pos++) buffer[pos] = size & 0xff;
		for(i=pos; i<(CIP_PACKET_WIDTH+7)/8;  i++, pos++)            buffer[pos] = tu & 0xff;
	}
}

/** Deserialize ci_packet_t. Information is extracted in the order specified in ci_packet_t::serialize() */
void ci_packet_t::deserialize(const char* buffer){
	this->clear();
	uint i,pos=0;
	uint size=0;
	uint64 tmp=0;

	for(i=0; i<IDATA_PAYL_SIZE; i++, pos++) size += ((buffer[pos] & 0xff) << (8*i));

	if(size > CIP_SIZE_REG){  // verbose stream
		std::cout << "Verbose deserialization of ci_packet_t not yet implemented!" << std::endl;
		//assert(false);
	}else{					// standard length
		for(i=0; i<(CIP_PACKET_WIDTH+7)/8;  i++, pos++) tmp += ((buffer[pos] & 0xff) << (8*i));

		*this = tmp;
	}
}



//_____________________________________________
// l2_packet_t
//_____________________________________________


std::string l2_packet_t::serialize()
{
	std::string outstr;
	
	// write destination ID and sub-ID (make use of sequence number)
	outstr.push_back( (unsigned char)((this->id >> 8 ) % (1<<8)) );
	outstr.push_back( (unsigned char)((this->id) % (1<<8)) );
	outstr.push_back( (unsigned char)((this->sub_id >> 8 ) % (1<<8)) );
	outstr.push_back( (unsigned char)((this->sub_id) % (1<<8)) );


	// write packet header (type)
	unsigned int entry_width = 64; // width of one data entry
	if (this->type == DNC_ROUTING)
	{
		outstr.push_back((unsigned char)0x1d);
		outstr.push_back((unsigned char)0x1a);
		entry_width = 24;
	}
	else if (this->type == FPGA_ROUTING)
	{
		outstr.push_back((unsigned char)0x0c);
		outstr.push_back((unsigned char)0xaa);
		entry_width = 47;
	}
	else if (this->type == DNC_DIRECTIONS)
	{
		outstr.push_back((unsigned char)0x0e);
		outstr.push_back((unsigned char)0x55);
		entry_width = 64;
	}
	else // should not occur
	{
		outstr.push_back('?');
		outstr.push_back('?');
		entry_width = 64;
	}

	unsigned char curr_char = 0;
	unsigned int curr_char_pos = 0;
	
	for (unsigned int ndata = 0;ndata < this->data.size(); ++ndata )
	{
		//printf("l2_packet_t::serialize: curr. entry: %llx\n",this->data[ndata]);
	
		uint64 curr_data = this->data[ndata];
		unsigned int remaining_bits = entry_width;
		while (remaining_bits >= 8-curr_char_pos) // still a whole byte to fill
		{
			unsigned int add_bits = 8-curr_char_pos;
			curr_char |= ((curr_data>>(remaining_bits-add_bits))%(1<<add_bits)) << curr_char_pos;
			//printf("Full char: %x, add_bits: %d, remaining_bits: %d\n",curr_char,add_bits,remaining_bits);
			outstr.push_back(curr_char);
			curr_char = 0;
			curr_char_pos = 0;
			remaining_bits -= add_bits;
		}
		
		if (remaining_bits > 0)
		{
			curr_char = curr_data % remaining_bits;
			curr_char_pos = remaining_bits;
		}
		
	}
	
	if (curr_char_pos > 0)
		outstr.push_back(curr_char);
		

	printf("l2_packet_t::serialize: string length: %d, data entries: %d\n",(unsigned int)outstr.size(),(unsigned int)this->data.size());

	return outstr;
}


bool l2_packet_t::deserialize(std::string instr)
{
	if (instr.length() <= 6) // minimum: 4Bytes ID, 2Bytes Header
	{
		printf("l2_packet_t::deserialize: String to deserialize is too short (%d<%d). Ignore.\n",(unsigned int)instr.length(),6);
		return false;
	}


	// read destination ID
	this->id = 0;
	this->id += (unsigned int)((unsigned char)instr[0]) << 8;
	this->id += (unsigned int)((unsigned char)instr[1]);

	this->sub_id = 0;
	this->sub_id += (unsigned int)((unsigned char)instr[2]) << 8;
	this->sub_id += (unsigned int)((unsigned char)instr[3]);
	

	// read packet header (type)
	unsigned int entry_width = 64; // width of one data entry
	if ( ((unsigned char)instr[4] == 0x1d) && ((unsigned char)instr[5] == 0x1a) )
	{
		this->type = DNC_ROUTING;
		entry_width = 24;
	}
	else if ( ((unsigned char)instr[4] == 0x0c) && ((unsigned char)instr[5] == 0xaa) )
	{
		this->type = FPGA_ROUTING;
		entry_width = 47;
	}
	else if ( ((unsigned char)instr[4] == 0x0e) && ((unsigned char)instr[5] == 0x55) )
	{
		this->type = DNC_DIRECTIONS;
		entry_width = 64;
	}
	else // should not occur
	{
		this->type = EMPTY;
		printf("l2_packet_t::deserialize: Unknown packet header (%d %d). Ignore.\n",(unsigned int)((unsigned char)instr[4]),(unsigned int)((unsigned char)instr[5]) );
		return false;
	}

	this->data.clear();
	uint64 curr_val = 0;
	unsigned int remaining_bits = entry_width;
	
	for (unsigned int npos = 6;npos < instr.length(); ++npos )
	{
		unsigned char curr_char = (unsigned char)instr[npos];
	
		if (remaining_bits >= 8)
		{
			remaining_bits -= 8;
			curr_val += (uint64)(curr_char) << remaining_bits;
			
		}
		else
		{
			unsigned int rem_charbits = 8-remaining_bits; // remaining bits in current character
			curr_val += (uint64)(curr_char) >> rem_charbits;
			this->data.push_back(curr_val);
			
			// write entries that are entirely inside current byte (for entry_width < 8)
			while (rem_charbits >= entry_width)
			{
				rem_charbits -= entry_width;
				curr_val = ((uint64)(curr_char) >> rem_charbits) % (1<<entry_width);
				this->data.push_back(curr_val);
			}
			
			// fill curr_val with overlapping part
			remaining_bits = entry_width - rem_charbits;
			curr_val = ((uint64)(curr_char) % (1<<rem_charbits)) << remaining_bits;
		}
		
		
	}
	
	if (remaining_bits == 0) // no remainder -> this is the last entry; else: it is overlap -> do not add
		this->data.push_back(curr_val);

	return true;
}



//_____________________________________________
// IData
//_____________________________________________

///< IData constructors
IData::IData():_payload(p_empty){}
IData::IData(ci_packet_t cip):_data(cip){_payload=p_ci;}
IData::IData(l2_pulse l2p):_event(l2p){_payload=p_event;}
IData::IData(l2_packet_t l2data):_l2data(l2data){_payload=p_layer2;}

///< IData destructor
IData::~IData(){}

///< IData (de)serialization methods
void IData::serialize(char* buffer, bool verbose){
	ptype temptype = this->_payload;
	uint tempaddr = this->_addr;
	
	uint i,pos=0; // loop counter
	
	// serialize IData content common for all types:
	for(i=0; i<IDATA_ID_SIZE;   i++, temptype = (ptype)((int)temptype >> 8), pos++) buffer[pos] = (uint)(int)temptype & 0xff;
	for(i=0; i<IDATA_ADDR_SIZE; i++, tempaddr >>= 8, pos++)                         buffer[pos] = tempaddr & 0xff;
	
	switch(_payload){
		case p_empty: std::cout << "IData::serialize: cannot serialize empty IData!" << std::endl;
		              //assert(false);
						      break;

		case p_ci:    _data.serialize(buffer + pos, verbose);
									break;

		case p_event: _event.serialize(buffer + pos, verbose);
		              break;

		case p_layer2:   std::cout << "IData::serialize: serialization of Layer2 data not yet implemented! Use native serialize method." << std::endl;
		              break;


		std::cout << "IData::serialize: illegal payload specified for serialization!" << std::endl;
	}
}

/** Deserialize IData. Information is extracted in the order specified in IData::serialize() */
void IData::deserialize(const char* buffer){
	this->clear();
	uint i,pos=0;
	uint temptype=0;
	
	// deserialize IData content common for all types:
	for(i=0; i<IDATA_ID_SIZE; i++, pos++)   temptype    += ((buffer[pos] & 0xff) << (8*i));
	for(i=0; i<IDATA_ADDR_SIZE; i++, pos++) this->_addr += ((buffer[pos] & 0xff) << (8*i));
	this->_payload = (ptype)temptype;

	//std::cout << "IData::deserialize: type = " << this->_payload << ", addr = " << this->_addr << " (p_ci = " << p_ci << ")" << std::endl;

	switch(_payload){
		case p_empty: std::cout << "IData::deserialize: cannot deserialize empty IData!" << std::endl;
		              //assert(false);
						      break;

		case p_ci:    _data.deserialize(buffer+pos);
						      break;

		case p_event: _event.deserialize(buffer+pos);
		              break;

		case p_layer2:   std::cout << "IData::deserialize: deserialization of Layer2 data not yet implemented!" << std::endl;
		              break;


		std::cout << "IData::deserialize: illegal payload for deserialization detected!" << std::endl;
	}
}
	
/// stream IData structs
std::ostream & operator<<(std::ostream &o,const IData & d){
	switch (d.payload()) {
	case p_empty: return o<<"ID:--"<<" A: " << d.addr();

	case p_ci:    return o<<"ID:p_ci"<<" -A: " << d.addr() << " -D: " << d.cmd();

	case p_event: return o<<"ID:p_event"<<" -A: " << d.addr() << endl; // " -D: " << d.event(); stack fault here ...

	case p_layer2: return o<<"ID:p_layer2"<<" -A: " << d.addr() << " -D: not implemented!";

	default: return o<<"ID:illegal ";
	} 
}


///< l2_pulse destructor
l2_pulse::~l2_pulse(){}

///< l2_pulse (de)serialization methods
void l2_pulse::deserialize(const char* buffer){
	this->clear();
	uint i,pos=0;
	uint size=0;
	
	for(i=0; i<IDATA_PAYL_SIZE; i++, pos++) size += ((buffer[pos] & 0xff) << (8*i));

	if(size > L2P_SIZE_REG){  // verbose stream
		std::cout << "Verbose deserialization of l2_pulse not yet implemented!" << std::endl;
		//assert(false);
	}else{					// standard length
		for(i=0; i<(L2_LABEL_WIDTH+7)/8; i++, pos++)                this->addr += ((buffer[pos] & 0xff) << (8*i));
		for(i=0; i<(L2_EVENT_WIDTH-L2_LABEL_WIDTH+7)/8; i++, pos++) this->time += ((buffer[pos] & 0xff) << (8*i));
	}
}
	
///< l2_pulse (de)serialization methods
void l2_pulse::serialize(char* buffer, bool verbose){
	l2_pulse tmp = *this; ///< temp object - content will be destroyed
	uint size;
	uint i,pos=0;

	
	if(verbose){
		size = L2P_SIZE_VERB;
		std::cout << "Verbose serialization of l2_pulse not yet implemented!" << std::endl;
		//assert(false);
	}else{
		size = L2P_SIZE_REG;
		for(i=0; i<IDATA_PAYL_SIZE;                     i++, size     >>= 8, pos++) buffer[pos] = size & 0xff;
		for(i=0; i<(L2_LABEL_WIDTH+7)/8;                i++, tmp.addr >>= 8, pos++) buffer[pos] = tmp.addr & 0xff;
		for(i=0; i<(L2_EVENT_WIDTH-L2_LABEL_WIDTH+7)/8; i++, tmp.time >>= 8, pos++) buffer[pos] = tmp.time & 0xff;
	}
}


/// stream SBData structs
std::ostream & operator<<(std::ostream &o,const SBData & d){
	switch (d.payload()) {
	case s_empty: return o<<"ID:--"<<" ";
	default: return o<<"ID:illegal";
	} 
}

} // namespace ess

