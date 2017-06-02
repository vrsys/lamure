import unittest

from pypro.entities_dense import *
from pypro.entities_sparse import *
from pypro.formats import *


class ProFormatsTestSuite(unittest.TestCase):
    def setUp(self):
        print("ProFormatsTestSuite : setUp")
        self.format_sparse_pro = FormatSparsePro()
        self.format_sparse_pro.cameras.append(
            Camera(0,
                   0.5,
                   Quat(0.1, 0.1, 0.1, 0.1),
                   Position(0.8, 0.8, 0.8),
                   "path",
                   MetaData(bytes([0xCC, 0xCC]))))
        self.format_sparse_pro.spoints.append(
            SparsePoint(0,
                        Position(0.2, 0.2, 0.2),
                        Color(1, 1, 1),
                        MetaData(bytes([0xDD, 0xDD])),
                        [Measurement(0, 0.3, 0.3)])
        )
        self.format_dense_pro = FormatDensePro()
        self.format_dense_pro.dpoints.append(
            DensePoint(0,
                       Position(0.2, 0.2, 0.2),
                       Color(1, 1, 1),
                       MetaData(bytes([0xBB, 0xBB])),
                       Normal(0.7, 0.7, 0.7))
        )

    def tearDown(self):
        print("ProFormatsTestSuite : tearDown")

    def test_formats(self):
        print("ProFormatsTestSuite : test_formats")
        self.format_sparse_pro.out()
        self.format_dense_pro.out()

if __name__ == '__main__':
    unittest.main()
