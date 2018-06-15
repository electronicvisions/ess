#include "systemc_test.h"

class SystemC : public SystemCTest {};

TEST_F(SystemC, Test0) {
	double duration = 10.;
    sc_start(duration,SC_NS);
	sc_stop();
}

TEST_F(SystemC, Test1) {
	double duration = 10.;
    sc_start(duration,SC_NS);
	sc_stop();
}
