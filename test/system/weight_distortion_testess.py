#!/usr/bin/python
import unittest
import numpy as np

class WeightDistortionTest(unittest.TestCase):
    """
    Tests Weight Distortion feature of ESS
    1.) Run: no distortion
    2.) Run: with distortion, response of two neurons to identical input
        spiketrain is no longer identical
    """

    def runTest(self):
        n0,n1 = self.run_experiment()
        self.assertEqual(n0,n1)
        n0,n1 = self.run_experiment(weight_distortion=0.5)
        self.assertNotEqual(n0,n1)

    def run_experiment(self, weight_distortion=None):
        import pyNN.hardware.brainscales as pynn
        setup_params = dict(useSystemSim = True,
                ignoreDatabase=True,
                ignoreHWParameterRanges=True,
                hardware=pynn.hardwareSetup["one-hicann"],
                rng_seeds=[123567],
                )

        if weight_distortion is not None:
            setup_params['ess_params'] = {'weightDistortion':weight_distortion}

        pynn.setup(**setup_params)
        num_neurons = 2
        pop = pynn.Population(num_neurons, pynn.IF_cond_exp)
        # two different input spike trains, with identical rate but shifted to avoid loss in merger tree
        spike_times1 = np.arange(100.,500,10.)
        spike_times2 = np.arange(105.,505,10.)
        pop_source = pynn.Population(2, pynn.SpikeSourceArray)
        pop_source.tset('spike_times',[spike_times1,spike_times2])

        weight = 0.1 # weight = pop[0].cell.getConductanceRange("weight")[1]
        conn = pynn.OneToOneConnector(weights=weight)
        proj_0=pynn.Projection(pop_source,pop, conn)

        pop.record()
        pynn.run(600.)

        spikes = pop.getSpikes()
        spikes_nrn_0 =  spikes[ spikes[:,0]==0,1]
        spikes_nrn_1 =  spikes[ spikes[:,0]==1,1]

        pynn.end()
        return len(spikes_nrn_0),len(spikes_nrn_1)

if __name__ == "__main__":
    WeightDistortionTest().runTest()
