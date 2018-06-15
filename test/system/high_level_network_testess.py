import unittest
import subprocess

@unittest.skip("HighLevelNetworks tests temporarily disabled. need to update tests in pyNN-hardware to support setup_arguments")
class HighLevelNetworks(unittest.TestCase):
    """
    runs the high-level networks tests for the ess
    which usually are located in 
    $SYMAP2IC_PATH/components/pynnhw/test/high_level_network_tests

    Those were designed to be used with inspector (symap2ic/test/inspector.py),
    but are now wrapped for testing the ESS only.
    TODO:
        * Pass option 'useSystemSim' to in high-level-network tests.
          easiest done by passing a path to a pickle with additional setup params
    """

    """
    def runTest(self):
        self.tests = (
            #("high-level-network-tests.stage2ess.v_thresh", "$SYMAP2IC_PATH/components/pynnhw/test/high_level_network_tests/master.py hardware.brainscales 2000 8 2 1 0 isolated v_thresh 2 1.0"),
            ("high-level-network-tests.stage2ess.v_thresh", "$SYMAP2IC_PATH/components/pynnhw/test/high_level_network_tests/master.py hardware.brainscales 10000 8 2 10 172 isolated v_thresh 3 1.0"),
            ("high-level-network-tests.stage2ess.stimrate", "$SYMAP2IC_PATH/components/pynnhw/test/high_level_network_tests/master.py hardware.brainscales 10000 62 2 10 172 isolated stimrate 3 0.4"),
            ("high-level-network-tests.stage2ess.weight", "$SYMAP2IC_PATH/components/pynnhw/test/high_level_network_tests/master.py hardware.brainscales 10000 62 2 10 172 isolated weight 3 0.4"),
            ("high-level-network-tests.stage2ess.syntype", "$SYMAP2IC_PATH/components/pynnhw/test/high_level_network_tests/master.py hardware.brainscales 10000 62 2 10 172 isolated syntype 3 0.4"),
            ("high-level-network-tests.stage2ess.stimnum", "$SYMAP2IC_PATH/components/pynnhw/test/high_level_network_tests/master.py hardware.brainscales 30000 8 2 10 172 isolated stimnum 3 1.0"),
        )
        for test in self.tests:
            self.assertTrue(self.run_test(test),"Test: {0} failed".format(test[0]))
    """

    def testVThresh(self):
         test = ("high-level-network-tests.stage2ess.v_thresh", "$SYMAP2IC_PATH/components/pynnhw/test/high_level_network_tests/master.py hardware.brainscales 10000 8 2 10 172 isolated v_thresh 3 1.0")
         self.assertTrue(self.run_test(test),"Test: {0} failed".format(test[0]))
    def testStimRate(self):
         test = ("high-level-network-tests.stage2ess.stimrate", "$SYMAP2IC_PATH/components/pynnhw/test/high_level_network_tests/master.py hardware.brainscales 10000 62 2 10 172 isolated stimrate 3 0.4")
         self.assertTrue(self.run_test(test),"Test: {0} failed".format(test[0]))

    def testWeight(self):
         test = ("high-level-network-tests.stage2ess.weight", "$SYMAP2IC_PATH/components/pynnhw/test/high_level_network_tests/master.py hardware.brainscales 10000 62 2 10 172 isolated weight 3 0.4")
         self.assertTrue(self.run_test(test),"Test: {0} failed".format(test[0]))

    def testSynType(self):
         test = ("high-level-network-tests.stage2ess.syntype", "$SYMAP2IC_PATH/components/pynnhw/test/high_level_network_tests/master.py hardware.brainscales 10000 62 2 10 172 isolated syntype 3 0.4")
         self.assertTrue(self.run_test(test),"Test: {0} failed".format(test[0]))

    def testStimNum(self):
         test = ("high-level-network-tests.stage2ess.stimnum", "$SYMAP2IC_PATH/components/pynnhw/test/high_level_network_tests/master.py hardware.brainscales 30000 8 2 10 172 isolated stimnum 3 1.0")
         self.assertTrue(self.run_test(test),"Test: {0} failed".format(test[0]))

    def run_test(self,test):
        """
        Function that runs a test tuple ("test-identifier", "command").
        """
        rv = self.run_command(test[1])
        # return value 0 means success
        if rv == 0:
            return True
        else:
            return False
    def run_command(self,command):
        """
        Function that runs a command (must be a string) using a shell and returns an exit code.
        """

        process = subprocess.Popen(command, shell=True, close_fds=True)
        #process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, close_fds=True)
        process.wait()
        return process.returncode



if __name__ == "__main__":
    unittest.main()
