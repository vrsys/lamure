#!/bin/bash

echo "RUNNING MULTI TEXTURE PIPELINE"
echo "----------------------------------------------------"

#lamure directory
LAM_DIR="../.."
cd $LAM_DIR

LAM_DIR=$PWD


if [[ $# != 1 ]]; then
	#input directory for multi texture obj
	SRC_DIR="/mnt/terabytes_of_textures/output_sensitive_rendering/nordportal/1k_test/"
else 
	SRC_DIR=$1
fi

echo "using directory $SRC_DIR"



#create folder for regression test
C_DATE=`date`
C_DATE=${C_DATE// /_}
C_DATE=${C_DATE//:/_}
REGR_DIR="${LAM_DIR}/data/regression/${C_DATE}/"
echo "creating folder ${REGR_DIR}"
mkdir "${REGR_DIR}"


#copy input files to regression folder
cp -a /${SRC_DIR}/. ${REGR_DIR}

#get path to obj file
for OBJS in ${REGR_DIR}/*.obj; do
	OBJPATH=$OBJS
	echo $OBJPATH
done
JPGPATH="${OBJPATH:0:${#OBJPATH}-4}_comp.jpg"

# echo "obj path: ${OBJPATH}"
# echo "jpg path: ${JPGPATH}"

#create compiled texture
echo "----------------------------------------------------"
echo "Compiling texture in folder $REGR_DIR"
echo "----------------------------------------------------"

SCALE=1
./install/bin/lamure_vt_atlas_compiler -d ${REGR_DIR} -s ${SCALE} -o ${JPGPATH}

#convert texture to png
mogrify -format png ${JPGPATH}

PNGPATH="${JPGPATH:0:${#JPGPATH}-3}png"



#adjust texture coordinates in obj
echo "----------------------------------------------------"
echo "Updating OBJ texture coordinates"
echo "----------------------------------------------------"


LOGPATH="${PNGPATH:0:${#PNGPATH}-4}.log"
echo "log path: ${LOGPATH}"

./install/bin/lamure_obj_texture_atlas_modifier -obj ${OBJPATH} -log ${LOGPATH} -tex ${PNGPATH}

#update OBJ path
OBJPATH="${OBJPATH:0:${#OBJPATH}-4}_w_atlas.obj"

#create charts
echo "----------------------------------------------------"
echo "Running chart creation with file $OBJPATH"
echo "----------------------------------------------------"

CHART_THRES=200
CELL_RES=10
NORMAL_VARIANCE_THRESHOLD=0.002

./install/bin/lamure_grid_face_clustering -f $OBJPATH -ch $CHART_THRES -cc $CELL_RES -ct $NORMAL_VARIANCE_THRESHOLD




#create hierarchy
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


#TODO - dilate new texture here 



#create vis file and run vis app
touch $VIS_PATH
echo $FINAL_BVH_PATH > $VIS_PATH

echo "using vis file $VIS_PATH"
echo "using tex file $FINAL_TEX_PATH"

./install/bin/lamure_vis $VIS_PATH $FINAL_TEX_PATH

#create hierarchy
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


#TODO - dilate new texture here 



#create vis file and run vis app
touch $VIS_PATH
echo $FINAL_BVH_PATH > $VIS_PATH

echo "using vis file $VIS_PATH"
echo "using tex file $FINAL_TEX_PATH"

./install/bin/lamure_vis $VIS_PATH $FINAL_TEX_PATH
