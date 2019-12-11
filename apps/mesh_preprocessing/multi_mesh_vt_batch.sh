#!/bin/bash

if [[ -z "${LAMURE_DIR}" ]]; then
  echo -e "\e[31m"
  echo -e "To run this multi mesh vt_preprocessing script, define \e[1;4mLAMURE_DIR\e[24m\e[21m environmental variable, e.g.:"
  echo -e "\e[1;4mexport LAMURE_DIR=/opt/lamure/install/bin/\e[24m\e[21m"
  echo -e "You might want to add this line to your \e[1;4m.bashrc\e[24m\e[21m for persistent definition. Do not forget to follow it by:"
  echo -e "\e[1;4msource .bashrc\e[24m\e[21m"
  echo -e "\e[0m"
  exit
fi


if [[ "$#" < 2 ]]; then
	echo "usage: " $0 " filename.obj ... target_directory_name"
fi

WORKING_DIR=${PWD}
TARGET_DIR=None

for arg in "$@"
do
    TARGET_DIR="$arg"
done


for SRC_OBJ in "$@"
do
	if [[ "$SRC_OBJ" != "$TARGET_DIR" ]]; then
		echo "capturing textures from " "$SRC_OBJ" " into target directory " "$TARGET_DIR"
		OBJ_DIR=`dirname $SRC_OBJ`
		SRC_OBJ_BASE=`basename ${SRC_OBJ}`
		cd $OBJ_DIR
		${LAMURE_DIR}multi_mesh_vt_preprocessing.sh -r capture -o ${SRC_OBJ_BASE} -t ${TARGET_DIR}
		cd $WORKING_DIR
	fi
done

${LAMURE_DIR}multi_mesh_vt_preprocessing.sh -r atlas -t ${TARGET_DIR}

for SRC_OBJ in "$@"
do
	if [[ "$SRC_OBJ" != "$TARGET_DIR" ]]; then
		echo "adjusting texture coordinates for " "$SRC_OBJ" " and generating result into target directory " "$TARGET_DIR"
		OBJ_DIR=`dirname $SRC_OBJ`
		SRC_OBJ_BASE=`basename ${SRC_OBJ}`
		cd $OBJ_DIR
		${LAMURE_DIR}multi_mesh_vt_preprocessing.sh -r adjust -o ${SRC_OBJ_BASE} -t ${TARGET_DIR}
		cd $WORKING_DIR
	fi
done

# clean up based on atlas.log
INPUT=${TARGET_DIR}/atlas.log
OLDIFS=$IFS
IFS=','
[ ! -f $INPUT ] && { echo "$INPUT file not found"; exit 99; }
while read val0 val1 val2 val3 val4 val5 val6
do
	if [[ "$val6" != "" ]]; then
		rm -f ${TARGET_DIR}/${val6}
	fi
	
done < $INPUT
IFS=$OLDIFS
rm -f ${TARGET_DIR}/atlas.log

