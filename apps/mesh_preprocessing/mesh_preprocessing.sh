#!/bin/bash

# Requires path of OBJ as argument

if [[ -z "${LAMURE_DIR}" ]]; then
  echo -e "\e[31m"
  echo -e "To run this mesh preprocessing script, define \e[1;4mLAMURE_DIR\e[24m\e[21m environmental variable, e.g.:"
  echo -e "\e[1;4mexport LAMURE_DIR=/opt/lamure/install/bin/\e[24m\e[21m"
  echo -e "You might want to add this line to your \e[1;4m.bashrc\e[24m\e[21m for persistent definition. Do not forget to follow it by:"
  echo -e "\e[1;4msource .bashrc\e[24m\e[21m"
  echo -e "\e[0m"
  exit
fi

############################
# user settings
############################
# charting:
KDTREE_TRI_BUDGET=24000
COST_THRESHOLD=0.05 # max cost

# BVH hierarchy creation
TRI_BUDGET=16000

#maximum output texture size
MAX_FINAL_TEX_SIZE=8192

#dilations
NUM_DILATIONS=4096

############################

echo "RUNNING MESHLOD PIPELINE"
echo "------------------------"

SRC_OBJ=$1

echo "Using obj model $SRC_OBJ"

#create path to obj file
OBJPATH="$SRC_OBJ"

echo -e "\e[32m"

read -p "Convert all textures to PNG (required)? (y/n)? " answer
case ${answer:0:1} in
    y|Y )
        # convert textures to PNG using mogrify
        targets=*.{jpg,JPG,png,PNG,bmp,BMP,tiff,TIFF,tif,TIF,ppm,PPM,pgm,PGM,pbm,PBM,pnm,PNM}
        echo "Converting to PNG: "
        echo $targets
        mogrify -format png $targets
    ;;
    * )
        echo "Skipping conversion (assuming was done before)"
    ;;
esac

read -p "Flip all textures (required)? (y/n)? " answer
case ${answer:0:1} in
    y|Y )
        targets=*.png
        echo "Flipping PNGs"
        mogrify -flip $targets
    ;;
    * )
        echo "Skipping flipping (assuming was done before)"
    ;;
esac

echo -e "\e[0m"

echo "Running chart creation with file $SRC_OBJ"
echo "-----------------------------------------"

DATE=`date '+%Y-%m-%d:%H:%M:%S'`
time ${LAMURE_DIR}lamure_mesh_preprocessing -f $OBJPATH -tkd $KDTREE_TRI_BUDGET -co $COST_THRESHOLD -tbvh $TRI_BUDGET -multi-max $MAX_FINAL_TEX_SIZE 2>&1 | tee log_${DATE}.txt






