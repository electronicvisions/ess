/* TODO:
* make members private (only 'mirror' missing)
* STDP ?
 
*/
#ifndef _syndriver_h_
#define _syndriver_h_
#include <vector>
#include <stdint.h>
#include <iostream>
#include "systemc.h"

#include "DenmemIF.h"

//	pre-declarations
class hw_neuron;

/** The syndriver class has a pointer to the array of its 2 synapse
    rows. When the syndriver is fired by calling <tt>pulse(addr)</tt>,
    it selects all the synapses with the correct decoder address (
    also the synapses of the neighbor syndrivers, if mirrored), and
    accumulates a value proportional to the synapse weight to the
    connected denmem.
  */
class syndriver
{
 private:
    // id of this hicann  
    short _hicann_id;
    // id of this syndriver
    short _id;
    // enable flag that determines if this syndriver is active or inactive
    bool _enable;
    // bit to determine whether this driver gets input via L1
    bool _l1;

    /** block_offset for the target hw_neurons.
	 *  0: upper block, SYN_PER_ROW: lower block*/
    int _block_offset;
    /** weights (0...15)*/
    char* _weights;
	/** synaptic weight distortion multiplicator*/
	float* _weight_distortions;
    // time efficient
    //map<int,vector<int> > synapses; // indicates the row!
    // space efficient
    /** addresses (0..63) */
    char* _addresses;

	std::array< std::unique_ptr<DenmemIF>, 512 > *_denmems;

    /** 1=up, 2=down 
     *  up = take the same input as the syndriver above
     *  down = use the same input as syndr. below
     * */
    bool _mirror;

    /** synapse type of a row: 0=exc, 1=inh
     *	exc means: the whole row is connected to the excitatory Input of denmems
	 *	index 0 corresponds to the bottom row of the syndriver, i.e. to an even synapse row.
	 *	index 1 corresponds to the top row of the syndriver, i.e. to an odd synapse row.
     * */
    char _syn_type[2];

	/** the pattern to which the strobe lines listen.
	 * i.e. the preout config. In contrast to the real HICANN, where the assignment of patterns to strobeline depends also on the global enable bits,
	 * we store here directly the pattern for each strobeline.
	 * possible values: 0,1,2,3
	 *	_pre_out[0][0] = preout_0: EVEN synapses in EVEN rows
	 *	_pre_out[0][1] = preout_2: ODD synapses in EVEN rows
	 *	_pre_out[1][0] = preout_1: EVEN synapses in ODD rows
	 *	_pre_out[1][1] = preout_3: ODD synapses in ODD rows
	 */
	char _pre_out[2][2];

	/** gmax divisor
	 * This parameter is not used during simulation, it is only there for completeness of ESS tests!
	 * possible values: 1-15
	 *
	 * _gmax_div[0][0]: even row, syntype=0
	 * _gmax_div[0][1]: even row, syntype=1
	 * _gmax_div[1][0]: odd row, syntype=0
	 * _gmax_div[1][1]: odd row, syntype=1
	 */
	uint8_t _gmax_div[2][2];

	/** select from 4 quadrantwise V_gmax current of shared floating gate params
	 * This parameter is not used during simulation, it is only there for completeness of ESS tests!
	 * index 0: row synapse row (bottom line of driver)
	 * index 1: odd synapse row (top line of driver)
	 */
	uint8_t _sel_Vgmax[2];

	/** the analog weight for each digital weight in each row.
	 *	index 0 corresponds to the bottom row of the syndriver, i.e. to an even synapse row.
	 *	index 1 corresponds to the top row of the syndriver, i.e. to an odd synapse row.
	 * size: 16
	 */
	std::vector<double> _analog_weight[2];

	/** the polynomial synapse trafo from digital to analog weight
	 *	index 0 corresponds to the bottom row of the syndriver, i.e. to an even synapse row.
	 *	index 1 corresponds to the top row of the syndriver, i.e. to an odd synapse row.
	 * size: usually 3
	 */
	std::vector<double> _synapse_trafo[2];
	/////////////////////////////
	///// Short Term Plasticity
	/////////////////////////////

