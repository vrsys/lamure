#!/bin/bash

# Logging routines
DATE=`date '+%Y-%m-%d:%H:%M:%S'`
PIPEFILE="${HOME}"/pipe"${DATE}"
mkfifo ${PIPEFILE}
tee log_${DATE}.txt < ${PIPEFILE} &
TEEPID=$!
exec > ${PIPEFILE} 2>&1

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

############################

echo -e "\e[32m"

read -p "Use default settings? (y/n)? " answer
case ${answer:0:1} in
    y|Y )
        echo "Using following default settings"
        echo "Triangle budget per KD-tree node: " ${KDTREE_TRI_BUDGET}
        echo "Chart creation cost threshold: " ${COST_THRESHOLD}
        echo "Triangle budget per BVH node: " ${TRI_BUDGET}
        echo "Maximum size of final texture: " ${MAX_FINAL_TEX_SIZE}
    ;;
    * )
        read -rp "Triangle budget per KD-tree node: " KDTREE_TRI_BUDGET
        read -rp "Chart creation cost threshold: " COST_THRESHOLD
        read -rp "Triangle budget per BVH node: " TRI_BUDGET
    ;;
esac

echo -e "\e[0m"

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
        echo "Converting to PNG... "
        mogrify -format png *.jpg
    ;;
    * )
        echo "Skipping conversion (assuming was done before)"
    ;;
esac

read -p "Flip all textures (required)? (y/n)? " answer
case ${answer:0:1} in
    y|Y )
        echo "Flipping PNGs..."
        mogrify -flip *.jpg
    ;;
    * )
        echo "Skipping flipping (assuming was done before)"
    ;;
esac

read -rp "Please input the maximum allowed size for the final texture: " MAX_FINAL_TEX_SIZE

echo -e "\e[0m"

read -p "Would you like to produce a .raw file for VT preprocessing at the end (if not, you get a .png)? (y/n)? " answer
case ${answer:0:1} in
    y|Y )
        echo "Going to create RAW texture file at the end."
        time ${LAMURE_DIR}lamure_mesh_preprocessing -f ${OBJPATH} -raw -tkd ${KDTREE_TRI_BUDGET} -co ${COST_THRESHOLD} -tbvh ${TRI_BUDGET} -multi-max ${MAX_FINAL_TEX_SIZE}
    ;;
    * )
        echo "Going to create PNG texture file at the end."
        time ${LAMURE_DIR}lamure_mesh_preprocessing -f ${OBJPATH} -tkd ${KDTREE_TRI_BUDGET} -co ${COST_THRESHOLD} -tbvh ${TRI_BUDGET} -multi-max ${MAX_FINAL_TEX_SIZE}
    ;;
esac

# Logging routines
exec 1>&- 2>&-
wait ${TEEPID}
rm ${PIPEFILE}