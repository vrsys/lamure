import math
import unittest
from random import random


class GenerateTestPlane(unittest.TestCase):
    def runTest(self, result=None):
        out_xyz_all = ""

        for i in range(1, 500):
            for j in range(1, 100):

                out_xyz_all += str("%.6f" % float(i / 50.0 + 2 * (random() - 0.5) * (i / 500.0))) + " "
                out_xyz_all += str("%.6f" % float(j / 50.0 + 2 * (random() - 0.5) * (i / 500.0))) + " "
                out_xyz_all += str("%.6f" % float(2 * (random() - 0.5) * (i / 500.0))) + " "

                nx = 2.0 * (random() - 0.5) * (i / 500.0)
                ny = 2.0 * (random() - 0.5) * (i / 500.0)
                nz = 2.0 * (random() - 0.5) * (i / 500.0)

                norm = nx * nx + ny * ny + nz * nz

                nx /= math.sqrt(norm)
                ny /= math.sqrt(norm)
                nz /= math.sqrt(norm)

                out_xyz_all += str("%.6f" % nx) + " "
                out_xyz_all += str("%.6f" % ny) + " "
                out_xyz_all += str("%.6f" % nz) + " "

                out_xyz_all += str(int((nx + 1.0) / 2.0 * 255)) + " "
                out_xyz_all += str(int((ny + 1.0) / 2.0 * 255)) + " "
                out_xyz_all += str(int((nz + 1.0) / 2.0 * 255)) + " "

                out_xyz_all += str("%.6f" % float(random() * (i / 500.0)))

                out_xyz_all += "\n"

        out_file = open('test_plane.xyz_all', 'w')
        out_file.write(out_xyz_all)
        out_file.close()


if __name__ == '__main__':
    unittest.main()
