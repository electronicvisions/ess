#include "syndriver.h"

#include "sim_def.h"
#include "hw_neuron.h"

#include "calibtic/HMF/STPUtilizationCalibration.h"
#include "paramtrafo_types.h"
#include <log4cxx/logger.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("ESS.HICANN.Layer1");

syndriver::syndriver(
            short hicann_id,                    //!< id of the hicann of this syndriver
            short id,                           //!< id of this syndriver
			char* weights,                      //!< pointer to the first weight of the first row of this syndriver 
			float* weight_distortions,          //!< pointer to the first  synaptic weight distortion multiplicator of the first row of this syndriver 
			char* addresses,                    //!< pointer to the first address decoder of the first row of this syndriver 
			std::array< std::unique_ptr<DenmemIF>, 512 > *denmems,
			char block                          //!< block of this syndriver. 0=upper, 1=lower
			):
    _hicann_id(hicann_id)
    ,_id(id)
    ,_enable(false)
    ,_l1(false)
	,_weights(weights)
	,_weight_distortions(weight_distortions)
	,_addresses(addresses)
	,_denmems(denmems)
	,_mirror(false)
	,_U_SE(7./15.)
	,_N_dep(0.)
	,_N_fac(1.)
	,_lambda(1.)
	,_tau_rec(0.1)
	,_stp_enable(false)
	,_mode(false)
	,_cap_2(3)
{
	_syn_type[0]=0;
	_syn_type[1]=0;

	_gmax_div[0][0]=1;
	_gmax_div[0][1]=1;
	_gmax_div[1][0]=1;
	_gmax_div[1][1]=1;

	_sel_Vgmax[0]=0;
	_sel_Vgmax[1]=0;

	_analog_weight[0] = std::vector<double>();
	_analog_weight[1] = std::vector<double>();
	std::vector<double> default_trafo(2,0.);
	default_trafo[1] = 50.e-9/16.;
	_synapse_trafo[0]=std::vector<double>();
	_synapse_trafo[1]=std::vector<double>();
	set_synapse_trafo(0,default_trafo);
	set_synapse_trafo(1,default_trafo);

	_STP_CAP.resize(64,0.0); // initialize Inactive Partiton with 0
	_STP_TIME.resize(64,0.0); // initialize time of last input with 0

	this->_block_offset=block*SYN_PER_ROW;
}


void
syndriver::set_STP(
	bool enable,  //!< enable STP: 1=STP is enabled
	bool mode,    //!< stp mode: 0=depression 1=facilitation
	uint8_t cap_2, //!<  size of cap_2: possible range: 0..7 -> determines usable synaptic efficacy U_SE(individual for every syndriver)*/
	double tau_rec, //!< recovery time constant in s
	double lambda, //!< stp model parameter lambda
	double N_dep, //!< model parameter N in depression mode
	double N_fac //!< model parameter N in facilitation mode
)
{
	// store hw-parameters
	_stp_enable = enable;
	_mode = mode;
	assert( cap_2 < 8);
	_cap_2 = cap_2;

	_tau_rec=tau_rec;
	_lambda=lambda;
	_N_dep = N_dep;
	_N_fac = N_fac;

	// TODO: U_SE should be set externally
	HMF::STPUtilizationCalibration U_calib;
	U_calib.setDefaults();
	_U_SE = U_calib.getUtilization(_cap_2);

}


