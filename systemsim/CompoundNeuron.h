#pragma once

#include "boost/numeric/odeint.hpp"
#include "heun2.hpp"
#include <boost/operators.hpp>
#include <vector>
#include <array>
#include <cmath>

#include "systemsim/DenmemParams.h"


/// state variables of a denmem
struct DenmemState {
	DenmemState():
		w(0.)
		,g_syn{{0}}
		,I_ext(0.)
	{}
	double w; //!< adaptation current w
	std::array<double,2> g_syn; //!< synaptic conductances
	double I_ext; //!< external current from current source
};


/// State variables of a compound neuron.
/// A compound neuron is built of N interconnected denmems sharing the same membrane voltage.
class CompoundNeuronState :
    boost::additive1< CompoundNeuronState,
    boost::additive2< CompoundNeuronState, double ,
    boost::multiplicative2< CompoundNeuronState, double > > >
{
public:
	double V; //!< membrane voltage (shared by all connected denmems)
	double t_end_of_refractory_period; //!< end of refractory period in seconds
	std::vector<DenmemState> denmems; //!< denmem states

	/// default constructor
	CompoundNeuronState():
		V(0.)
		,t_end_of_refractory_period(0.)
		,denmems() {}

	/// constructor for compound neuron with `N` denmems
	CompoundNeuronState(size_t N):
		V(0.)
		,t_end_of_refractory_period(0.)
		,denmems(N) {}

	CompoundNeuronState& operator+= (CompoundNeuronState const &rhs) {
		V += rhs.V;
		t_end_of_refractory_period += rhs.t_end_of_refractory_period;
		for (size_t ii = 0; ii < denmems.size(); ++ii) {
			auto & md = denmems[ii];
			auto const & od = rhs.denmems[ii];
			md.w += od.w;
			for (size_t jj=0; jj < md.g_syn.size(); ++jj) {
				md.g_syn[jj] += od.g_syn[jj];
			}
		}
		return *this;
	}

	CompoundNeuronState& operator*= (const double a) {
		V *= a;
		// t_end_of_refractory_period is not changed!
		for (size_t ii = 0; ii < denmems.size(); ++ii) {
			auto & md = denmems[ii];
			md.w *= a;
			for (size_t jj=0; jj < md.g_syn.size(); ++jj) {
				md.g_syn[jj] *= a;
			}
		}
		return *this;
	}

    size_t size() const {
		return denmems.size();
	}

    void resize( const size_t n ) {
		denmems.resize( n );
	}
};

// CompoundNeuronState resizeability
namespace boost { namespace numeric { namespace odeint {

template<>
struct is_resizeable< CompoundNeuronState >
{ // declare resizeability
    typedef boost::true_type type;
    static const bool value = type::value;
};

template< >
struct same_size_impl< CompoundNeuronState , CompoundNeuronState >
{ // define how to check size
    static bool same_size( const CompoundNeuronState &v1 ,
                           const CompoundNeuronState &v2 )
    {
        return v1.size() == v2.size();
    }
};

template< >
struct resize_impl< CompoundNeuronState , CompoundNeuronState >
{ // define how to resize
    static void resize( CompoundNeuronState &v1 ,
                        const CompoundNeuronState &v2 )
    {
        v1.resize( v2.size() );
    }
};

} } }


/// IF_cond_exp dynamics of a compound neuron
/// This class only exists for testing.
class CompoundIfCondExpDynamics {
public:
	/// denmem parameters
	std::vector<DenmemParams> const & mParams;

	/// constructor
	CompoundIfCondExpDynamics( std::vector<DenmemParams> const & params);

	typedef CompoundNeuronState state_type;

	/// function for differential state update
	void operator() ( const state_type &x , state_type &dxdt , const double t) const;

	/// initialize the dynamics class.
	/// computes the total capacitance.
	void initialize();

private:
	/// total membrane capacitance
	double cm_total;
};


/// AdEx dynamics of a compound neuron
class CompoundAdExDynamics{
public:
	/// denmem parameters
	std::vector<DenmemParams> const & mParams;

	/// constructor
	CompoundAdExDynamics( std::vector<DenmemParams> const & params);

	typedef CompoundNeuronState state_type;

	/// function for differential state update
	void operator() ( const state_type &x , state_type &dxdt , const double t) const;

	/// initialize the dynamics class.
	/// sets the firing denmem and computes the total capacitance.
	void initialize(
		size_t id_firing_denmem //!< id of of firing denmem
			);

private:
	/// total membrane capacitance
	double cm_total;

	/// id of firing denmem
	size_t firing_denmem;
};


/** A compound neuron.
 * Holds the state variables and parameters of a compound neuron built of
 * several denmems.
 * Updates the state according to AdEX dynamics.
 * Allows to set the firing denmem which determines the used tau_refrac,
 * v_spike and v_reset.
 * Offers functions for spike and current input.
 * All variables and parameters in SI units.
 */
class CompoundNeuron {
public:
	typedef CompoundNeuronState state_type;

	/// create a compound neuron of `N` connected denmems
	CompoundNeuron(size_t N);

	/// set the parameters of denmem `id`.
	void		 setDenmemParams(size_t id, DenmemParams const & params);
	DenmemParams getDenmemParams(size_t id) const;

	/// adds the synaptic weight `weight` (in Siemens) to the synaptic conductance `input`
	/// of denmem `id`.
	void inputSpike(size_t id, bool input, double weight);

	/// updates the input current into denmem `id` to `current`.
	/// @param current external current in Ampere
	void inputCurrent(size_t id, double current);

	/// sets the id of the firing denmem.
	/// From this the parameters "tau_refrac", "v_spike" and "v_reset" are used
	void   setFiringDenmem(size_t id);
	size_t getFiringDenmem() const;

	/// updates the state of the compound neuron from `t_start` to `t_end` (in seconds).
	/// returns true if the neuron has fired (crossed the spiking threshold)
	/// between `t_start` to `t_end`.
	bool update(double t_start, double t_end);

	/// initiates the state variables of the compound neuron
	/// sets the membrane voltage to the resting potential
	/// of the firing denmem
	/// @todo: we might make this more verbose, e.g. set to average resting
	/// potential
	void initialize();

	state_type mState; //!< compound neuron state
private:
	/// check whether `id` is smaller than the size of the compound neuron.
	void checkDenmemId(size_t id) const;

	const size_t mSize; //!< nr of denmems in compound neuron
	size_t mFiring; //!< id of firing denmem
	std::vector<DenmemParams> mParams; //!< denmem parameters
	CompoundAdExDynamics mDynamics; //!< AdEx dynamics

	/// the stepper, which performs the actual numerical integration.
	///
	/// Here, we use the 2nd order Heun method.
	/// It does not make sense to use higher order methods, as the effect of
	/// of the detection of spikes in the priority encoder leads to an error
	/// in the order of the the HICANN PLL clock cycle.
	/// Hence, if we assume a step size lower or equal than the PLL clock cycle,
	/// it does not make sense to use a method like runge_kutta4, as the overall
	/// accuracy gain is neglectable.
	/// Henker et al 2012: Stephan Henker, Johannes Partzsch und René Schüffny,
	/// Accuracy evaluation of numerical methods used in state-of-the-art
	/// simulators for spiking neural networks, Journal of Computational
	/// Neuroscience, vol. 32, Feb 2012, p. 309-326.
	boost::numeric::odeint::heun2<
		CompoundNeuron::state_type, double, CompoundNeuron::state_type, double,
		boost::numeric::odeint::vector_space_algebra > mStepper;

};

