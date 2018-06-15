//_____________________________________________
// Company      :	uhei
// Author       : 	Johannes Fieres
// E-Mail   	:	fieres@kip.uni-heidelberg.de
// Last Change  :   mehrlich 11/29/2011
//_____________________________________________
#ifndef ANNCORE_TASK_IF_H
#define ANNCORE_TASK_IF_H

#include "systemc.h"

#include <vector>

#include "sim_def.h"


/** interface of the anncore for sending pulses to the core and configuring it */
class  anncore_task_if :  virtual public sc_interface
{
public:
    /** Use this fct to send a pulse with addr to the syndriver_id-th syndriver.
	 * syndrivers are counted first all on the left side, from top to bottom, then right side ...*/
    virtual void recv_pulse(
			unsigned int syndriver_id,  //!< syndriver id: in range(2*SYNDRIVERS), 
			unsigned int addr  //!< 6-bit l1 address of sending neuron
			)=0;

    //FIXME CP: I changed initialization of weights to 0, setting it to 1 activates sysnapses that should not be activated
    // and hence makes debugging impossible
    /** all weights of the anncore, in row-first order, first the
	upper block, then the lower (values: 0...15).  Before calling
	this fct, all weight values are initialized with 1, and the
	thresholds by 0, so every incoming spike should result in
	output spikes.*/
    virtual void configWeights(const std::vector<char> &weights)=0;
    /** Set one specific weight (0-15). SYN_NUMCOLS_PER_ADDR columns addressed at once - column address limited to 2^SYN_COLDATA_WIDTH! */
    virtual void configWeight(int x, int y, sc_uint<SYN_NUMCOLS_PER_ADDR*SYN_COLDATA_WIDTH> weight)=0;
    /** returns reference to the internal weight array. Data format:
	see doc of configWeights())*/
    virtual const std::vector<char>& getWeights() const =0;
    /** returns reference to the weight values of one address within syn memory. Data format:
		    see doc of configWeight(). */
	virtual const sc_uint<SYN_NUMCOLS_PER_ADDR*SYN_COLDATA_WIDTH>& getWeight(int x, int y) const =0;


    /** The adresses of all synapse decoders in row-first order, first
	the upper block, then the lower (values: 0...L1_ADDR_RANGE-1).  */
    virtual void configAddressDecoders(const std::vector<char> &conns)=0;
    /** Configure one specific address decoder. SYN_NUMCOLS_PER_ADDR columns addressed at once - column address limited to 2^SYN_COLDATA_WIDTH! */
    virtual void configAddressDecoder(int x, int y, sc_uint<SYN_NUMCOLS_PER_ADDR*SYN_COLDATA_WIDTH> addr)=0;
    /** returns reference to the internal decoder array. Data format:
	see doc of configAddressDecoders())*/
    virtual const std::vector<char>& getAddressDecoders() const =0;
//    /** returns reference to the decoder addresses of one address within syn memory. Data format:
//		    see doc of configAddressDecoder(). */
//		virtual const sc_uint<SYN_NUMCOLS_PER_ADDR*SYN_COLDATA_WIDTH>& getAddressDecoder(int x, int y)=0;


    /** Set syndriver mirrors.  <tt>conns</tt> must be of size
	2*SYNDRIVERS. conns[n] denotes the connection between the n-th
	and the (n+1)th syndriver. Syndrivers are counted from top of
	the anncore to the bottom, first the complete left side, then
	the complete right side.

	<p>A value of 1 means mirroring takes place in the downward
	direction, a value of 2 indicates upward direction, a value of
	0 indicates no mirroring. All other values are forbidden.

	<p> The following positions are ignored, since they would
	connect syndrivers of different blocks: SYNDRIVERS/2-1,
	2*SYNDRIVERS/2-1, 3*SYNDRIVERS/2-1, 4*SYNDRIVERS/2-1.
    */
    //CP: Dont need this any more is done via configSyndriver
    //virtual void configSyndriverMirrors(const std::vector<char>& conns)=0;
    /** configure a specific syndriver mirror.*/
    // virtual void configSyndriverMirror(int side, int position, char value)=0;

    /**  returns reference to the internal syndriver mirror array.
	 Data format: see doc of configSyndriverMirrors())*/
    virtual const std::vector<char>& getSyndriverMirrors() const =0;
    /** returns reference to the mirror configuration of one synapse driver. Data format:
		    see doc of configSyndriverMirror(). */
	virtual bool getSyndriverMirror(int side, int position) const =0;

    virtual ~anncore_task_if(){}
};

#endif // ANNCORE_TASK_IF_H

