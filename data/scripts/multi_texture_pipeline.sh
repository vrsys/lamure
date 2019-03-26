#!/bin/bash

# Requires path of OBJ as argument, or will run with default (nordportal)


############################
# user settings
############################
# charting:
CHART_THRES=100
CELL_RES=20
NORMAL_VARIANCE_THRESHOLD=0.01

# hierarchy creation
TRI_BUDGET=1000

#maximum single output texture size
MAX_TEX_SIZE=1024
MAX_MULTI_TEX_SIZE=1024

#dilations
NUM_DILATIONS=1000


############################

echo "RUNNING MULTI TEXTURE PIPELINE"
echo "----------------------------------------------------"

#lamure directory
LAM_DIR="../.."
cd $LAM_DIR

LAM_DIR=$PWD


if [[ $# != 1 ]]; then
	#input directory for multi texture obj
	SRC_OBJ="/mnt/terabytes_of_textures/output_sensitive_rendering/nordportal/1k_test/Nordportal.obj"
else 
	SRC_OBJ=$1
fi

echo "using obj model $SRC_OBJ"


#create folder for regression test
C_DATE=`date`
C_DATE=${C_DATE// /_}
C_DATE=${C_DATE//:/_}
REGR_DIR="${LAM_DIR}/data/regression/${C_DATE}/"
echo "creating folder ${REGR_DIR}"
mkdir "${REGR_DIR}"


#copy input files to regression folder
SRC_DIR=$(dirname "${SRC_OBJ}")

#copy only necessary files. allow jpgs that need to be converted
echo "copying obj"
cp /${SRC_OBJ} ${REGR_DIR}
echo "copying material files" 
cp /${SRC_DIR}/*.mtl ${REGR_DIR}
echo "copying png files"
cp /${SRC_DIR}/*.png ${REGR_DIR}
echo "copying jpg files"
cp /${SRC_DIR}/*.jpg ${REGR_DIR}

#create path to obj file
OBJPATH="${REGR_DIR}/$(basename "${SRC_OBJ}")"

cd ${REGR_DIR}
#convert textures to png if necessary
echo "converting jpgs to pngs"
mogrify -format png *.jpg
#flip all texture images
echo "Flipping texture images"
mogrify -flip *.png
cd ${LAM_DIR}


#create charts
echo "----------------------------------------------------"
echo "Running chart creation with file $OBJPATH"
echo "----------------------------------------------------"

./install/bin/lamure_grid_face_clustering -debug -f $OBJPATH -ch $CHART_THRES -cc $CELL_RES -ct $NORMAL_VARIANCE_THRESHOLD

#create hierarchy
echo "----------------------------------------------------"
echo "Creating LOD hierarchy"
echo "----------------------------------------------------"

CHART_OBJPATH="${OBJPATH:0:${#OBJPATH}-4}_charts.obj"
CHARTFILE_PATH="${CHART_OBJPATH:0:${#CHART_OBJPATH}-4}.chart"

echo "using obj file $CHART_OBJPATH"
echo "using chart file $CHARTFILE_PATH"


./install/bin/lamure_mesh_hierarchy -f $CHART_OBJPATH -cf $CHARTFILE_PATH -t $TRI_BUDGET

BVH_PATH="${CHART_OBJPATH:0:${#CHART_OBJPATH}-4}.bvh"
LODCHART_PATH="${CHART_OBJPATH:0:${#CHART_OBJPATH}-4}.lodchart"

#create texture and LOD file with updated coordinates
echo "----------------------------------------------------"
echo "Create texture and updated LOD file"
echo "----------------------------------------------------"

echo "using bvh file $BVH_PATH"
echo "using lodchart file $LODCHART_PATH"
echo "using png file $PNGPATH"

./install/bin/lamure_mesh_preprocessing -f $BVH_PATH -single-max ${MAX_TEX_SIZE} -multi-max ${MAX_MULTI_TEX_SIZE}

FINAL_BVH_PATH="${BVH_PATH:0:${#BVH_PATH}-4}_uv.bvh"
VIS_PATH="${BVH_PATH:0:${#BVH_PATH}-4}_uv.vis"
FINAL_TEX_PATH="${BVH_PATH:0:${#BVH_PATH}-4}_uv0.png"


echo "----------------------------------------------------"
echo "Dilation"
echo "----------------------------------------------------"
#dilate new texture to avoid cracks 
# for dilation_iteration in `seq 1 "$NUM_DILATIONS"`;
# do
#   ./install/bin/lamure_texture_dilation $FINAL_TEX_PATH $FINAL_TEX_PATH
# done

./install/bin/lamure_texture_dilation $FINAL_TEX_PATH $FINAL_TEX_PATH $NUM_DILATIONS

echo "----------------------------------------------------"
echo "Visualising"
echo "----------------------------------------------------"

#create vis file and run vis app
touch $VIS_PATH
echo $FINAL_BVH_PATH > $VIS_PATH

echo "using vis file $VIS_PATH"
echo "using tex file $FINAL_TEX_PATH"

./install/bin/lamure_vis $VIS_PATH $FINAL_TEX_PATH







