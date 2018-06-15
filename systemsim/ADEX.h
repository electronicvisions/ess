//_____________________________________________
// Company      :	UNI Heidelberg	
// Author       :	Bernhard Vogginger
// E-Mail   	:	bernhard.vogginger@kip.uni-heidelberg.de
//_____________________________________________

#ifndef _ADEX_H_
#define _ADEX_H_

#include <string>
#include <fstream>

/** class ADEX
 * provides an event-based simulator for a single adaptive-Exponential Integrate&Fire Neuron, see paper from Brette/Gerstner 2005.
 * it uses numerical Integration(implemented Heun's method=2nd order Runge-Kutta) to calculate the time of the next spike(->time_to_next_spike)
 * and updates all Variables(V and w) at an incoming or outgoing spike.
 * g_e and g_i are changed by (add_exc_w) and (add_inh_w), respectively!
 * exponential decaying of conductances of course is considered at integration.
 * The membrane voltage can be recorded.
 */
class ADEX {
	public:
		static const double rel_float_tolerance; ///< relative float tolerance with respect to variable dt, defaults to 1.e-6

		// Neuron parameters
		double C; ///< membrane capacitance
		double g_l; ///< leakage conductance
		double E_l; ///< reversal resting potential
		double E_reset; ///< spike reset potential
		double E_e; ///< excitatroy reversal potential
		double E_i; ///< inhibitory reversal potential
		double tau_syn_e; ///< excitatory synaptic time constant
		double tau_syn_i; ///< inhibitory synaptic time constant
		double V_t; ///< spike initiation threshold
		double D_t; ///< slope factor
		double tau_w; ///< adaption time constant
		double a; ///< adaptation constant a
		double b; ///< adaptation constant b
		double V_spike; ///< Voltage at which an output spike is generated

		int denmemId;

	private:
		// state variables (change)
		double V; ///< membrane voltage
		double w; ///< adaptation variable	
		double g_e; ///< excitatory synaptic conductance
		double g_i; ///< inhibitory synaptic conductance
        double I; ///< current offset due to stimulus

		//  storage of state of variables at t_last
		double V_last; ///< V at t_last
		double w_last; ///< w at t_last
		double t_last; ///< time of last determined update(i.e. at time of incoming or outgoing spike)
		double t_sim; ///< time of last simulated variables(V,w,g_e,g_i refer to this)
		double g_e_last; ///< g_e at t_last
		double g_i_last; ///< g_i at t_last

		// pre-calculation of exponential decay of synaptic conductances
		double g_e_exp_fac; ///< equals Exp(-dt/tau_syn_e)
		double g_i_exp_fac; ///< equals Exp(-dt/tau_syn_i)

		double dt; ///< integration time step (in seconds)

		double total_run_time; ///< total (biological) time run in this Adex
		double extra_time; ///< extra time run for analyze of this pseude-event-based model

		/** the maximum value for dvdt. 
		 * If dv_dt is higher than this during integration, it is set to max_dvdt.
		 * makes sure, that V doesnt go to plus infinity.
		 * is calculated in init method regarding the dynamic range of v and the chosen dt.
		 * must be positive.
		 */
		double max_dvdt;

		/** minimum change of v per timestep to continue calculation of next spike.
		 * when calculating the time of the next spike in time_to_next_spike(), one breaking condition is that the membrane potential
		 * stops increasing. However, we can not just compare for dvdt > 0, fo example if we are in the case that we are 
		 * below resting potential and now decay to resting potential. To avoid infinite integration we have to set a
		 * finite (non-zero) break condition.
		 * calculated in method init.
		 * must be positive.
		 */
		double min_dvdt;

		/** as for min_dvdt we now investigate the contribution of the adaptation current w onto the change of the membrane voltage.
		 * We consider this additionally to the increase of dv_dt, as the adaption usually has the longer time constant.
		 * So for the short time, the membrane might decrease but after some ms it will increase, due to the effect of the 
		 * adaption current. This happen e.g. at inhibitory rebound spikes.
		 * For this case, the adaptation current has to be negative(thats why I call it max_*). 
		 * And also this value is negative and is set to (-1 * min_dvdt*C), i.e. we check if the increase of the w-contribution of dv/dt is higher than min_dvdt.
		 */
		double max_w_influence_on_dvdt;

