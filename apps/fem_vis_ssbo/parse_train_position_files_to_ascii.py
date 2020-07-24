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
  print("This program takes *trainPos.mat-File with the trainPositions according to the FEM-trainPos-headerFile and writes them line by line into an ascii file ending in *.train_pos")
  print("Please provide directory containing  *trainPos.mat-File to parse!")
  sys.exit(-1)



complete_path_string = sys.argv[1]

#for transforming the train positions properly (because they,too, are defined in FEM space)

transform_mat = np.empty((4,4), dtype=np.float32 )
#print(transform_mat.shape)

transformation_available = False

if number_of_arguments > 2:

    transform_string_token = "fem_to_pcl_transform:"

    vis_file_string = sys.argv[2]
    #print("Going to transform the vectors")

    
    vis_file = open(vis_file_string, 'r')
    vis_line_list = vis_file.readlines()
    vis_file.close()
    found = False
    
    for line in vis_line_list:
        if transform_string_token in line:
            #print("Found it")
            transform_vector = True
            line = line.replace(transform_string_token, '')
            #print(line)
            found = True
            tf_mat_float_vals = line.split()

            #print(tf_mat_float_vals)

            element_idx_x = 0
            element_idx_y = 0

            for float_val_as_string in tf_mat_float_vals:
                transform_mat[element_idx_y][element_idx_x] = float(float_val_as_string)

                #print("xxxx" + float_val_as_string)
                #print(float(float_val_as_string) )

                element_idx_x += 1
                if(element_idx_x % 4 == 0):
                    element_idx_x = 0
                    element_idx_y += 1
            #transform_mat
            #sys.exit(-1)

    transformation_available = True
#print("Printing  t")
#print(transform_mat)

    

#sys.exit(-1)


complete_path = pathlib.PurePath(complete_path_string)


directory_name = str(complete_path.name)

#print("Directory name: " + directory_name )
#print(sys.argv[1])


complete_out_path_base_name = complete_path_string

mat_file_list = []

for mat_file in os.listdir( sys.argv[1] ):

    if mat_file.startswith(directory_name) and mat_file.endswith("trainPos.mat"):
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



all_train_positions_file_path = complete_path_string + "/all_train_positions.train_pos"

all_train_positions_file = open(all_train_positions_file_path, 'w')

is_first_train_pos_file = True

for mat_file_string in mat_file_list:



    in_current_mat_file_name = mat_file_string
    #print("X: " + in_current_mat_file_name)

    curr_mat_contents = None
    try:
        curr_mat_contents = sio.loadmat(in_current_mat_file_name)
    except:
        curr_mat_contents = mat73.loadmat(in_current_mat_file_name)

    #print(curr_mat_contents)

    curr_sim_array = curr_mat_contents['dataSave']


    curr_num_attributes_in_sim_array = curr_sim_array.shape[1]

    #print("Current num attributes: " + str( curr_sim_array[0]) )
    #sys.exit(-1)

    if is_first_train_pos_file:
        all_train_positions_file.write(str(curr_sim_array.shape[0]) )
        all_train_positions_file.write(" Achsen je 3 Attributen [x in Metern] [y in Metern] [z in Metern] pro Zeile\n" )
        is_first_train_pos_file = False

    axis_string = ""



    for train_position in range(curr_sim_array.shape[0]):


        vec_head_to_transform = np.empty( (4,1), dtype=np.float32)
        vec_head_to_transform[0] = curr_sim_array[train_position][1]
        vec_head_to_transform[1] = curr_sim_array[train_position][2]
        vec_head_to_transform[2] = curr_sim_array[train_position][3]
        vec_head_to_transform[3] = 1.0

        #print("pre-transform: " + str(vec_head_to_transform) )

        if transformation_available:
            transformed_head_vec = np.matmul(transform_mat, vec_head_to_transform)
        else:
            transformed_head_vec = vec_head_to_transform

        #print("post-transform: " + str(transformed_head_vec) )


        curr_sim_array[train_position][1] = transformed_head_vec[0]
        curr_sim_array[train_position][2] = transformed_head_vec[1]
        curr_sim_array[train_position][3] = transformed_head_vec[2]

        #print(train_position + "tf")

        """
        vec_tail_to_transform = np.empty( (4,1), dtype=np.float32)
        vec_tail_to_transform[0] = curr_sim_array[train_position][1]
        vec_tail_to_transform[1] = curr_sim_array[train_position][2] - 1.0
        vec_tail_to_transform[2] = curr_sim_array[train_position][3]
        vec_tail_to_transform[3] = 1.0

        transformed_tail_vec = np.matmul(transform_mat, vec_head_to_transform)

        curr_sim_array[train_position][1] = vec_head_to_transform[0]
        curr_sim_array[train_position][2] = vec_head_to_transform[1]
        curr_sim_array[train_position][3] = vec_head_to_transform[2]
        """
        
        for element_idx in range(1, 4):
            axis_string += str(curr_sim_array[train_position][element_idx])

            if element_idx < 3:
                axis_string += " "
            else:
                if train_position != curr_sim_array.shape[0] - 1:
                    axis_string += " "
                else:
                    axis_string += "\n"

        #all_train_positions_file.write(" Achsen je 4 Attributen [Kraft in Newton] [x in Metern] [y in Metern] [z in Metern] pro Zeile\n" )

    all_train_positions_file.write(axis_string)


all_train_positions_file.close()

print ("Done transforming and writing train positions: "  + str(all_train_positions_file) )

sys.exit(-1)