#include <gtest/gtest.h>

#include "systemsim/HAL2ESSContainer.h"
#include "systemsim/DenmemParams.h"

TEST(DenmemParams, BioToHWToBio)
{
	typedef PyNNParameters::EIF_cond_exp_isfa_ista pynn_adex;
	pynn_adex pynn;
	ESS::BioParameter bio;
	bio.initFromPyNNParameters(pynn);
	DenmemParams hw(bio);
	ESS::BioParameter bio2 = hw.toBioParameter();
	pynn_adex pynn2 = bio2; // upcast
	const double abs_err = 1.e-6;
	ASSERT_NEAR( pynn.a          , pynn2.a, abs_err);
	ASSERT_NEAR( pynn.b          , pynn2.b, abs_err);
	ASSERT_NEAR( pynn.cm         , pynn2.cm, abs_err);
	ASSERT_NEAR( pynn.delta_T    , pynn2.delta_T, abs_err);
	ASSERT_NEAR( pynn.e_rev_E    , pynn2.e_rev_E, abs_err);
	ASSERT_NEAR( pynn.e_rev_I    , pynn2.e_rev_I, abs_err);
	ASSERT_NEAR( pynn.tau_m      , pynn2.tau_m, abs_err);
	ASSERT_NEAR( pynn.tau_refrac , pynn2.tau_refrac, abs_err);
	ASSERT_NEAR( pynn.tau_syn_E  , pynn2.tau_syn_E, abs_err);
	ASSERT_NEAR( pynn.tau_syn_I  , pynn2.tau_syn_I, abs_err);
	ASSERT_NEAR( pynn.tau_w      , pynn2.tau_w, abs_err);
	ASSERT_NEAR( pynn.v_reset    , pynn2.v_reset, abs_err);
	ASSERT_NEAR( pynn.v_rest     , pynn2.v_rest, abs_err);
	ASSERT_NEAR( pynn.v_spike    , pynn2.v_spike, abs_err);
	ASSERT_NEAR( pynn.v_thresh   , pynn2.v_thresh, abs_err);
	ASSERT_NEAR( bio.g_l         , bio2.g_l, abs_err);
}