int
syndriver::pulse(int addr)
{
    LOG4CXX_TRACE(logger, "syndriver with id: " << _id << " on hicann: " << _hicann_id << " pulse called with address:" << addr );

	// weight multiplier is calculated only once, and is possible further modified by STP
	double stp_factor = 1.;
	////////////////////////////////////
	/// if STP: update Inactive Partition
	////////////////////////////////////
	if(_stp_enable==true)
	{
		double current_time = sc_time_stamp().to_seconds();
		// get the current inactive partition, i.e. compute the exponential recovering
		double current_inactive_partition = exp(-(current_time - _STP_TIME[addr])/_tau_rec)*_STP_CAP[addr];
		// STP move utilized fraction to I
		// this happens AFTER the spike, but we can do it now.
		_STP_CAP[addr] = current_inactive_partition + _U_SE*(1-current_inactive_partition);
		// update time
		_STP_TIME[addr]=current_time;
		// update stp_factor according to mode
		if(_mode==0)	// depression
			stp_factor *= (1-_lambda*(current_inactive_partition-_N_dep));
		else		// facilitation
			stp_factor *= (1+_lambda*(current_inactive_partition-_N_fac));

	}
	/////////////////////////////
	//////// Send PULSES ////////
	/////////////////////////////
	int count=0;

	// We go through the synapses of this driver strobeline wise:
	// first row-wise, than strobeline-wise
	const char adr_2MSB = addr/16;
	const char adr_4LSB = addr%16;
	for(size_t row = 0; row < 2; ++row){
		char syn_type_row = _syn_type[row];
		const std::vector<double>& analog_weight = _analog_weight[row];
		size_t row_offset = row*SYN_PER_ROW; // 0 or 256
		for(size_t strobeline = 0; strobeline < 2; ++strobeline){
			// check if this strobeline is programmed for this pattern
            if (adr_2MSB == _pre_out[row][strobeline]){
			    LOG4CXX_TRACE(logger, "syndriver with id: " << _id << " on hicann " << _hicann_id << " received pulse with Address:" << addr << " that fits to DriverDecoder: " << (int)adr_2MSB << " in row: " << row << " and strobeline: " << strobeline);
				// go through all synapses of this strobeline
				// check whether addres decoder corresponds to 4 LSB
				std::array< std::unique_ptr<DenmemIF>, 512 >::iterator h=_denmems->begin() + _block_offset + strobeline;
				for(size_t i=row_offset + strobeline; i<SYN_PER_ROW + row_offset; i+=2,h+=2){
					char digital_w = _weights[i];
                    //logging more for debbuging synapses
                    //LOG4CXX_TRACE(logger, "syndriver with id: " << _id << " on hicann " << _hicann_id << " synapse " << i 
                    //        << " weight " << (int )digital_w << " syn_decoder " << (int) _addresses[i] << " neuron " << (*h));
                    if(_addresses[i]==adr_4LSB && digital_w < 16 && (*h != NULL)){
							double effective_weight = analog_weight[digital_w] * stp_factor * _weight_distortions[i];
                            LOG4CXX_TRACE(logger, "syndriver with id: " << _id << " on hicann " << _hicann_id << " pulse: addr:" << addr
								  << " sent pulse to synapse " << i << " with digital weight:" << (int)_weights[i] 
								  << " analog_weight " << effective_weight << " address:" << (int)_addresses[i] 
							      << " hwneuron:" << (*h).get() );
                            (*h)->inputSpike(syn_type_row, effective_weight);
							++count;
					}
				}
			}
		}
	}
	return count;
}

void syndriver::set_synapse_trafo(bool row, const std::vector<double>& trafo){
	_analog_weight[row].clear();
	for(size_t dw = 0; dw<16; ++dw)
		_analog_weight[row].push_back(trafo_polynomial<double,double>( static_cast<double>(dw), trafo));
	_synapse_trafo[row]=trafo;
}

const std::vector<double>& syndriver::get_synapse_trafo(bool row) const
{
    return _synapse_trafo[row];
} 

void syndriver::set_mirror(bool mirror)
{
    _mirror = mirror;
}

bool syndriver::get_mirror() const
{
    return _mirror;
}

void syndriver::set_syn_type(bool row, char syn_type)
{
    _syn_type[row]=syn_type;
}

char syndriver::get_syn_type(bool row) const 
{
    return _syn_type[row];
}

void syndriver::set_pre_out(unsigned int id, uint8_t pattern)
{
    _pre_out[id%2][id/2] = static_cast<char>(pattern);
}

uint8_t syndriver::get_pre_out(unsigned int id) const 
{
    return static_cast<uint8_t>(_pre_out[id%2][id/2]);
}

void syndriver::set_enable(bool enable)
{
    _enable = enable;
}

bool syndriver::get_enable() const
{
    return _enable;
}
    
void syndriver::set_l1(bool l1)
{
    _l1 = l1;
}
    
bool syndriver::get_l1() const
{
    return _l1;
}

bool syndriver::get_stp_enable() const 
{
    return _stp_enable;
}

bool syndriver::get_mode() const 
{
    return _mode;
}

double syndriver::get_U_SE() const 
{
    return _U_SE;
}

double syndriver::get_tau_rec() const 
{
    return _tau_rec;
}

int syndriver::get_stp_cap() const
{
    return _cap_2;
}

void syndriver::set_sel_Vgmax(bool row, uint8_t val) {
	assert(val < 4);
	_sel_Vgmax[row] = val;
}

uint8_t syndriver::get_sel_Vgmax(bool row) const {
	return _sel_Vgmax[row];
}

void syndriver::set_gmax_div(bool row, bool syn_type, uint8_t val) {
	assert (val < 16 && val > 0);
	_gmax_div[row][syn_type] = val;
}

uint8_t syndriver::get_gmax_div(bool row, bool syn_type) const {
	return _gmax_div[row][syn_type];
}

double syndriver::get_lambda() const {
	return _lambda;
}

double syndriver::get_N() const {
	return _mode ? _N_fac : _N_dep;
}
