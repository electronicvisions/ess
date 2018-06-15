#include <gtest/gtest.h>

#include <iostream>
#include "systemsim/CompoundNeuron.h"
#include "systemsim/DenmemIF.h"
#include "systemc.h"
#include <vector>
#include "boost/numeric/odeint.hpp"
#include <tuple>

#include <fstream>

using namespace boost::numeric::odeint;
using namespace std;

typedef CompoundNeuronState state_type;

struct push_back_state_and_time
{
    std::vector< state_type >& m_states;
    std::vector< double >& m_times;

    push_back_state_and_time( std::vector< state_type > &states , std::vector< double > &times )
    : m_states( states ) , m_times( times ) { }

    void operator()( const state_type &x , double t )
    {
        m_states.push_back( x );
        m_times.push_back( t );
    }
};

void write_out( std::string fname, std::vector< state_type > const & states, std::vector< double > const & times ) {
	std::ofstream os(fname);
	for ( size_t i=0; i< times.size(); ++i ){
		os << times[i] << " " << states[i].V;
		for ( size_t j=0; j< states[i].size(); ++j) {
			os << " " << states[i].denmems[j].w;
			for (auto const & val : states[i].denmems[j].g_syn) {
				os << " " << val;
			}
		}
		os << endl;
	}
	os.close();
}


TEST(CompoundNeuronDynamics, DoubleDenmem) {
	const size_t N = 2;
	std::vector<DenmemParams> params(N);
	state_type state(N);
	CompoundIfCondExpDynamics dynamics(params);

	state.V = -0.065;
	params[0].tau_syn[0] = 0.01;
	params[1].tau_syn[0] = 0.1;
	params[1].tau_syn[1] = 0.001;

	// initialize dynamics before start of simulation
	dynamics.initialize();

	double dt = 0.0001;

	typedef tuple<double,size_t,bool> spike;
	vector< spike > spikes = { spike(0.5,0,false), spike(0.6,0,true), spike(0.7,1,false), spike(3.2,1,true) };

    runge_kutta4< state_type, double, state_type, double, vector_space_algebra > stepper;

	vector<state_type> x_vec;
	vector<double> times;

	double weight = 1e-7;
	double t = 0;
	double t_end = 3.5;
	for (auto s : spikes ) { // assume to be sorted
		double spike_time; size_t nrn; bool side;
		std::tie( spike_time, nrn, side ) = s;
		t += dt*integrate_const(stepper, dynamics, state,t,spike_time,dt, push_back_state_and_time( x_vec , times ));
		state.denmems[nrn].g_syn[side] += weight;
		cout << "V(" << t << ") = " <<  state.V << endl;
	}
	if (t_end > t) {
		t += dt*integrate_const(stepper, dynamics, state,t,t_end,dt, push_back_state_and_time( x_vec , times ));
	}
	cout << "V(" << t << ") = " <<  state.V << endl;

	write_out("data.txt", x_vec, times);
}

TEST(CompoundNeuron, DoubleDenmem) {
	CompoundNeuron cn(2);
	DenmemParams params1;
	params1.tau_syn[0] = 0.01;
	DenmemParams params2;
	params2.tau_syn[0] = 0.1;
	params2.tau_syn[1] = 0.001;
	double dt = 0.0001;
	DenmemIF D1(cn,0);
	DenmemIF D2(cn,1);
	D1.setDenmemParams(params1);
	D2.setDenmemParams(params2);

	// Before start of simulation:
	// 1.) set firing denmem
	cn.setFiringDenmem(1);
	// 2.) initialize
	cn.initialize();

	typedef tuple<double,size_t,bool> spike;
	vector< spike > spikes = { spike(0.5,0,false), spike(0.6,0,true), spike(0.7,1,false), spike(3.2,1,true) };

	vector<state_type> x_vec;
	vector<double> times;

	push_back_state_and_time recorder( x_vec , times );

	double weight = 1e-7;
	double t = 0;
	double t_end = 3.5;
	for (auto s : spikes ) { // assume to be sorted
		double spike_time; size_t nrn; bool side;
		std::tie( spike_time, nrn, side ) = s;
		while(t < spike_time) {
			cn.update(t,t+dt);
			t += dt;
			recorder(cn.mState, t);
		}
		cn.inputSpike(nrn,side,weight);
		cout << "V(" << t << ") = " <<  cn.mState.V << endl;
	}
	while(t < t_end) {
		cn.update(t,t+dt);
		t += dt;
		recorder(cn.mState, t);
	}
	cout << "V(" << t << ") = " <<  cn.mState.V << endl;

	write_out("data2.txt", x_vec, times);
}