	///// functional parameters
    /** Capacitor vector managing the Short-Term-Plasticity Mechanism
	* There is one Capacitor for each presynaptic Neuron that stores the INACTIVE FRACTION.
	* between 0..1
    **/
	std::vector<double> _STP_CAP;
	/** vector of last input times - one for every presynaptic Neuron*/
	std::vector<double> _STP_TIME;
	/** usable synaptic efficacy */
	double _U_SE;
	/** model parameter N in depression mode*/
	double _N_dep;
	/** model parameter N in facilitation mode*/
	double _N_fac;
	/** scaling factor */
	double _lambda;
	/** recovery time constant*/
	double _tau_rec;

////// hardware parameters 
	/** 1=STP is enabled*/
	bool _stp_enable;
	/** 0=depression 1=facilitation*/
	bool _mode;
	/** size of cap_2: possible range: 0..7 -> determines usable synaptic efficacy U_SE(individual for every syndriver)*/
	uint8_t _cap_2;

 public:
   syndriver(){}

    /** Constructor of a syndriver.
    */
    syndriver(
            short hicann_id,                    //!< id of the hicann of this syndriver
            short id,                           //!< id of this syndriver
            char* weights,                      //!< pointer to the first weight of the first row of this syndriver 
			float* weight_distortions,          //!< pointer to the first  synaptic weight distortion multiplicator of the first row of this syndriver 
			char* addresses,                    //!< pointer to the first address decoder of the first row of this syndriver 
			std::array< std::unique_ptr<DenmemIF>, 512 > *denmems,
			char block                          //!< block of this syndriver. 0=upper, 1=lower
			);

    /**This functions sends a pulse to all corresponding neurons
     * implements pulse routing in the anncore from the syndriver to the hw_neurons where the fct spike_in(weight, syn_type) is called.
     * If i-th entry of address decoder(= i-th synapse) equals addr (sender address), the i-th hw_neuron gets an input spike.
     * this is done for upper and lower row of the syndriver(ROWS_PER_SYNDRIVER must be 2)
     * returns the number of synapses which were fired
     */
    int pulse(int addr);

	/** Function to to set Short Term Plasticity Mechanism of this syndriver.
	 * also initialize variables for functional STP*/
	void set_STP(
		bool enable,  //!< enable STP: 1=STP is enabled
		bool mode,    //!< stp mode: 0=depression 1=facilitation
		uint8_t cap_2, //!<  size of cap_2: possible range: 0..7 -> determines usable synaptic efficacy U_SE(individual for every syndriver)*/
		double tau_rec, //!< recovery time constant in s
		double lambda, //!< stp model parameter lambda
		double N_dep, //!< model parameter N in depression mode
		double N_fac //!< model parameter N in facilitation mode
	);

    bool get_mirror() const;
    void set_mirror(bool mirror);

    void set_synapse_trafo(bool row, const std::vector<double>& trafo);
    const std::vector<double>& get_synapse_trafo(bool row) const;

    void set_syn_type(bool row, char syn_type);
    char get_syn_type(bool row) const;
    
    void set_pre_out(unsigned int id, uint8_t pattern);
    uint8_t get_pre_out(unsigned int id) const;

	void set_sel_Vgmax(bool row, uint8_t val);
	uint8_t get_sel_Vgmax(bool row) const;

	void set_gmax_div(bool row, bool syn_type, uint8_t val);
	uint8_t get_gmax_div(bool row, bool syn_type) const;

    void set_enable(bool enable);
    bool get_enable() const;
    
    void set_l1(bool l1);
    bool get_l1() const;

    bool get_stp_enable() const;
	
    bool get_mode() const;
	
    double get_U_SE() const;
	
    double get_tau_rec() const;
    
    int get_stp_cap() const;

    double get_V_max() const;

    double get_V_fac() const;

	/// returns model parameter lambda
    double get_lambda() const;

	/// returns model parameter N
	/// which is _N_dep in depression mode, and _N_fac in facilitation mode.
    double get_N() const;
};
#endif // _syndriver_h_

