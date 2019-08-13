#!/bin/bash

# Logging routines
DATE=`date '+%Y-%m-%d:%H:%M:%S'`
PIPEFILE="${HOME}"/pipe"${DATE}"
mkfifo ${PIPEFILE}
tee log_mesh_preprocessing_${DATE}.txt < ${PIPEFILE} &
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
# settings
############################
# charting:
KDTREE_TRI_BUDGET=24000
COST_THRESHOLD=0.03 # max cost

# BVH hierarchy creation
TRI_BUDGET=16000

# texturing
MAX_FINAL_TEX_SIZE=65536
VT_PREPROCESSING_MEMORY_BUDGET_GB=50

FLIP_PNGS=No
SRC_OBJ=None
RUN_VT=Yes
RUN_MESH_FIXER=No
PRINT_HELP=No
CONVERT_TIFS_TO_JPG=No
############################


POSITIONAL=()
while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    -o|--objfile)
    SRC_OBJ="$2"
    shift # past argument
    shift # past value
    ;;
    -v|--vtbudget)
    VT_PREPROCESSING_MEMORY_BUDGET_GB="$2"
    shift # past argument
    shift # past value
    ;;
    -k|--kdtreebudget)
    KDTREE_TRI_BUDGET="$2"
    shift # past argument
    shift # past value
    ;;
    -t|--treebudget)
    TRI_BUDGET="$2"
    shift # past argument
    shift # past value
    ;;
    -c|--costthreshold)
    COST_THRESHOLD="$2"
    shift # past argument
    shift # past value
    ;;
    -m|--maxfinaltexsize)
    MAX_FINAL_TEX_SIZE="$2"
    shift # past argument
    shift # past value
    ;; 
    -f|--flippngs)
    FLIP_PNGS=Yes
    shift # past argument
    ;;
    -n|--novt)
    RUN_VT=No
    shift # past argument
    ;;  
    -x|--xfixer)
    RUN_MESH_FIXER=Yes
    shift # past argument
    ;;
    -j|--jpg)
    CONVERT_TIFS_TO_JPG=Yes
    shift # past argument
    ;;
    -h|--help)
    PRINT_HELP=Yes
    shift # past argument
    ;;                 
    --default)
    DEFAULT=YES
    shift # past argument
    ;;
    *)    # unknown option
    POSITIONAL+=("$1") # save it in an array for later
    shift # past argument
    ;;
esac
done
set -- "${POSITIONAL[@]}" # restore positional parameters

if [ "$SRC_OBJ" == "None" ] || [ "$PRINT_HELP" == "Yes" ]; then
   echo $0 "please specify objfile to preprocess with -o option"
   echo ""
   echo "other settings are follows:"
   echo "------------------------"
   echo "Triangle budget per KD-tree node: " ${KDTREE_TRI_BUDGET} "(set with option -k)"
   echo "Chart creation cost threshold: " ${COST_THRESHOLD} "(set with option -c)"
   echo "Triangle budget per BVH node: " ${TRI_BUDGET} "(set with option -t)"
   echo "Maximum size of final texture atlas: " ${MAX_FINAL_TEX_SIZE} "(set with option -m)"
   echo "Memory budget for virtual texture processing: " ${VT_PREPROCESSING_MEMORY_BUDGET_GB} "(set with option -v)"
   echo "Convert tifs to jpgs: " ${CONVERT_TIFS_TO_JPG} "(enable with option -j)"
   echo "Flip pngs: " ${FLIP_PNGS} "(enable with option -f)"
   echo "Run mesh fixer: " ${RUN_MESH_FIXER} "(enable with option -x)"
   echo "Run virtual texture processing: " ${RUN_VT} "(disable with option -n)"   
   echo "Processing obj model: " ${SRC_OBJ} "(set with option -o)"
   exit 0
fi

echo "RUNNING MESHLOD PIPELINE with settings as follows:"
echo "------------------------"
echo "Triangle budget per KD-tree node: " ${KDTREE_TRI_BUDGET}
echo "Chart creation cost threshold: " ${COST_THRESHOLD}
echo "Triangle budget per BVH node: " ${TRI_BUDGET}
echo "Maximum size of final texture atlas: " ${MAX_FINAL_TEX_SIZE}
echo "Memory budget for virtual texture processing: " ${VT_PREPROCESSING_MEMORY_BUDGET_GB}
echo "Convert tifs to jpgs: " ${CONVERT_TIFS_TO_JPG}
echo "Flip textures: " ${FLIP_PNGS}
echo "Run mesh fixer: " ${RUN_MESH_FIXER}
echo "Run virtual texture processing: " ${RUN_VT}
echo "Processing $SRC_OBJ"


MTL_NAME=None
MTL_NAME_BACKUP=None

if [ "$CONVERT_TIFS_TO_JPG" == "Yes" ]; then
    echo "converting tifs to jpgs"
    SRC_OBJ_NAME_WITHOUT_EXTENSION=${SRC_OBJ%.*}
    MTL_NAME=${SRC_OBJ_NAME_WITHOUT_EXTENSION}.mtl
    MTL_NAME_BACKUP=${SRC_OBJ_NAME_WITHOUT_EXTENSION}.mtl.backup
    Ycp ${MTL_NAME} ${MTL_NAME_BACKUP}
    sed -i -e 's/.tif/.jpg/g' ${MTL_NAME}
    mogrify -format jpg -quality 98 *tif
