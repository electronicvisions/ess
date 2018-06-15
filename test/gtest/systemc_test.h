#pragma once
#include "systemc.h"
#include <gtest/gtest.h>

/// Base class for all tests using SystemC tests
/// resets system kernel
class SystemCTest : public ::testing::Test {
protected:
	virtual void SetUp () {
		sc_get_curr_simcontext()->reset();
	}
};

