#pragma once
#include <stddef.h>

class CompoundNeuron;
class DenmemParams;

/// Interface of a single Denmem.
/// Allows to set parameters of a denmem and to inject spikes.
class DenmemIF {
public:
	/// constructor
	/// @param compound_neuron associated compound neuron
	/// @param id ID of this denmen in the compound neuron.
	DenmemIF(CompoundNeuron & compound_neuron, size_t id);

	// set/get denmem parameters of this denmem to/from associated compound neuron.
	void		 setDenmemParams(DenmemParams const & params);
	DenmemParams getDenmemParams() const;

	/// add weight `weight` (in Siemens) to synaptic conductance `input` of the
	/// denmem
	void inputSpike(bool input, double weight);

	/** update amplitude of external current injected into neuron.
	 * @params current current amplitude in Ampere
	 * */
	void inputCurrent(double const current);

private:
	CompoundNeuron & mCompoundNeuron; //!< associated compound neuron
	const size_t mId; //!< id of denmem in the compound neuron
};
