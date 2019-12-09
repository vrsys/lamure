#!/bin/bash

# Logging routines
DATE=`date '+%Y-%m-%d:%H:%M:%S'`
PIPEFILE="${HOME}"/pipe"${DATE}"
mkfifo ${PIPEFILE}
tee log_mesh_vt_preprocessing_${DATE}.txt < ${PIPEFILE} &
TEEPID=$!
exec > ${PIPEFILE} 2>&1

# Requires path of OBJ as argument

if [[ -z "${LAMURE_DIR}" ]]; then
  echo -e "\e[31m"
  echo -e "To run this mesh vt_preprocessing script, define \e[1;4mLAMURE_DIR\e[24m\e[21m environmental variable, e.g.:"
  echo -e "\e[1;4mexport LAMURE_DIR=/opt/lamure/install/bin/\e[24m\e[21m"
  echo -e "You might want to add this line to your \e[1;4m.bashrc\e[24m\e[21m for persistent definition. Do not forget to follow it by:"
  echo -e "\e[1;4msource .bashrc\e[24m\e[21m"
  echo -e "\e[0m"
  exit
fi

############################
# settings
############################

# texturing
ATLAS_SIZE=1.0
VT_PREPROCESSING_MEMORY_BUDGET_GB=50

FLIP_PNGS=No
SRC_OBJ=None
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
    -a|--atlas_size)
    ATLAS_SIZE="$2"
    shift # past argument
    shift # past value
    ;; 
    -f|--flippngs)
    FLIP_PNGS=Yes
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
   echo "Memory budget for virtual texture processing: " ${VT_PREPROCESSING_MEMORY_BUDGET_GB} "(set with option -v)"
   echo "Atlas size: " ${ATLAS_SIZE} "(set with option -a)"
   echo "Convert tifs to jpgs: " ${CONVERT_TIFS_TO_JPG} "(enable with option -j)"
   echo "Flip textures: " ${FLIP_PNGS} "(enable with option -f)"
   echo "Processing obj model: " ${SRC_OBJ} "(set with option -o)"
   exit 0
fi

echo "RUNNING MESH VT PIPELINE with settings as follows:"
echo "------------------------"
echo "Memory budget for virtual texture processing: " ${VT_PREPROCESSING_MEMORY_BUDGET_GB}
echo "Atlas size: " ${ATLAS_SIZE}
echo "Convert tifs to jpgs: " ${CONVERT_TIFS_TO_JPG}
echo "Flip textures: " ${FLIP_PNGS}
echo "Processing $SRC_OBJ"


MTL_NAME=None
MTL_NAME_BACKUP=None

if [ "$CONVERT_TIFS_TO_JPG" == "Yes" ]; then
    echo "converting tifs to jpgs"
    SRC_OBJ_NAME_WITHOUT_EXTENSION=${SRC_OBJ%.*}
    MTL_NAME=${SRC_OBJ_NAME_WITHOUT_EXTENSION}.mtl
    MTL_NAME_BACKUP=${SRC_OBJ_NAME_WITHOUT_EXTENSION}.mtl.backup
    cp ${MTL_NAME} ${MTL_NAME_BACKUP}
    sed -i -e 's/.tif/.jpg/g' ${MTL_NAME}



    for i in `ls *tif`
    do

    name=${i%%.*}
    targetname=${i%%.*}.jpg

    echo generating $targetname from $i

    convert -verbose -delete 1 $i $name_tmp.tif
    mv $name_tmp.tif $i
    mogrify -verbose -format jpg $i

    if [ ! -f $targetname ]; then
        echo Error could not generate $targetname from $i
        exit
    fi
    done
    
fi


# convert textures from jpg to png
echo converting jpgs to pngs
mogrify -format png *jpg

if [ "$FLIP_PNGS" == "Yes" ]; then
    echo "flipping textures"
    mogrify -flip *png
fi

mkdir -p tmp_images
mv *png tmp_images


echo "performing texture atlas generation for " $SRC_OBJ
SRC_OBJ_NAME_WITHOUT_EXTENSION=${SRC_OBJ%.*}
time env DISPLAY=:0.0 ${LAMURE_DIR}lamure_vt_atlas_compiler -d ./tmp_images/ -s ${ATLAS_SIZE} -o ${SRC_OBJ_NAME_WITHOUT_EXTENSION}.jpg

echo "generating " ${SRC_OBJ_NAME_WITHOUT_EXTENSION}.atlas

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
time env DISPLAY=:0.0 ${LAMURE_DIR}lamure_vt_preprocessing process $TEXTURE_ATLAS_DATA_NAME rgb $TEXTURE_ATLAS_WIDTH $TEXTURE_ATLAS_HEIGHT 256 256 1 $SRC_OBJ_NAME_WITHOUT_EXTENSION rgb $VT_PREPROCESSING_MEMORY_BUDGET_GB

echo "adjusting texture coordinates in " $SRC_OBJ " and generating " ${SRC_OBJ_NAME_WITHOUT_EXTENSION}_vt.obj
#echo update log...
sed -i -e 's/.png/.jpg/g' ${SRC_OBJ_NAME_WITHOUT_EXTENSION}.log

time env DISPLAY=:0.0 ${LAMURE_DIR}lamure_vt_obj_processor -obj $SRC_OBJ -log ${SRC_OBJ_NAME_WITHOUT_EXTENSION}.log -atlas ${SRC_OBJ_NAME_WITHOUT_EXTENSION}.atlas

# clean up
if [ "$CONVERT_TIFS_TO_JPG" == "Yes" ]; then
    mv ${MTL_NAME_BACKUP} ${MTL_NAME}
    rm -f *jpg
fi

rm -rf tmp_images
rm -f *.data
rm -f *.log


# Logging routines
exec 1>&- 2>&-
wait ${TEEPID}
rm ${PIPEFILE}

exit 0
