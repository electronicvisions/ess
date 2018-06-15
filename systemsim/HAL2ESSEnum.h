#pragma once
#include <vector>
#include <utility>
#include <assert.h>

namespace ESS {

enum HICANNSide
{
	S_LEFT = 0,
	S_RIGHT = 1
};

enum HICANNBlock
{
	BL_UP = 0,
    BL_DOWN = 1
};


enum RepeaterLocation
{
	REP_L = 0,  //!< left repeater block (32 repeaters)
	REP_R = 1,  //!< right repeater block (32)
	REP_UL = 2,  //!< upleft repeater block (64)
	REP_DL = 3,  //!< downleft repeater block (64)
	REP_UR = 4,  //!< upright repeater block (64)
	REP_DR = 5  //!< downright repeater block (64)
};

enum size
{
	neurons_on_hicann = 512,
    neurons_per_line = 256,
	hicanns_on_wafer = 384,
	hlines_on_hicann = 64,
	switches_per_row = 4,
	synswitch_rows = 112,
	synswitches_per_row = 16,
	synapses_per_row = 256,
	synapses_per_col = 224,
	neuron_blocks = 8,
	row_neurons_on_block = 32,
	dnc_channels = 8
};

enum DNCIfDirections {
	OFF,
	TO_HICANN,
	TO_DNC
};

} //end namespace ESS