        /** Minimum constant current for which a spike occurs: We consider the LIF with constant current:
         * C dV/dt = -g_l (V - E_l) + I
         * The stationary solution is: V_inf = I / g_l + E_l 
         * A spike occurs if V_inf > min(V_thresh,V_spike)
         * -> I_min = g_l (min(V_thresh,V_spike) - E_l) */
        double I_min;

		/** epsilon used to compare equality of t_sim and time_to_run in method run().
		 * This is needed as machine errors add up, so we want to compare double only with a certain precision.
		 * Is set to rel_float_tolerance*dt in init()
		 */
		double t_comp_epsilon;
		std::ofstream voltage_file; ///< outstream to voltage recording file
		bool rec_voltage;  ///< record voltages

		/// standard constructor
		ADEX();

	public:

		ADEX(double _V, double _w);
		/// standard destructor
		~ADEX();

	public:
		void set_V(double _V); //!< sets V to a given value (also V_last is set to this value)
		void set_w(double _w); //!< sets w to a given value (also w_last is set to this value)
		/** adds excitatory weight to g_e */
		void add_exc_w(double weight);
		/** adds inhibitory weight to g_e */
		void add_inh_w(double weight);
		double get_t_sim() const {return t_sim;}
		double get_t_total() const {return total_run_time;}
		/** returns the to the next spike.
		 * integrates until V either reaches spike threshold or dVdt < 0, returns -1 if there is no spike or time of next spike(t_sim)*/
		double time_to_next_spike();
		/** integrates differential equations for time "time_to_run". During this there usually should be no spike!" 
		 * If the neuron is in refractory period, the state variables are advanced as usual, with the only difference that the voltage is clamped to the reset value
		 * @param time_to_run Time(in biological time) to run
		 * @param is_refractory specify, whether neuron is in refractory period
		 * */
		void run(double time_to_run, bool is_refractory=false);
		/** initializes this class. This has to be called before start of simulation
		 * @param delta_t integration time step (in biological time)
		 * @param rec true = record membrane voltage 
		 * @param fn filename for membrane voltage recording
		 */
		void init(double delta_t, bool rec, std::string fn, int id = -1);
		/** updates Variables at an Outgoing Spike and possibly records voltage up to actual time
		 * sets V back to V_reset and adds adaption constant b to w.
		 * resets internal simulated(t_sim) time to zero and updates *_last variables
		 */
		void OutgoingSpike();
        void set_current_offset(double const current);
	private:
		/** returns dV/dt according to ADEX-model with delivered V and w. needed by HEUN_method
		 * @param _V membrane Voltage V 
		 * @param _w adaptation Current w
		 * @param _g_e excitatory conductance
		 * @param _g_i inhibitory conductance
		 */
		double dV_dt(double _V, double _w, double _g_e, double _g_i) const;
		/** returns dw/dt according to ADEX-model with delivered V and w. needed by HEUN_method
		 * @param _V membrane Voltage V 
		 * @param _w adaptian Current w
		 */
		double dw_dt(double _V, double _w) const;
		/** calculates dv/dt and dw/dt needed for numerical integration according to Heun's method.
		 * This provides a better integration than for example explicit-Euler-method(uses just dV_dt() from above)
		 * for more information see: http://en.wikipedia.org/wiki/Heun's_method
		 * @param dvdt reference to dV/dt (return value)
		 * @param dwdt reference to dw/dt (return value)
		 * @param clamp_V if true, clamp voltage, i.e. use the same voltage for first and second HEUN part (used during refractory period)
		 */
		void HEUN_method(double & dvdt, double & dwdt, bool clamp_V=false) const;

		/** advance state by one time step.
		 * updates variables V, w, g_e, g_i and t_sim.
		 * @param dvdt change of V to be applied
		 * @param dwdt change of w to be applied
		 */
		void advance_one_step( double dvdt, double dwdt );


		/** resets the state variables to the last secure(determined) time.
		 * i.e. sets V to V_last, etc. and t_sim to 0.
		 */
		void reset_to_last_secure_state();
		/** sets the current state to be a secure state.
		 * i.e. sets V_last to V, w_last to V, and so on.
		 * Also resets t_sim to 0.
		 */
		void set_secure_state();

};
#endif // _ADEX_H_
