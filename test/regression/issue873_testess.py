import unittest

class Issue873(unittest.TestCase):
    """
    PyNN script reproducing issue 873: ADEX adaptation current w during refractory period.
    Description
        Currently, in the ESS implementation, w is kept at the reset value (w+b) during refractory period.
        This is different to other simulators (brian, nest, neuron), where w continuously evolves during refractory period.
        This should be adapted.
    Test method:
    We stimulate a neuron to produce a rebound burst (2 spikes).
    However, we delete the 2nd spike by setting the refractory time to 10 ms.
    During refractory time, w decays, such that the 2nd spike is not generated anymore.
    The 2nd spike is still there in the ESS, as w is clamped during refractory period.
    Hence, if there is only one spike, the test is passed.
    (Parameter are fine-tuned and were cross-checked with NEST, NEURON and BRIAN)
    """
    @unittest.skip(("Test uses fine-tuned AdEx parameters, which are no longer "
                    "available in default transformation (delta_T=2.0 ms)"))
    def runTest(self):
        import numpy
        backend = "hardware.brainscales"

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

        neuron_count = 1 # size of the Population we will create

        # Set the neuron model class
        neuron_model = pynn.EIF_cond_exp_isfa_ista # an Adaptive Exponential I&F Neuron

        neuron_parameters = {
         'a'          : 10.0,    # adaptation variable a in nS
         'b'          : 0.0805, # adaptation variable b in pA
         'cm'         : 0.281,  # membrane capacitance nF
         'delta_T'    : 2.0,    # delta_T fom Adex mod in mV, determines the sharpness of spike initiation
         'e_rev_E'    : 0.0,    # excitatory reversal potential in mV
         'e_rev_I'    : -100.0,  # inhibitory reversal potential in mV
         'i_offset'   : 0.0,    # offset current
         'tau_m'      : 9.3667, # membrane time constant
         'tau_refrac' : 8.0,    # absolute refractory period
         'tau_syn_E'  : 5.0,    # excitatory synaptic time constant
         'tau_syn_I'  : 5.0,    # inhibitory synaptic time constant
         'tau_w'      : 144.0,  # adaptation time constant
         'v_reset'    : -70.6,  # reset potential in mV
         'v_rest'     : -70.6,  # resting potential in mV
         'v_spike'    : -40.0,  # spike detection voltage in mV
         'v_thresh'   : -66.9,  # spike initiaton threshold voltage in mV
         }

        # We create a Population with 1 neuron of our 
        N1 = pynn.Population( size = 1, cellclass = neuron_model, cellparams = neuron_parameters)
        N1.initialize('v',neuron_parameters['v_rest'])

        spiketimes = numpy.arange(100.,800.,0.5)
        S1 = pynn.Population(1,pynn.SpikeSourceArray, cellparams = {'spike_times':spiketimes})

        pynn.Projection(S1,N1,pynn.OneToOneConnector(weights=0.0123938304114), target='inhibitory') # max weight in hardware


        # record the spikes of all neurons of the population
        N1.record()

        # run the simulation for 500 ms
        pynn.run(1000.)


        num_spikes = 0
        # After the simulation, we get Spikes and save the voltage trace to a file
        spike_times = N1.getSpikes()
        for pair in spike_times:
            print "Neuron ", int(pair[0]), " spiked at ", pair[1]
            num_spikes += 1

        pynn.end()

        self.assertEqual(int(num_spikes),1,'The Neuron spiked more than once, i.e. there was a rebound burst, and not a single rebound spike as it should be')

if __name__ == "__main__":
    Issue873().runTest()
