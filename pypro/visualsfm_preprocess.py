from pypro import *
import sys

if len(sys.argv) != 4:
	print('arguments: <nvm-file> <patch-file> <ply-file>')
else:
	format_sparse_nvm = FormatSparseNVMV3(sys.argv[1])
	format_sparse_nvm.out()

	format_dense_pmvs = FormatDensePMVS(sys.argv[2], sys.argv[3])
	format_dense_pmvs.out()