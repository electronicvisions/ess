#include "systemsim/DenmemIF.h"
#include "systemsim/CompoundNeuron.h"

DenmemIF::DenmemIF(CompoundNeuron & compound_neuron, size_t id):
	mCompoundNeuron(compound_neuron), mId(id) {}

void
DenmemIF::setDenmemParams(DenmemParams const & params) {
	mCompoundNeuron.setDenmemParams(mId, params);
}


DenmemParams
DenmemIF::getDenmemParams() const {
	return mCompoundNeuron.getDenmemParams(mId);
}

void
DenmemIF::inputSpike(bool input, double weight) {
	mCompoundNeuron.inputSpike(mId, input, weight);
}

void
DenmemIF::inputCurrent(double const current) {
	mCompoundNeuron.inputCurrent(mId, current);
}