fi


if [ "$RUN_MESH_FIXER" == "Yes" ]; then
    echo "running mesh fixer"
    SRC_OBJ_NAME_WITHOUT_EXTENSION=${SRC_OBJ%.*}
    FIXED_SRC_OBJ=${SRC_OBJ_NAME_WITHOUT_EXTENSION}_fixed.obj
    time env DISPLAY=:0.0 ${LAMURE_DIR}lamure_mesh_fixer ${SRC_OBJ} ${FIXED_SRC_OBJ}
    SRC_OBJ=${FIXED_SRC_OBJ}
    echo "SRC_OBJ changed to " ${SRC_OBJ}
fi

# convert textures from jpg to png
echo converting jpgs to pngs
mogrify -format png *jpg

if [ "$FLIP_PNGS" == "Yes" ]; then
    echo "flipping textures"
    mogrify -flip *png
fi

if [ "$RUN_VT" == "Yes" ]; then
    echo "performing mesh preprocessing and texture atlas generation for " $SRC_OBJ
    time env DISPLAY=:0.0 ${LAMURE_DIR}lamure_mesh_preprocessing -f ${SRC_OBJ} -raw -tkd ${KDTREE_TRI_BUDGET} -co ${COST_THRESHOLD} -tbvh ${TRI_BUDGET} -multi-max ${MAX_FINAL_TEX_SIZE}
    SRC_OBJ_NAME_WITHOUT_EXTENSION=${SRC_OBJ%.*}
    echo "performing vt preprocessing for " $SRC_OBJ_NAME_WITHOUT_EXTENSION

    TEXTURE_ATLAS_DATA_NAME=`ls $SRC_OBJ_NAME_WITHOUT_EXTENSION*data`
    TEXTURE_ATLAS_DATA_NAME_WITHOUT_EXTENSION=${TEXTURE_ATLAS_DATA_NAME%.*}

    #echo $TEXTURE_ATLAS_DATA_NAME
    TEXTURE_ATLAS_NAME_TOKENS=`sed 's/_/\n/g' <<< $TEXTURE_ATLAS_DATA_NAME_WITHOUT_EXTENSION`
    #echo $TEXTURE_ATLAS_NAME_TOKENS
    TEXTURE_ATLAS_NAME_TOKENS_ARR=($TEXTURE_ATLAS_NAME_TOKENS)
    NUM_TOKENS=${#TEXTURE_ATLAS_NAME_TOKENS_ARR[@]}
    #echo $NUM_TOKENS ${TEXTURE_ATLAS_NAME_TOKENS_ARR[$NUM_TOKENS-1]}

    TEXTURE_ATLAS_WIDTH_TMP=${TEXTURE_ATLAS_NAME_TOKENS_ARR[$NUM_TOKENS-2]}
    TEXTURE_ATLAS_WIDTH=${TEXTURE_ATLAS_WIDTH_TMP#?}
    TEXTURE_ATLAS_HEIGHT_TMP=${TEXTURE_ATLAS_NAME_TOKENS_ARR[$NUM_TOKENS-1]}
    TEXTURE_ATLAS_HEIGHT=${TEXTURE_ATLAS_HEIGHT_TMP#?}
    #echo env DISPLAY=:0.0 ${LAMURE_DIR}lamure_vt_preprocessing process Domfigur_rgba_w16384_h16384.data rgba 16384 16384 256 256 1 Domfigur rgb 90
    time env DISPLAY=:0.0 ${LAMURE_DIR}lamure_vt_preprocessing process $TEXTURE_ATLAS_DATA_NAME rgba $TEXTURE_ATLAS_WIDTH $TEXTURE_ATLAS_HEIGHT 256 256 1 $SRC_OBJ_NAME_WITHOUT_EXTENSION rgb $VT_PREPROCESSING_MEMORY_BUDGET_GB

    # clean up
    rm *png *data
fi

if [ "$RUN_VT" == "No" ]; then
    echo "performing mesh preprocessing and texture atlas generation for " $SRC_OBJ
    time env DISPLAY=:0.0 ${LAMURE_DIR}lamure_mesh_preprocessing -f ${SRC_OBJ} -tkd ${KDTREE_TRI_BUDGET} -co ${COST_THRESHOLD} -tbvh ${TRI_BUDGET} -multi-max ${MAX_FINAL_TEX_SIZE}
    echo "Note that intermediate png files are kept"
fi


# clean up
if [ "$CONVERT_TIFS_TO_JPG" == "Yes" ]; then
    mv ${MTL_NAME_BACKUP} ${MTL_NAME}
    rm *jpg
fi
# optionally output a scaled down jpg one needs to calculate width and height
# env DISPLAY=:0.0 ${LAMURE_DIR}lamure_vt_preprocessing extract name.atlas 3 name.rgb 



# Logging routines
exec 1>&- 2>&-
wait ${TEEPID}
rm ${PIPEFILE}