TEST(AdExDynamics, SingleDenmem) {
	const size_t N = 1;
	std::vector<DenmemParams> params(N);
	state_type state(N);
	CompoundAdExDynamics dynamics(params);

	state.V = -0.065;
	params[0].tau_syn[0] = 0.01;
	params[0].v_spike = -0.04;

	// initialize dynamics before start of simulation
	dynamics.initialize(/*id_firing_denmem*/ 0);

	double dt = 0.0001;

	typedef tuple<double,size_t,bool> spike;
	vector< spike > spikes = { spike(0.5,0,false), spike(0.6,0,true), spike(0.7,0,false), spike(3.2,0,true) };

    runge_kutta4< state_type, double, state_type, double, vector_space_algebra > stepper;

	vector<state_type> x_vec;
	vector<double> times;
	push_back_state_and_time recorder( x_vec , times );

	double weight = 1e-8;
	double t = 0;
	double t_end = 3.5;
	for (auto s : spikes ) { // assume to be sorted
		double spike_time; size_t nrn; bool side;
		std::tie( spike_time, nrn, side ) = s;
		// t += dt*integrate_const(stepper, dynamics, state,t,spike_time,dt, push_back_state_and_time( x_vec , times ));
		while (t < spike_time) {
			recorder(state, t);
			stepper.do_step(dynamics, state, t, dt);
			t += dt;
			// TODO: should we check for being refractory again?
			if ( state.V >= params[0].v_spike ) {
				state.V = params[0].v_reset;
				state.t_end_of_refractory_period = t + params[0].tau_refrac;
				// spike triggered adaptation
				for (size_t ii = 0; ii < state.size(); ++ii) {
					state.denmems[ii].w += params[ii].b;
				}
				// TODO: trigger spike
			}
		}

		assert (nrn < state.size());
		state.denmems[nrn].g_syn[side] += weight;
		cout << "V(" << t << ") = " <<  state.V << endl;
	}
	if (t_end > t) {
		//t += dt*integrate_const(stepper, dynamics, state,t,t_end,dt, push_back_state_and_time( x_vec , times ));
		while (t < t_end) {
			recorder(state, t);
			stepper.do_step(dynamics, state, t, dt);
			t += dt;
			// TODO: should we check for being refractory again?
			if ( state.V >= params[0].v_spike ) {
				state.V = params[0].v_reset;
				state.t_end_of_refractory_period = t + params[0].tau_refrac;
				// spike triggered adaptation
				for (size_t ii = 0; ii < state.size(); ++ii) {
					state.denmems[ii].w += params[ii].b;
				}
				// TODO: trigger spike
			}
		}
	}
	cout << "V(" << t << ") = " <<  state.V << endl;

	write_out("data_adex.txt", x_vec, times);
}


// basic test for integrating CompoundNeuron into systemc
class CompoundNeuronModule : public sc_module
{
public:
	SC_HAS_PROCESS(CompoundNeuronModule);

	CompoundNeuronModule(sc_module_name name, size_t N):
		sc_module(name)
		,mNeuron(N)
		,mClock("mClock",10,SC_US)
		,mLastTime(0)
	{
		SC_METHOD(tick);
		dont_initialize();
		sensitive << mClock.posedge_event();

		mNeuron.setFiringDenmem(0);
		mNeuron.initialize();
	}
	void tick() {
		double CurrentTime = sc_time_stamp().to_seconds();
		// avoid to call update multiple times
		if (CurrentTime > mLastTime ) {
			mNeuron.update(mLastTime, CurrentTime);
			mLastTime=CurrentTime;
		}
	}

	CompoundNeuron mNeuron;
private:
	sc_clock mClock; //!< systemc clock
	double mLastTime;
};

TEST(CompoundNeuronModule, Base)
{
	sc_get_curr_simcontext()->reset();
	CompoundNeuronModule cnm("cnm",2);
	double duration = 1.;
    sc_start(duration,SC_SEC);
	sc_stop();
}
