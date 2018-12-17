//_____________________________________________
// Company      :	uhei
// Author       : 	Johannes Fieres
// E-Mail   	:	fieres@kip.uni-heidelberg.de
// last change	:	2009/07/03 bvogging
//_____________________________________________
//

#ifndef ANNCORE_BEHAV_H
#define ANNCORE_BEHAV_H

#include "systemc.h"

#include "mutexfile.h"
#include "anncore_task_if.h"
#include "anncore_pulse_if.h"
#include "syndriver.h"
#include "CompoundNeuronModule.h"
#include "DenmemIF.h"
#include "HAL2ESSContainer.h"

#include <vector>
#include <map>

// pre-declarations
class hw_neuron;

/** This class represents the detailed behavioural description of the
anncore. The TLM interface anncore_task_if is implemented.
*/
class anncore_behav :
			public sc_module,
			public anncore_task_if
{

	private:
		/** ID of this ANNCORE in the wafer-system*/
		short _anncore_id;	// anncore_id
		/** flag for spike_debugging **/
        bool _spike_debugging;
        /** count of used(configured) hw_neurons */
		unsigned int _hw_neuron_count;
		/** Flag indicating whether hw_neurons already have been created.*/
		bool _hw_neurons_created;
		/** count of mapped biological neurons on this anncore */
		unsigned int _bio_neuron_count;
		/** all synaptic weights (0..15), row-first order*/
		std::vector<char> _weights;
		/** all synaptic 6-bit addressed, row-first order*/
		std::vector<char> _addresses;
		/** fixed pattern noise of synaptic weights, row-first order*/
		std::vector<float> _weight_distortions;
		/** all syndrivers, in their geometric order. Top to bottom, left to right, i.e
		0...111 = left, 112...223: right.*/
		std::vector<syndriver> _syndrivers;
		/** this vec holds the syndriver mirror settings. This is redundant because this info
		is also stored in the elements of <tt>syndrivers</tt> but this one is easier to
		return by getSyndriverMirrors()*/
		std::vector<char> _syndriver_mirrors;

		/** map of firing denmems to hw neurons */
		std::map<unsigned int, hw_neuron*> _firing_denmem_2_hw_neuron; // TODO: removeme

		/** all synpse columns of the array, upper block first*/
		std::array< std::unique_ptr<DenmemIF>, 512 > _denmems;
		std::vector< CompoundNeuronModule* > _compound_neurons;
		/** map of firing denmems to compound neurons */
		std::map<unsigned int, CompoundNeuronModule*> _firing_denmem_2_compound_neuron;

        //**********************************
        //Implementation of Current Stimulus
        //**********************************
        
        sc_clock _clock;

        //struct for current stimulus
        struct CurrentStimulus
        {
            bool enable;                              // enable stimuli from this FGBlock
            bool continuous;                          // flag for continuous current stimuli
            uint8_t period;                           // stores how long a single current value is delivered
            std::array<uint16_t, 129> Currents;       // stores the current values

            size_t position;                          // current position in Currents array
            size_t clk_counter;                       // current clk-count (is reset every period)
            std::vector<size_t> neurons;              // stores the addresses of neurons which receive current input from this block (i.e. the denmem id in range 0..511)
        
            const double scaling = 2500./1023. * 1e-9;// scaling factor from DAC to nA and from nA to A
        };

        std::array<CurrentStimulus, 4> _fg_stim;      // the 4 stimulus container
        unsigned long _global_cnt;                    // counts the clock periods, needed to detect first clock cycle in check_for_stimchange

        //checks for changing current stimuli and change current offset of neuron in case 
        void check_for_stimchange();
        
        //updates the stimulus every time stim.clk_counter == stim.period
        void update_stim(CurrentStimulus & stim);
        
        //initializes the stimulus at the first clock cycle
        void init_stim(CurrentStimulus & stim);
        
	public:
        // write all denmem that belong to the hw neuron with hw_neuron_id to the set connected_denmems
        // this information comes from the map denmems2nrns
        void getConnectedDenmems(   std::set<unsigned int>& connected_denmems,
                                    std::map<unsigned int, unsigned int> const & denmems2nrns,
                                    unsigned int const hw_neuron_id,
                                    unsigned int const denmem_id) const;
    protected:
		ess::mutexfile& spike_rx_file;	///<FILE handle for recording of received events

	public:
	    /// SC_HAS_PROCESS needed for declaration of SC_METHOD if not using SC_CTOR for current stimuli
        SC_HAS_PROCESS(anncore_behav);
		/** constructor of anncore_behav.*/
		anncore_behav (
				const sc_module_name& anncore_i,    //!< module instance name
				short _anncore_id,                  //!< ID of this ANNCORE
				ess::mutexfile& spike_rcx_file,     //!< FILE to handle recording of received events
				uint8_t PLL_period_ns,              //!< Period of the PLL
                bool enable_spike_debugging         //!< Flag for spike_debugging
                );
		~anncore_behav();

		
        /** Pulse interface (out). Def'd in anncore_pulse_if.
		 * This is now implemented by the spl1_merger, but we need to pass it to the neurons. Thats why it is still hee.*/
		sc_port<anncore_pulse_if> out_port;

		/** pulse interface (in). Def'd in anncore_task_if. */
		void recv_pulse(unsigned int syndriver_id, unsigned int addr);

        //new functions added by Constantin Pape
		
        //returns a hw_neurons
		hw_neuron * get_neuron(unsigned int nrn);
		hw_neuron const* get_neuron(unsigned int nrn) const;

        syndriver & get_syndriver(unsigned int drv);
        syndriver const& get_syndriver(unsigned int drv) const;
        
        //sets the configuration of the current stimulus for a single FGBlock
        void configCurrentStimulus(ESS::StimulusContainer const& stim, ESS::HICANNSide side, ESS::HICANNBlock block);
        
        //activates the current input of denmem
        void setCurrentInput(unsigned int denmem);
        //end of new functions

		// implementing anncore_task_if:
		/** doc in anncore_task_if*/
		virtual void configWeights(const std::vector<char> &weights);
		/** doc in anncore_task_if*/
		virtual void configWeight(int x, int y, sc_uint<SYN_NUMCOLS_PER_ADDR*SYN_COLDATA_WIDTH> weight);
		/** doc in anncore_task_if*/
		virtual void configAddressDecoders(const std::vector<char> &addr);
		/** doc in anncore_task_if*/
		virtual void configAddressDecoder(int, int, sc_uint<SYN_NUMCOLS_PER_ADDR*SYN_COLDATA_WIDTH>){};
		/** doc in anncore_task_if*/
		virtual void configSyndriver(
				unsigned int syndr_id, //!< ID of Syndriver to be configured
				const ESS::syndriver_cfg& syndriver_cfg,
				const std::vector<double>& synapse_trafo_up,
				const std::vector<double>& synapse_trafo_down,
				double tau_rec, //!< recovery time constant in s
				double lambda, //!< stp model parameter lambda
				double N_dep, //!< model parameter N in depression mode
				double N_fac //!< model parameter N in facilitation mode
				);

		/** doc in anncore_task_if*/
		virtual const sc_uint<SYN_NUMCOLS_PER_ADDR*SYN_COLDATA_WIDTH> getWeight(int x, int y) const;
		/** doc in anncore_task_if*/
		virtual const std::vector<char>& getWeights() const;
		//    /** doc in anncore_task_if*/
		//    virtual const sc_uint<SYN_NUMCOLS_PER_ADDR*SYN_COLDATA_WIDTH>& getAddressDecoder();
		/** doc in anncore_task_if*/
		virtual const std::vector<char>& getAddressDecoders() const {return _addresses;}
		/** doc in anncore_task_if*/
		virtual bool getSyndriverMirror(int side, int position) const;
		/** doc in anncore_task_if*/
		virtual const std::vector<char>& getSyndriverMirrors() const {return _syndriver_mirrors;}

		/** Instantiates and initalizes as many Neurons as required.
		 * This method must be called before end_of_elaboration as it instantiates new sc_modules(hw_neuron).
		 * connects Synapse columns to corresponding hw_neurons. At the moment a single hw_neuron simulates a group of denmems, so it needs to get all of their inputs.
		 * Creates a hw_neuron only for the denmem that fires, and connects the other synapse columns to this hw_neuron.
         * The information, which denmems connect to the logical neuron is obtained from the denmems2nrns map.
		 */
		unsigned int initializeNeurons(
				const std::map< unsigned int, unsigned int >& firing_denmems_and_addresses, //!< firing denmems and associated l1 addresses.
				const std::map< unsigned int, unsigned int >& denmems2nrns                  //!< maps denmem to hardware neurons.
                );

		/// configure the analog parameters of a single denmem
		void configSingleDenmem(
				unsigned int denmem_id, //< id of denmem to be configured.
				ESS::BioParameter& _neuron_parameter //!< neuron parameters
				);

		/** get biological neuron parameters of a denmem.
		 * returns PyNN like parameters in hardware voltage and time domain.*/
        ESS::BioParameter get_neuron_parameter(
				unsigned int denmem_id //< id of denmem
				) const;

		/** get 6 bit l1 address of a denmem.
		 * only works for firing denmems.*/
		unsigned int get_6_bit_address(
				unsigned int denmem_id //< id of denmem
				) const;

		/** configure a compound neuron.
		 * denmem id refers to neuron, that can fire. (there is one compound
		 * neuron for each firing denmem, that simulates a group of connected
		 * denmems implementing one neuron.)*/
		void configCompoundNeuron(
				unsigned int denmem_id, //< id of firing denmem.
				unsigned int addr,  //!< 9-bit address: first 3-bit for wta, and 6-bit for l1-address on wta.
				bool rec,  //!< if flag is true, voltage of this neuron will be recorded.
				std::string fn,  //!< string of filename, to which the voltage shall be recorded.
				double dt  //!< integration time-step for this neuron
				);

		/** Print Configuration  of Anncore to a file for Debugging */
		int print_cfg(
				std::string fn //!< string of filename, to which debug information shall be printed
				);
		/** set fixed pattern noise of synaptic weights*/
		void set_weight_distortion(double mean, double stddev);

        void disable_weight_distortion();

};
#endif //ANNCORE_BEHAV_H
