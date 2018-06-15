#include "systemsim/CompoundNeuron.h"
#include <stdexcept>


CompoundIfCondExpDynamics::CompoundIfCondExpDynamics(
	std::vector<DenmemParams> const & params
	):
	mParams(params)
	,cm_total(DenmemParams().cm)
{}

void CompoundIfCondExpDynamics::operator() ( const state_type &x , state_type &dxdt , const double t) const
{
	assert (x.size() ==  mParams.size() );
	assert (x.size() > 0);
	double I_syn = 0.;
	double I_g_l = 0.;
	double I_ext = 0.;
	for (size_t ii = 0; ii < x.size(); ++ii) {
		auto & d_denmem = dxdt.denmems[ii];
		auto const & denmem = x.denmems[ii];
		auto const & p = mParams[ii];
		for (size_t jj = 0; jj < denmem.g_syn.size(); ++jj) {
			d_denmem.g_syn[jj] = -denmem.g_syn[jj] / p.tau_syn[jj];
			I_syn += denmem.g_syn[jj]*(p.e_rev[jj] - x.V);
		}
		I_g_l += -p.g_l*(x.V -p.v_rest);
		I_ext += denmem.I_ext;
	}
	// the refractory period is handled in the dynamics, as it will
	// influence adaptation current state w in Adex
	if (t >= x.t_end_of_refractory_period)
		dxdt.V  = (I_g_l + I_syn + I_ext)/cm_total;
	else
		dxdt.V  = 0.;
}

void CompoundIfCondExpDynamics::initialize()
{
	// total cap
	cm_total = 0.;
	for (DenmemParams d: mParams)
		cm_total += d.cm;
}



CompoundAdExDynamics::CompoundAdExDynamics(
	std::vector<DenmemParams> const & params
	):
	mParams(params)
	,cm_total(DenmemParams().cm)
	,firing_denmem(0)
{}

void CompoundAdExDynamics::operator() ( const state_type &x , state_type &dxdt , const double t) const
{
	assert (x.size() ==  mParams.size() );
	assert (x.size() > 0);
	double I_syn = 0.;
	double I_g_l = 0.;
	double I_exp = 0.;
	double I_adapt = 0.;
	double I_ext = 0.;

	// we limit the membrane voltage V used to compute dxdt to v_spike
	// This avoids 2 possible problems:
	// - "w" can become too big which would
	//   destroy the correctness of further computation
	// - the exponential current goes towards infinity
	double const V = std::min(x.V, mParams[firing_denmem].v_spike);
	for (size_t ii = 0; ii < x.size(); ++ii) {
		auto & d_denmem = dxdt.denmems[ii];
		auto const & denmem = x.denmems[ii];
		auto const & p = mParams[ii];
		// synaptic input
		for (size_t jj = 0; jj < denmem.g_syn.size(); ++jj) {
			d_denmem.g_syn[jj] = -denmem.g_syn[jj] / p.tau_syn[jj];
			I_syn += denmem.g_syn[jj]*(p.e_rev[jj] - V);
		}
		// leak current
		I_g_l += -p.g_l*(V -p.v_rest);
		// exponential term
		I_exp +=  p.g_l*p.delta_T*std::exp((V-p.v_thresh)/p.delta_T);
		// adaptation
		d_denmem.w = (p.a * (V-p.v_rest) - denmem.w)/p.tau_w;
		I_adapt += denmem.w;
		// external current
		I_ext += denmem.I_ext;
	}

	// the refractory period is handled in the dynamics,
	// as it will influence adaptation current state w in Adex
	// in multi step solvers
	if (t >= x.t_end_of_refractory_period) {
		dxdt.V  = (I_g_l + I_exp - I_adapt + I_syn + I_ext)/cm_total;
	} else {
		dxdt.V  = 0.;
	}
}

void CompoundAdExDynamics::initialize(
		size_t id_firing_denmem //!< id of of firing denmem
		)
{
	assert(id_firing_denmem < mParams.size());
	firing_denmem = id_firing_denmem;

	// total cap
	cm_total = 0.;
	for (DenmemParams d: mParams)
		cm_total += d.cm;
}

CompoundNeuron::CompoundNeuron(size_t N):
	mState(N)
	,mSize(N)
	,mFiring(0)
	,mParams(N)
	,mDynamics(mParams)
	,mStepper()
{}

void
CompoundNeuron::setDenmemParams(size_t id, DenmemParams const & params) {
	checkDenmemId(id);
	mParams[id] = params;
}

DenmemParams
CompoundNeuron::getDenmemParams(size_t id) const {
	checkDenmemId(id);
	return mParams[id];
}

void
CompoundNeuron::checkDenmemId(size_t id) const {
	if (id >= mSize) {
		std::stringstream ss;
		ss << "ID of denmem in compound neuron too high. ";
		ss << "ID: " << id << ", size: " << mSize;
		throw std::out_of_range(ss.str());
	}
}

void
CompoundNeuron::inputSpike(size_t id, bool input, double weight) {
	checkDenmemId(id);
	mState.denmems[id].g_syn[input] += weight;
}

bool
CompoundNeuron::update(double t_start, double t_end)
{
	assert(t_end > t_start);

	mStepper.do_step(mDynamics, mState, t_start, t_end - t_start);

	bool spike = false;

	if ( mState.V >= mParams[mFiring].v_spike ) {
		mState.V = mParams[mFiring].v_reset;
		mState.t_end_of_refractory_period = t_end + mParams[mFiring].tau_refrac;
		// spike triggered adaptation
		for (size_t ii = 0; ii < mState.size(); ++ii) {
			mState.denmems[ii].w += mParams[ii].b;
		}
		spike = true;
	}
	return spike;
}

void
CompoundNeuron::initialize() {
	mState.V = mParams[mFiring].v_rest;
	mDynamics.initialize(mFiring);
}

void
CompoundNeuron::setFiringDenmem(size_t id) {
	checkDenmemId(id);
	mFiring=id;
}

size_t
CompoundNeuron::getFiringDenmem() const {
	return mFiring;
}

void
CompoundNeuron::inputCurrent(size_t id, double current) {
	checkDenmemId(id);
	mState.denmems[id].I_ext = current;
}
