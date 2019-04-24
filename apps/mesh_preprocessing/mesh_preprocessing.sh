#!/bin/bash

# Requires path of OBJ as argument


############################
# user settings
############################
# charting:
KDTREE_TRI_BUDGET=24000
GRID_RES=30 #initial resolution
COST_THRESHOLD=0.05 # max cost

# BVH hierarchy creation
TRI_BUDGET=8192

#maximum output texture size
MAX_FINAL_TEX_SIZE=8192

#dilations
NUM_DILATIONS=4096

############################

echo "RUNNING MESHLOD PIPELINE"
echo "------------------------"

#lamure directory
LAMURE_DIR=/home/nite5129/svn/lamure_gary444_2/lamure/install/bin/

SRC_OBJ=$1

echo "using obj model $SRC_OBJ"

#create path to obj file
OBJPATH="$SRC_OBJ"
CHART_OBJPATH="${OBJPATH:0:${#OBJPATH}-4}_charts.obj"

#convert textures to png if necessary
#echo "converting jpgs to pngs"
#mogrify -format png *.jpg
#flip all texture images
#echo "Flipping texture images"
#mogrify -flip *.png

echo "Running chart creation with file $SRC_OBJ"
echo "-----------------------------------------"

time ${LAMURE_DIR}lamure_mesh_preprocessing -f $OBJPATH -of $CHART_OBJPATH -tkd $KDTREE_TRI_BUDGET -co $COST_THRESHOLD -cc GRID_RES -tbvh $TRI_BUDGET -multi-max $MAX_FINAL_TEX_SIZE






