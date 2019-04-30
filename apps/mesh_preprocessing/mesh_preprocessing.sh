#!/bin/bash

# Requires path of OBJ as argument

#lamure directory
#SCRIPT_CALL=$0
#LAMURE_DIR=$(basename -- "$SCRIPT_CALL")

#alternatively, use user specified directory:
LAMURE_DIR=~/svn/lamure/install/bin/

############################
# user settings
############################
# charting:
KDTREE_TRI_BUDGET=24000
GRID_RES=30 #initial resolution
COST_THRESHOLD=0.05 # max cost

# BVH hierarchy creation
TRI_BUDGET=16000

#maximum output texture size
MAX_FINAL_TEX_SIZE=65536

#dilations
NUM_DILATIONS=4096

############################

echo "RUNNING MESHLOD PIPELINE"
echo "------------------------"

SRC_OBJ=$1

echo "Using obj model $SRC_OBJ"

#create path to obj file
OBJPATH="$SRC_OBJ"

#convert textures to png if necessary
#echo "Converting jpgs to pngs"
#mogrify -format png *.jpg
#flip all texture images
#echo "Flipping texture images"
#mogrify -flip *.png

echo "Running chart creation with file $SRC_OBJ"
echo "-----------------------------------------"

time ${LAMURE_DIR}lamure_mesh_preprocessing -f $OBJPATH -tkd $KDTREE_TRI_BUDGET -co $COST_THRESHOLD -cc GRID_RES -tbvh $TRI_BUDGET -multi-max $MAX_FINAL_TEX_SIZE






