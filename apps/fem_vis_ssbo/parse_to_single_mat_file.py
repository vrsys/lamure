#!/usr/bin/python

import scipy.io as sio
import numpy as np
import sys

from joblib import Parallel, delayed
import multiprocessing

import mat73

import pathlib

import os


number_of_arguments = len(sys.argv)


transform_vector = False

if number_of_arguments < 2:
  print("This program takes an *.mat-File with the FEM-Attributes as defined before and creates a binary stream of the relevant data as *.mat.bin file")
  print("Please provide an input *.mat-File to parse!")
  sys.exit(-1)



complete_path_string = sys.argv[1]

#for transforming the vectors properly, we need to provide the application with a vis file containing the transformation

transform_mat = np.empty((4,4), dtype=np.float32 )
print(transform_mat.shape)

if number_of_arguments > 2:

    transform_string_token = "fem_to_pcl_transform:"

    vis_file_string = sys.argv[2]
    print("Going to transform the vectors")

    
    vis_file = open(vis_file_string, 'r')
    vis_line_list = vis_file.readlines()
    vis_file.close()
    found = False
    
    for line in vis_line_list:
        if transform_string_token in line:
            print("Found it")
            transform_vector = True
            line = line.replace(transform_string_token, '')
            print(line)
            found = True
            tf_mat_float_vals = line.split()

            print(tf_mat_float_vals)

            element_idx_x = 0
            element_idx_y = 0

            for float_val_as_string in tf_mat_float_vals:
                transform_mat[element_idx_y][element_idx_x] = float(float_val_as_string)

                print("xxxx" + float_val_as_string)
                print(float(float_val_as_string) )

                element_idx_x += 1
                if(element_idx_x % 4 == 0):
                    element_idx_x = 0
                    element_idx_y += 1
            #transform_mat
            #sys.exit(-1)


print("Printing  t")
print(transform_mat)

    

#sys.exit(-1)


complete_path = pathlib.PurePath(complete_path_string)


directory_name = str(complete_path.name)

print("Directory name: " + directory_name )
print(sys.argv[1])


complete_out_path_base_name = complete_path_string

mat_file_list = []

for mat_file in os.listdir( sys.argv[1] ):

    if mat_file.startswith(directory_name) and mat_file.endswith(".mat") and not mat_file.endswith("trainPos.mat"):
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
current_mag_u = 0

mag_u_min =  999999999
mag_u_max = -999999999

global_mag_u_min = mag_u_min
global_mag_u_max = mag_u_max

