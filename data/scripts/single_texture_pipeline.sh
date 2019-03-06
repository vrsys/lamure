#!/bin/bash

#lamure directory
LAM_DIR="../.."
cd $LAM_DIR

LAM_DIR=$PWD

mkdir cddata/regression

echo "detected $# arguments"


if [[ $# != 2 ]]; then
	#input files
	SRC_OBJPATH="/home/hoqe4365/Desktop/lamure/lamure/install/bin/data/zebra/zebra_small.obj"
	SRC_PNGPATH="/home/hoqe4365/Desktop/lamure/lamure/install/bin/data/zebra/zebra-atlas-flip.png"
else 

	SRC_OBJPATH=$1
	SRC_PNGPATH=$2
fi

echo "using files $SRC_OBJPATH and $SRC_PNGPATH"





# Absolute path to this script
SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

#create folder for regression test
C_DATE=`date`
C_DATE=${C_DATE// /_}
C_DATE=${C_DATE//:/_}
REGR_DIR="$LAM_DIR/data/regression/$C_DATE"
echo "creating folder $REGR_DIR"
mkdir "$REGR_DIR"


#copy input files to regression folder
cp $SRC_OBJPATH $REGR_DIR
cp $SRC_PNGPATH $REGR_DIR

#get copies of files
OBJPATH="$REGR_DIR/$(basename $SRC_OBJPATH)"
PNGPATH="$REGR_DIR/$(basename $SRC_PNGPATH)"




#create charts
echo "----------------------------------------------------"
echo "Running chart creation with file $OBJPATH"
echo "----------------------------------------------------"

CHART_THRES=200
CELL_RES=10
NORMAL_VARIANCE_THRESHOLD=0.002

# `./install/bin/lamure_grid_face_clustering -f ${OBJPATH} -ch ${CHART_THRES} -cc ${CELL_RES} -ct ${NORMAL_VARIANCE_THRESHOLD}`
./install/bin/lamure_grid_face_clustering -f ${OBJPATH} -ch ${CHART_THRES} -cc ${CELL_RES} -ct ${NORMAL_VARIANCE_THRESHOLD}




#create hierarchy
# create LOD hierarchy and simplify nodes
echo "----------------------------------------------------"
echo "Creating LOD hierarchy"
echo "----------------------------------------------------"

CHART_OBJPATH="${OBJPATH:0:${#OBJPATH}-4}_charts.obj"
CHARTFILE_PATH="${CHART_OBJPATH:0:${#CHART_OBJPATH}-4}.chart"

echo "using obj file $CHART_OBJPATH"
echo "using chart file $CHARTFILE_PATH"

TRI_BUDGET="1000"
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

./install/bin/lamure_mesh_preprocessing -f $BVH_PATH -c $LODCHART_PATH -t $PNGPATH

FINAL_BVH_PATH="${BVH_PATH:0:${#BVH_PATH}-4}_uv.bvh"
VIS_PATH="${BVH_PATH:0:${#BVH_PATH}-4}_uv.vis"
FINAL_TEX_PATH="${BVH_PATH:0:${#BVH_PATH}-4}_uv.png"


#dilate new texture to avoid cracks 
NUM_DILATIONS=4
for dilation_iteration in `seq 1 "$NUM_DILATIONS"`;
do
  ./install/bin/lamure_texture_dilation $FINAL_TEX_PATH $FINAL_TEX_PATH
done


#create vis file and run vis app
touch $VIS_PATH
echo $FINAL_BVH_PATH > $VIS_PATH

echo "using vis file $VIS_PATH"
echo "using tex file $FINAL_TEX_PATH"

./install/bin/lamure_vis $VIS_PATH $FINAL_TEX_PATH
