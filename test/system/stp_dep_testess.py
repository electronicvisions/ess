import unittest
import numpy


# helper functions
def extract_max_in_time_series(values, dt,t_start=None, t_stop=None):
    idx_start = int(t_start/dt) if t_start is not None else 0
    idx_stop = int(t_stop/dt) if t_stop is not None else None
    return numpy.max(values[idx_start:idx_stop])


class STPDepressionTest(unittest.TestCase):
    """
    Test for the short term plasticity
    Here: Tests if depression works.
    5 Spikes are generated.
    The EPSPs are supposed to get smaller everytime
    """
    def runTest(self):
        backend = "hardware.brainscales"
        #backend = "nest"

        exec('import pyNN.%s as pynn' % backend)

        dt = 0.1

        # set-up the simulator
        pynn.setup(timestep=dt,
                useSystemSim=True,
                speedupFactor=10000,
                ignoreDatabase=True,
                ignoreHWParameterRanges=True,
                hardware=pynn.hardwareSetup['one-hicann'],
                )

        neuron_count = 1 # size of the Population we will create

        # Set the neuron model class
        neuron_model = pynn.IF_cond_exp # an Adaptive Exponential I&F Neuron

        # neuron parameters
        neuron_parameters = {
         'cm'         : 0.281,  # membrane capacitance nF
         'e_rev_E'    : 0.0,    # excitatory reversal potential in mV
         'e_rev_I'    : -100.0,  # inhibitory reversal potential in mV
         'i_offset'   : 0.0,    # offset current
         'tau_m'      : 9.3667, # membrane time constant
         'tau_refrac' : 1.0,    # absolute refractory period
         'tau_syn_E'  : 6.1,    # excitatory synaptic time constant
         'tau_syn_I'  : 6.1,    # inhibitory synaptic time constant
         'v_reset'    : -70.6,  # reset potential in mV
         'v_rest'     : -70.6,  # resting potential in mV
         'v_thresh'   : -55.,  # spike initiaton threshold voltage in mV
         }


        # We create a Population with 1 neuron of our 
        N1 = pynn.Population( size = 1, cellclass = neuron_model, cellparams = neuron_parameters)
        N1.initialize('v', neuron_parameters['v_rest'])

        spiketimes = numpy.arange(10.,200.,40.)
        S1 = pynn.Population(1,pynn.SpikeSourceArray, cellparams = {'spike_times':spiketimes})
        # We use a parameter set that is mapped without distortions
        U_SE = 3./11
        STD_1=pynn.TsodyksMarkramMechanism(U=U_SE, tau_rec=200.0, tau_facil=0.0)
        SD = pynn.SynapseDynamics(fast=STD_1)
        # max_weight =  N1[0].cell.getConductanceRange('weight')[1]
        # We choose some weight that fits into range
        max_weight = 0.0025

        pynn.Projection(S1,N1,pynn.OneToOneConnector(weights=max_weight/U_SE), synapse_dynamics=SD,target='excitatory') # max weight in hardware

        # record the spikes of all neurons of the population
        N1.record()
        N1.record_v()

        # run the simulation for 500 ms
        pynn.run(210.)
        vs = N1.get_v(compatible_output=True).transpose()[2]
        doPlot=False
        if doPlot:
            import pylab
            pylab.plot(vs)
            pylab.show()
        pynn.end()
        maximas = []
        for i in range(len(spiketimes)-1):
            maximas.append(extract_max_in_time_series(vs,dt,spiketimes[i], spiketimes[i+1]))
        maximas.append(extract_max_in_time_series(vs,dt,spiketimes[-1]))
        print maximas
        self.assertEqual(len(maximas),5,'There should be 5 EPSPs')
        for i in range(len(maximas)-1):
            self.assertTrue(maximas[i] > maximas[i+1],'The heights of the EPSPs shoud be decreasing')

if __name__ == "__main__":
    STPDepressionTest().runTest()
