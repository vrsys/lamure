import unittest
from time import time

from pypro.formats_bridge import *


class NVMFormatSparseTestSuite(unittest.TestCase):
    def setUp(self):
        print("ProFormatsTestSuite : setUp")
        init_time = time()
        self.format_sparse_nvm = FormatSparseNVMV3('/home/anton/Downloads/mvs_output/out_1.nvm')
        elapsed_time = time() - init_time
        print("ProFormatsTestSuite : read took " + str(elapsed_time) + " sec")

    def tearDown(self):
        print("ProFormatsTestSuite : tearDown")

    def test_formats(self):
        print("ProFormatsTestSuite : test_formats")
        out_time = time()
        self.format_sparse_nvm.out()
        elapsed_time = time() - out_time
        print("ProFormatsTestSuite : write took " + str(elapsed_time) + " sec")


if __name__ == '__main__':
    unittest.main()
