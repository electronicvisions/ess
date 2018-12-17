#!/usr/bin/env python
APPNAME='SystemSim'

import sys, os
from waflib.extras import symwaf2ic

base_dir = symwaf2ic.get_toplevel_path()

def depends(ctx):
    ctx('logger')
    ctx('halbe')
    ctx('calibtic')
    ctx('systemsim-stage2', 'systemc_for_python')
    ctx('odeint-v2')

def options(ctx):
    ctx.load('compiler_cxx')
    ctx.load('boost')

def configure(ctx):
    ctx.load('compiler_cxx')
    ctx.load('boost')

    ctx.check_boost('filesystem system', uselib_store='BOOST4SYSTEMSIM')
    ctx.check_cxx(lib='pthread')

def build(ctx):
    # system simulation sources
    sources = [ ctx.path.find_resource(x) for x in [
        'global_src/systemc/types.cpp',
        'systemsim/ADEX.cpp',
        'systemsim/IFSC.cpp',
        'systemsim/anncore_behav.cpp',
        'systemsim/bg_event_generator.cpp',
        'systemsim/common.cpp',
        'systemsim/dnc_if.cpp',
        'systemsim/dnc_merger.cpp',
        'systemsim/dnc_merger_timed.cpp',
        'systemsim/dnc_ser_channel.cpp',
        'systemsim/dnc_tx_fpga.cpp',
        'systemsim/HALaccess.cpp',
        'systemsim/HAL2ESSContainer.cpp',
        'systemsim/hicann_behav_V2.cpp',
        'systemsim/hw_neuron.cpp',
        'systemsim/hw_neuron_ADEX.cpp',
        'systemsim/hw_neuron_ADEX_clocked.cpp',
        'systemsim/hw_neuron_IFSC.cpp',
        'systemsim/l1_behav_V2.cpp',
        'systemsim/l2_dnc.cpp',
        'systemsim/l2_fpga.cpp',
        'systemsim/l2tol1_tx.cpp',
        'systemsim/lost_event_logger.cpp',
        'systemsim/merger.cpp',
        'systemsim/merger_timed.cpp',
        'systemsim/pcb.cpp',
        'systemsim/priority_encoder.cpp',
        'systemsim/spl1_merger.cpp',
        'systemsim/stage2_virtual_hardware.cpp',
        'systemsim/syndriver.cpp',
        'systemsim/wafer.cpp',
        'systemsim/CompoundNeuron.cpp',
        'systemsim/DenmemIF.cpp',
        'systemsim/DenmemParams.cpp',
        'systemsim/CompoundNeuronModule.cpp',
        ] ]

    includes = [ ctx.path.find_dir(x) for x in [
            'global_src/systemc',
            'systemc_for_python/src',
            '.'
    ] ]
    includes += [os.path.join(base_dir, 'odeint-v2', 'include')]

    ctx(
        target          = 'systemsim',
        features        = 'cxx cxxshlib pyembed',
        source          = sources,
        includes        = includes,
        export_includes = includes,
        use             = ['logger_obj',
                           's2hal_inc',
                           'systemc',
                           'systemc_config',
                           'halbe_container',
                           'hicann_cfg_inc',
                           'hmf_calibration',
                           'BOOST4SYSTEMSIM',
                           'PTHREAD'],
        install_path    = "${PREFIX}/lib",
        defines         = [ 'USE_HAL', 'USE_SCTYPES', 'VIRTUAL_HARDWARE']
    )

    ctx(target          = 'test-systemsim',
        features        = 'cxx cxxprogram gtest',
        source          = ctx.path.ant_glob('test/gtest/*.cpp'),
        install_path    = os.path.join('bin', 'tests'),
        use             = [ 'systemsim' ]
    )
