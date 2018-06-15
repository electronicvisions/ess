import unittest

class Issue845(unittest.TestCase):
    """
    Voltage Recording.
    """
    def runTest(self):
        import numpy

        import pyNN.hardware.brainscales as pynn


        # set-up the simulator
        pynn.setup(
                timestep=0.1,
                useSystemSim=True,
                speedupFactor=10000,
                ignoreDatabase=True,
                ignoreHWParameterRanges=True,
                hardware=pynn.hardwareSetup['one-hicann'],
                )

        # Set the neuron model class
        neuron_model = pynn.IF_cond_exp # I&F

        neuron_parameters = {
         'cm'         : 0.281,  # membrane capacitance nF
         'e_rev_E'    : 0.0,    # excitatory reversal potential in mV
         'e_rev_I'    : -100.0,  # inhibitory reversal potential in mV
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
        N1 = pynn.Population( size = 2, cellclass = neuron_model, cellparams = neuron_parameters)
        N1a = N1[0:1]
        N1b = N1[1:2]
        v_shift = 5.
        # shift all voltages of 2nd neuron  by 5 mV
        N1b.set('v_rest', neuron_parameters['v_rest']-v_shift)
        N1b.set('v_reset', neuron_parameters['v_reset']-v_shift)
        N1b.set('v_thresh', neuron_parameters['v_thresh']-v_shift)
        N1b.set('e_rev_E', neuron_parameters['e_rev_E']-v_shift)
        N1b.set('e_rev_I', neuron_parameters['e_rev_I']-v_shift)

        spiketimes = [10., 50., 110., 122.5]
        S1 = pynn.Population(1,pynn.SpikeSourceArray, cellparams = {'spike_times':spiketimes})

        pynn.Projection(S1,N1,pynn.AllToAllConnector(weights=0.0123938304114), target='inhibitory') # max weight in hardware


        # record the voltage of all neurons of the population
        N1.record_v()

        # run the simulation for 200 ms
        pynn.run(150.)

        # format of get_v: [id, t, v]
        mean_v_a = N1a.get_v().transpose()[2].mean()
        mean_v_b = N1b.get_v().transpose()[2].mean()
        self.assertAlmostEqual(mean_v_a, mean_v_b + v_shift, places=3,msg='The mean voltage of neuron a and b should have a difference of 5 mV')
        pynn.end()

if __name__ == "__main__":
    Issue845().runTest()
