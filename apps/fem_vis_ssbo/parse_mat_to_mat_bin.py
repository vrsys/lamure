#!/usr/bin/python

import scipy.io as sio
import numpy as np
import sys


number_of_arguments = len(sys.argv)

if number_of_arguments < 2:
  print("This program takes an *.mat-File with the FEM-Attributes as defined before and creates a binary stream of the relevant data as *.mat.bin file")
  print("Please provide an input *.mat-File to parse!")
  sys.exit(-1)


in_mat_file_name = sys.argv[1]
mat_contents = sio.loadmat(in_mat_file_name)

sim_array = mat_contents['data']

#print(mat_contents)


#all_indices = sim_array[:,0]

#read all the relevant columns for the simulation data
all_u_x     = sim_array[:,1]
all_u_y     = sim_array[:,2]
all_u_z     = sim_array[:,3]
#all_mag_u will be computed based on all_u_x, all_u_y and all_u_z
all_sig_xx  = sim_array[:,4]
all_tau_xy  = sim_array[:,5]
all_tau_xz  = sim_array[:,6]
all_tau_mag = sim_array[:,7]
all_sig_v   = sim_array[:,8]
all_eps_x   = sim_array[:,9]


#all_indices_as_f32 = all_indices.astype(np.float32)


num_elements_per_attribute = all_u_x.size;

all_mag_u = np.sqrt(all_u_x * all_u_x + all_u_y * all_u_y + all_u_z * all_u_z)

all_mag_u   = all_mag_u.astype(np.float32)
all_u_x     = all_u_x.astype(np.float32)
all_u_y     = all_u_y.astype(np.float32)
all_u_z     = all_u_z.astype(np.float32)

all_sig_xx  = all_sig_xx.astype(np.float32)
all_tau_xy  = all_tau_xy.astype(np.float32)
all_tau_xz  = all_tau_xz.astype(np.float32)
all_tau_mag = all_tau_mag.astype(np.float32)
all_sig_v   = all_sig_v.astype(np.float32)
all_eps_x   = all_eps_x.astype(np.float32)



print(all_mag_u)

#print(sim_array)

#write out byte vectors in the order that was provided by fak B last

out_mat_bin_filename = in_mat_file_name + '.bin'
out_mat_bin_file = open(out_mat_bin_filename, 'wb')

out_mat_bin_file.write(all_u_x.tobytes())
out_mat_bin_file.write(all_u_y.tobytes())
out_mat_bin_file.write(all_u_z.tobytes())
out_mat_bin_file.write(all_mag_u.tobytes())
out_mat_bin_file.write(all_sig_xx.tobytes())
out_mat_bin_file.write(all_tau_xy.tobytes())
out_mat_bin_file.write(all_tau_xz.tobytes())
out_mat_bin_file.write(all_tau_mag.tobytes())
out_mat_bin_file.write(all_sig_v.tobytes())
out_mat_bin_file.write(all_eps_x.tobytes())

out_mat_bin_file.close()

#print(all_sig_xx_as_f32.tobytes())
#np.save('temperatur_test.mat.bin', all_sig_xx_as_f32, allow_pickle=False)
#print(all_sig_xx_as_f32)