for mat_file_string in mat_file_list:
    in_current_mat_file_name = mat_file_string
    print("X: " + in_current_mat_file_name)
    curr_mat_contents = None
    try:
        curr_mat_contents = sio.loadmat(in_current_mat_file_name)
    except:
        curr_mat_contents = mat73.loadmat(in_current_mat_file_name)

    curr_sim_array = curr_mat_contents['dataSave']


    curr_num_attributes_in_sim_array = curr_sim_array.shape[1]

    if 0 == num_attributes_files_to_open:
        num_attributes_files_to_open = curr_num_attributes_in_sim_array
        print(num_attributes_files_to_open)

        for attrib_id in range(num_attributes_files_to_open):
            open_attribute_file_paths.append(complete_path_string + "/attribute_" + str(attrib_id) + ".mat.bin")
            open_attribute_file_handles.append( open(open_attribute_file_paths[attrib_id], 'wb') ) 

    else:
        if num_attributes_files_to_open != curr_num_attributes_in_sim_array:
            print("Different number of attributes per timestep. Exiting.")
            sys.exit(-1)

    additional_mag_u_offset = 0     



    for attrib_id in range(3):
        print("Iterating over attrib id file handle: " + str(attrib_id))
        #if 3 == attrib_id:
        #   additional_mag_u_offset = 1

        #skip the first column because it only contains vertex ids
        curr_attrib_for_all_vertices = curr_sim_array[:,(1 + attrib_id)]
        curr_attrib_for_all_vertices = curr_attrib_for_all_vertices.astype(np.float32)

        #for now we assume that attribute 0 will be mag x
        if 0 == attrib_id:
            current_mag_x = curr_attrib_for_all_vertices
        elif 1 == attrib_id:
            current_mag_y = curr_attrib_for_all_vertices
        elif 2 == attrib_id:
            current_mag_z = curr_attrib_for_all_vertices



            if transform_vector:


                
                for vertex_idx in range(current_mag_x.shape[0]):
                    vec_to_transform = np.empty( (4,1), dtype=np.float32)
                    vec_to_transform[0] = current_mag_x[vertex_idx]
                    vec_to_transform[1] = current_mag_y[vertex_idx]
                    vec_to_transform[2] = current_mag_z[vertex_idx]
                    vec_to_transform[3] = 0.0


                    transformed_vec = np.matmul(transform_mat, vec_to_transform)

                    current_mag_x[vertex_idx] = transformed_vec[0]
                    current_mag_y[vertex_idx] = transformed_vec[1]
                    current_mag_z[vertex_idx] = transformed_vec[2]


                
                    """
                    if current_mag_x[vertex_idx] != 0.0:
                        print("before:" + str(vec_to_transform)) 
                        current_mag_u = np.sqrt(  vec_to_transform[0]*vec_to_transform[0] 
                                                + vec_to_transform[1]*vec_to_transform[1]  
                                                + vec_to_transform[2]*vec_to_transform[2] )
                        print("Mag U before: " + str(current_mag_u) )

                        print("after:" + str(transformed_vec)) 

                        current_mag_u = np.sqrt(  transformed_vec[0]*transformed_vec[0] 
                                                + transformed_vec[1]*transformed_vec[1]  
                                                + transformed_vec[2]*transformed_vec[2] )
                        print("Mag U after: " + str(current_mag_u) )
                    """


            current_mag_u = np.sqrt(current_mag_x*current_mag_x + current_mag_y * current_mag_y + current_mag_z * current_mag_z)

            #for element in range(current_mag_u.shape[0]):
            #   print(current_mag_u[element] )

            #current_mag_u.astype(dtype=np.float32).tofile(open_attribute_file_handles[3])
            #print("XXXX " + str(current_mag_u[40000]) )

            #for element in range(current_mag_u.shape[0]):
            #   x = np.float32(1000.0)
            #   open_attribute_file_handles[3].write( x.tobytes() )

            current_mag_u = current_mag_u.astype(dtype=np.float32)
            #current_mag_u.tofile(open_attribute_file_handles[3])
            open_attribute_file_handles[3].write(current_mag_u.astype(dtype=np.float32).tobytes('C'))
            #open_attribute_file_handles[3].write(current_mag_u.astype(dtype=np.float32).tobytes('C'))

            mag_u_max = np.amax(current_mag_u)
            mag_u_min = np.amin(current_mag_u)

            global_mag_u_min = min(global_mag_u_min, mag_u_min)
            global_mag_u_max = max(global_mag_u_max, mag_u_max)

            print("Mag u min & max: " + str(mag_u_min) + "    " + str(mag_u_max))

        #if(attrib_id > 2):
        #open_attribute_file_handles[additional_mag_u_offset + attrib_id].write(curr_attrib_for_all_vertices.astype(dtype=np.float32).tobytes('C'))



    for attrib_id in range(num_attributes_files_to_open-1):
        if 0 == attrib_id:
            curr_attrib_for_all_vertices = current_mag_x
        elif 1 == attrib_id:
            curr_attrib_for_all_vertices = current_mag_y
        elif 2 == attrib_id:
            curr_attrib_for_all_vertices = current_mag_z
        else:
            curr_attrib_for_all_vertices = curr_sim_array[:,(1 + attrib_id)]
            curr_attrib_for_all_vertices = curr_attrib_for_all_vertices.astype(np.float32)

        if 3 == attrib_id:
            additional_mag_u_offset = 1

        open_attribute_file_handles[additional_mag_u_offset + attrib_id].write(curr_attrib_for_all_vertices.astype(dtype=np.float32).tobytes('C'))

print("Global Mag u min & max: " + str(global_mag_u_min) + "    " + str(global_mag_u_max))
#path = os.path.dirname(os.path.realpath(sys.argv[1]))






#print(mat_contents)





#print(sim_array.shape[1])

#in_mat_file_name = sys.argv[1]
#mat_contents = sio.loadmat(in_mat_file_name)

#sim_array = mat_contents['dataSave']


for attrib_id in range(num_attributes_files_to_open):
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