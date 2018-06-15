import unittest
import xmlrunner
import os

path2testscripts = os.path.dirname(os.path.join(os.getcwd(), os.sys.argv[0]))
suite = unittest.TestLoader().discover(path2testscripts,pattern='*_testess.py')
xmloutputdir = os.path.join(path2testscripts, 'testrunner')
print 'xmloutput goes to: ',xmloutputdir
xmlrunner.XMLTestRunner(output=xmloutputdir).run(suite)
