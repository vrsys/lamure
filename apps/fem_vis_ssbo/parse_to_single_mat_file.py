#!/usr/bin/python

import scipy.io as sio
import numpy as np
import sys

import pathlib

import os


number_of_arguments = len(sys.argv)

if number_of_arguments < 2:
  print("This program takes an *.mat-File with the FEM-Attributes as defined before and creates a binary stream of the relevant data as *.mat.bin file")
  print("Please provide an input *.mat-File to parse!")
  sys.exit(-1)



complete_path_string = sys.argv[1]

complete_path = pathlib.PurePath(complete_path_string)


directory_name = str(complete_path.name)

print("Directory name: " + directory_name )
print(sys.argv[1])


complete_out_path_base_name = complete_path_string

mat_file_list = []

for mat_file in os.listdir( sys.argv[1] ):

    if mat_file.startswith(directory_name) and mat_file.endswith(".mat"):
    	mat_file_list.append(complete_path_string + "/" + mat_file)
        #print(os.path.join("/mydir", mat_file))

mat_file_list.sort() 


num_attributes_files_to_open = 0


open_attribute_file_paths = []
open_attribute_file_handles = []

all_attributes_file_handle = 0


current_mag_x = 0
current_mag_y = 0
current_mag_z = 0

for mat_file_string in mat_file_list:
	in_current_mat_file_name = mat_file_string
	print("X: " + in_current_mat_file_name)
	curr_mat_contents = sio.loadmat(in_current_mat_file_name)

	curr_sim_array = curr_mat_contents['dataSave']


	curr_num_attributes_in_sim_array = curr_sim_array.shape[1]

	if 0 == num_attributes_files_to_open:
		num_attributes_files_to_open = curr_num_attributes_in_sim_array
		print(num_attributes_files_to_open)

		for attrib_id in range(num_attributes_files_to_open - 3):
			open_attribute_file_paths.append(complete_path_string + "/attribute_" + str(attrib_id) + ".mat.bin")
			open_attribute_file_handles.append( open(open_attribute_file_paths[attrib_id], 'wb') ) 

	else:
		if num_attributes_files_to_open != curr_num_attributes_in_sim_array:
			print("Different number of attributes per timestep. Exiting.")
			sys.exit(-1)

	additional_mag_u_offset = 0		



	for attrib_id in range(num_attributes_files_to_open-1):

		if 3 == attrib_id:
			additional_mag_u_offset = 1

		curr_attrib_for_all_vertices = curr_sim_array[:,(1 + attrib_id)]
		curr_attrib_for_all_vertices = curr_attrib_for_all_vertices.astype(np.float32)

		#for now we assume that attribute 0 will be mag x
		if 0 == attrib_id:
			current_mag_x = curr_attrib_for_all_vertices
		elif 1 == attrib_id:
			current_mag_y = curr_attrib_for_all_vertices
		elif 2 == attrib_id:
			current_mag_z = curr_attrib_for_all_vertices
			current_mag_u = np.sqrt(current_mag_x*current_mag_x + current_mag_y * current_mag_y + current_mag_z * current_mag_z)
			open_attribute_file_handles[0].write(current_mag_u.tobytes())

		if(attrib_id > 2):
			open_attribute_file_handles[additional_mag_u_offset + attrib_id - 3].write(curr_attrib_for_all_vertices.tobytes())

#path = os.path.dirname(os.path.realpath(sys.argv[1]))






#print(mat_contents)





#print(sim_array.shape[1])

#in_mat_file_name = sys.argv[1]
#mat_contents = sio.loadmat(in_mat_file_name)

#sim_array = mat_contents['dataSave']


for attrib_id in range(num_attributes_files_to_open - 3):
	open_attribute_file_handles[attrib_id].close()


append_count = 0

all_attribs_file_path = complete_path_string + "/all_attributes.mat.bin"

for attrib_file_path in open_attribute_file_paths:
	print(str(attrib_file_path) + "     " + str(all_attribs_file_path))
	
	if append_count > 0:
		os.system("cat " + attrib_file_path + " >> " + all_attribs_file_path)
	else:
		os.system("cat " + attrib_file_path + " > " + all_attribs_file_path)
	
	append_count += 1


#all_attributes_file_handle = open(complete_path_string + "/all_attributes.mat.bin", 'wb')

#all_attributes_file_handle.close()

sys.exit(-1)