import unittest
import numpy

class Issue1129(unittest.TestCase):
    """
    Wrong integration when e_rev_I higher than v_spike.
    """
    def runTest(self):
        import numpy

        import pyNN.hardware.brainscales as pynn


        # set-up the simulator
        pynn.setup(
                timestep=0.1,
                useSystemSim=True,
                speedupFactor=8000,
                ignoreDatabase=True,
                ignoreHWParameterRanges=True,
                hardware=pynn.hardwareSetup['one-hicann'],
                )

        # Set the neuron model class
        neuron_model = pynn.IF_cond_exp # I&F

        neuron_parameters = {
         'cm'         : 0.281,  # membrane capacitance nF
         'e_rev_E'    : 0.0,    # excitatory reversal potential in mV
         'e_rev_I'    : -50.0,  # inhibitory reversal potential in mV (HIGHER THAN v_thresh)
         'i_offset'   : 0.0,    # offset current
         'tau_m'      : 9.3667, # membrane time constant
         'tau_refrac' : 1.0,    # absolute refractory period
         'tau_syn_E'  : 5.0,    # excitatory synaptic time constant
         'tau_syn_I'  : 5.0,    # inhibitory synaptic time constant
         'v_reset'    : -70.6,  # reset potential in mV
         'v_rest'     : -70.6,  # resting potential in mV
         'v_thresh'   : -55.,  # spike initiaton threshold voltage in mV
         }

        # We create a Population with 1 neuron of our 
        N1 = pynn.Population( size = 1, cellclass = neuron_model, cellparams = neuron_parameters)

        spiketimes = numpy.arange(10.,60.,5.)
        S1 = pynn.Population(1,pynn.SpikeSourceArray, cellparams = {'spike_times':spiketimes})

        pynn.Projection(S1,N1,pynn.AllToAllConnector(weights=0.010425507084882381),target='excitatory') # max weight in hardware

        # record the voltage of all neurons of the population
        N1.record()

        # run the simulation for 200 ms
        pynn.run(80.)
        spikes = N1.getSpikes()
        pynn.end()
        self.assertEqual(spikes.shape, (3,2), "Number of spikes is not as expected") # should have 3 spikes!!

if __name__ == "__main__":
    Issue1129().runTest()
