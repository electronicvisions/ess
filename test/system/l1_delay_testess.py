#!/usr/bin/python
import unittest

class MergerTreeDelayTest(unittest.TestCase):
    """
    Test for the l1_delay, which is modelled at the neuron
    The delay should be dependent on the path taken through the merger tree.
    I.e. the path via the central merger chain should take longest.
    """

    def runTest(self):
        import pyNN.hardware.brainscales as pynn
        from pyNN.hardware.brainscales import mapper
        import os

        place = mapper.place()

        current_directory = os.path.dirname(os.path.realpath(__file__))
        pynn.setup(timestep=0.1,
                useSystemSim = True,
                algoinitFilename= current_directory + "/algoinit_l1_delay.pl",
                mappingStrategy="user",
                loglevel=2,
                hardware=[dict(setup="wafer", wafer_id=0,hicannIndices=[279,280])],
                hardwareNeuronSize=1,
                logfile='logfile.txt',
                speedupFactor=10000,
                rng_seeds=[123567],
                ignoreHWParameterRanges=True,
                ignoreDatabase=True,
                tempFolder="debug_l1_delay",
                )
        LIF_nrn_params = {
         'cm': 1.0,
         'e_rev_E': 0.0,
         'e_rev_I': -70.0,
         'i_offset': 0.0,
         'tau_m': 20.0,
         'tau_refrac': 5., # high value to avoid 2nd spike
         'tau_syn_E': 5.0,
         'tau_syn_I': 5.0,
         'v_reset': -65.0,
         'v_rest': -65.0,
         'v_thresh': -60.0} # low value for 1 spike

        num_pe = 8 # number of priority encoders
        pop_1 = pynn.Population(num_pe, pynn.IF_cond_exp, LIF_nrn_params)
        pop_2 = pynn.Population(num_pe, pynn.IF_cond_exp, LIF_nrn_params)
        pop_source = pynn.Population(num_pe, pynn.SpikeSourceArray)

        spike_times = [[i*10. + 10.] for i in range(num_pe)]
        pop_source.tset('spike_times',spike_times)
        weight = 0.05
        conn = pynn.OneToOneConnector(weight,0.1)
        proj_0=pynn.Projection(pop_source,pop_1, conn)
        proj_1=pynn.Projection(pop_1,pop_2, conn)
        for i in range(num_pe):
            place.to(pop_1[i], hicann=279, neuron=32*i)
            place.to(pop_2[i], hicann=279, neuron=32*i +1)
        place.commit()

        pop_1.record()
        pop_2.record()

        pynn.run(100.)

        spikes_1 = pop_1.getSpikes()
        spikes_2 = pop_2.getSpikes()
        self.assertEqual(8,len(spikes_1))
        self.assertEqual(8,len(spikes_2))

        # sort spikes by ID:
        spikes_1.sort()
        spikes_2.sort()
        # spiketimes only
        spikes_1 = spikes_1[:,(1,)]
        spikes_2 = spikes_2[:,(1,)]
        delays = spikes_2 - spikes_1

        # delay pe 1 > 0, 2, 4, 7
        self.assertTrue(delays[1] > delays[0])
        self.assertTrue(delays[1] > delays[2])
        self.assertTrue(delays[1] > delays[4])
        self.assertTrue(delays[1] > delays[7])

        # delay pe 6 > 0, 2, 4, 7
        self.assertTrue(delays[6] > delays[0])
        self.assertTrue(delays[6] > delays[2])
        self.assertTrue(delays[6] > delays[4])
        self.assertTrue(delays[6] > delays[7])

        # delay pe 5 > than everything expect 3
        self.assertTrue(delays[5] > delays[0])
        self.assertTrue(delays[5] > delays[1])
        self.assertTrue(delays[5] > delays[2])
        self.assertTrue(delays[5] > delays[4])
        self.assertTrue(delays[5] > delays[6])
        self.assertTrue(delays[5] > delays[7])

        # delay pe 3 > than everything
        self.assertTrue(delays[3] > delays[0])
        self.assertTrue(delays[3] > delays[1])
        self.assertTrue(delays[3] > delays[2])
        self.assertTrue(delays[3] > delays[4])
        self.assertTrue(delays[3] > delays[5])
        self.assertTrue(delays[3] > delays[6])
        self.assertTrue(delays[3] > delays[7])

if __name__ == "__main__":
    MergerTreeDelayTest().runTest()
