#!/bin/bash

# Logging routines
DATE=`date '+%Y-%m-%d:%H:%M:%S'`
PIPEFILE="${HOME}"/pipe"${DATE}"
mkfifo ${PIPEFILE}
tee log_multi_mesh_vt_preprocessing_${DATE}.txt < ${PIPEFILE} &
TEEPID=$!
exec > ${PIPEFILE} 2>&1

# Requires path of OBJ as argument

if [[ -z "${LAMURE_DIR}" ]]; then
  echo -e "\e[31m"
  echo -e "To run this multi mesh vt_preprocessing script, define \e[1;4mLAMURE_DIR\e[24m\e[21m environmental variable, e.g.:"
  echo -e "\e[1;4mexport LAMURE_DIR=/opt/lamure/install/bin/\e[24m\e[21m"
  echo -e "You might want to add this line to your \e[1;4m.bashrc\e[24m\e[21m for persistent definition. Do not forget to follow it by:"
  echo -e "\e[1;4msource .bashrc\e[24m\e[21m"
  echo -e "\e[0m"
  exit
fi

############################
# settings
############################
# run modes
# 1. capture images
# 2. generate atlas and log
# 3. create _vt.obj using log
RUN_MODE=None
TMP_TEXTURE_DIR=None
# texturing
ATLAS_SIZE=1.0
VT_PREPROCESSING_MEMORY_BUDGET_GB=50

SRC_OBJ=None
PRINT_HELP=No
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
    -a|--atlas_size)
    ATLAS_SIZE="$2"
    shift # past argument
    shift # past value
    ;;
    -r|--run_mode)
    RUN_MODE="$2"
    shift # past argument
    shift # past value
    ;;
    -t|--tmp_texture_dir)
    TMP_TEXTURE_DIR="$2"
    shift # past argument
    shift # past value
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

if [ "$RUN_MODE" == "None" ] || [ "$PRINT_HELP" == "Yes" ]; then
   echo "ERROR" $0 "please specify run mode with -r option"
   echo ""
   echo "other settings are follows:"
   echo "------------------------"
   echo "Memory budget for virtual texture processing: " ${VT_PREPROCESSING_MEMORY_BUDGET_GB} "(set with option -v)"
   echo "Atlas size: " ${ATLAS_SIZE} "(set with option -a)"
   echo "Processing obj model: " ${SRC_OBJ} "(set with option -o)"
   echo "Run mode: " ${RUN_MODE} "(set with option -r can be: capture atlas adjust)"
   exit 0
fi

echo "RUNNING MULTI MESH VT PIPELINE with settings as follows:"
echo "------------------------"
echo "Entering run mode " ${RUN_MODE}
echo "Memory budget for virtual texture processing: " ${VT_PREPROCESSING_MEMORY_BUDGET_GB}
echo "Atlas size: " ${ATLAS_SIZE}


if [ "$RUN_MODE" == "capture" ]; then
    if [ "$SRC_OBJ" == "None" ]; then
      echo "ERROR" $0 "please specify obj with -o option"
      exit 0
    fi
    if [ "$TMP_TEXTURE_DIR" == "None" ]; then
      echo "ERROR" $0 "please specify temp texture dir -t option"
      exit 0
    fi
    echo "Copying textures of $SRC_OBJ to $TMP_TEXTURE_DIR"

    SRC_OBJ_NAME_WITHOUT_EXTENSION=${SRC_OBJ%.*}
    SRC_MTL_FILE=${SRC_OBJ_NAME_WITHOUT_EXTENSION}.mtl
    IMAGE_TYPE=None
    if grep -q ".tiff" "$SRC_MTL_FILE"; then
      IMAGE_TYPE=".tiff"
    elif grep -q ".tif" "$SRC_MTL_FILE"; then
      IMAGE_TYPE=".tif"
    elif grep -q ".png" "$SRC_MTL_FILE"; then
      IMAGE_TYPE=".png"
    elif grep -q ".jpg" "$SRC_MTL_FILE"; then
      IMAGE_TYPE=".jpg"
    elif grep -q ".jpeg" "$SRC_MTL_FILE"; then
      IMAGE_TYPE=".jpeg"
    fi

    if [ "$IMAGE_TYPE" == "None" ]; then
      echo "ERROR" $0 ": $SRC_MTL_FILE does not contain valid textures(.tiff, .tif, .jpg, .png)"
      exit 0
    else
      echo "$SRC_MTL_FILE uses image type $IMAGE_TYPE"
    fi

    mkdir -p ${TMP_TEXTURE_DIR}
    cp *${IMAGE_TYPE} ${TMP_TEXTURE_DIR}
fi


if [ "$RUN_MODE" == "atlas" ]; then
    if [ "$TMP_TEXTURE_DIR" == "None" ]; then
      echo "ERROR" $0 "please specify temp texture dir -t option"
      exit 0
    fi  
    echo "performing texture atlas generation"
    
    time env DISPLAY=:0.0 ${LAMURE_DIR}lamure_vt_atlas_compiler -d ${TMP_TEXTURE_DIR} -s ${ATLAS_SIZE} -o atlas.jpg

    echo "generating " atlas

    TEXTURE_ATLAS_DATA_NAME=`ls atlas*.data`
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
    time env DISPLAY=:0.0 ${LAMURE_DIR}lamure_vt_preprocessing process $TEXTURE_ATLAS_DATA_NAME rgb $TEXTURE_ATLAS_WIDTH $TEXTURE_ATLAS_HEIGHT 256 256 1 atlas rgb $VT_PREPROCESSING_MEMORY_BUDGET_GB
    rm atlas*.data

    mv atlas.atlas atlas.log ${TMP_TEXTURE_DIR}
fi


if [ "$RUN_MODE" == "adjust" ]; then
    if [ "$SRC_OBJ" == "None" ]; then
      echo "ERROR" $0 "please specify obj with -o option"
      exit 0
    fi
    if [ "$TMP_TEXTURE_DIR" == "None" ]; then
      echo "ERROR" $0 "please specify temp texture dir -t option"
      exit 0
    fi  
    SRC_OBJ_NAME_WITHOUT_EXTENSION=${SRC_OBJ%.*}
    echo "adjusting texture coordinates in " $SRC_OBJ " and generating " ${SRC_OBJ_NAME_WITHOUT_EXTENSION}_vt.obj
    time env DISPLAY=:0.0 ${LAMURE_DIR}lamure_vt_obj_processor -obj $SRC_OBJ -log ${TMP_TEXTURE_DIR}/atlas.log -atlas ${TMP_TEXTURE_DIR}/atlas.atlas
    mv ${SRC_OBJ_NAME_WITHOUT_EXTENSION}_vt.obj ${TMP_TEXTURE_DIR}
fi

# Logging routines
exec 1>&- 2>&-
wait ${TEEPID}
rm ${PIPEFILE}

exit 0
