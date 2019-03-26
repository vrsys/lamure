#!/bin/bash

############################
# user settings
############################
# charting:
CHART_THRES=50 # number of charts created
CELL_RES=20 # starting grid - how many cells across
NORMAL_VARIANCE_THRESHOLD=0.01 # how much charts are split after grid is used

# hierarchy creation
TRI_BUDGET=2000

#maximum single output texture size
MAX_TEX_SIZE=1024
MAX_MULTI_TEX_SIZE=1024

#dilations
NUM_DILATIONS=1000

############################

#lamure directory
LAM_DIR="../.."
cd $LAM_DIR

LAM_DIR=$PWD

mkdir data/regression

echo "detected $# arguments"


if [[ $# != 2 ]]; then
	#input files
	SRC_OBJPATH="/home/hoqe4365/Desktop/lamure/lamure/install/bin/data/zebra/zebra_small.obj"
	SRC_PNGPATH="/home/hoqe4365/Desktop/lamure/lamure/install/bin/data/zebra/zebra-atlas.png"
else 

	SRC_OBJPATH=$1
	SRC_PNGPATH=$2
fi

SRC_MTLPATH="${SRC_OBJPATH:0:${#SRC_OBJPATH}-4}.mtl"

echo "using files $SRC_OBJPATH and $SRC_PNGPATH and ${SRC_MTLPATH}"


#create folder for regression test
C_DATE=`date`
C_DATE=${C_DATE// /_}
C_DATE=${C_DATE//:/_}
REGR_DIR="$LAM_DIR/data/regression/$C_DATE"
echo "creating folder $REGR_DIR"
mkdir "$REGR_DIR"


#copy input files to regression folder
# cp $SRC_OBJPATH $REGR_DIR
cp $SRC_PNGPATH ${REGR_DIR}
cp $SRC_MTLPATH ${REGR_DIR}

#get copies of files
OBJPATH="$REGR_DIR/$(basename $SRC_OBJPATH)"
CHART_OBJPATH="${OBJPATH:0:${#OBJPATH}-4}_charts.obj"
PNGPATH="$REGR_DIR/$(basename $SRC_PNGPATH)"

echo "Flipping texture image"
mogrify -flip ${PNGPATH}

#create charts
echo "----------------------------------------------------"
echo "Running chart creation with file $SRC_OBJPATH"
echo "----------------------------------------------------"

./install/bin/lamure_grid_face_clustering -f ${SRC_OBJPATH} -of $CHART_OBJPATH -ch ${CHART_THRES} -cc ${CELL_RES} -ct ${NORMAL_VARIANCE_THRESHOLD}

# create LOD hierarchy and simplify nodes
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

./install/bin/lamure_mesh_preprocessing -f $BVH_PATH -c $LODCHART_PATH -t $PNGPATH -single-max ${MAX_TEX_SIZE} -multi-max ${MAX_MULTI_TEX_SIZE}

FINAL_BVH_PATH="${BVH_PATH:0:${#BVH_PATH}-4}_uv.bvh"
VIS_PATH="${BVH_PATH:0:${#BVH_PATH}-4}_uv.vis"
FINAL_TEX_PATH="${BVH_PATH:0:${#BVH_PATH}-4}_uv0.png"
DILATED_TEX_PATH="${FINAL_TEX_PATH:0:${#FINAL_TEX_PATH}-4}_dil.png"

echo "----------------------------------------------------"
echo "Dilation"
echo "----------------------------------------------------"

#dilate new texture to avoid cracks 
./install/bin/lamure_texture_dilation $FINAL_TEX_PATH $DILATED_TEX_PATH $NUM_DILATIONS


echo "----------------------------------------------------"
echo "Visualising"
echo "----------------------------------------------------"

#create vis file and run vis app
touch $VIS_PATH
echo $FINAL_BVH_PATH > $VIS_PATH

echo "using vis file $VIS_PATH"
echo "using tex file $DILATED_TEX_PATH"

./install/bin/lamure_vis $VIS_PATH $DILATED_TEX_PATH
