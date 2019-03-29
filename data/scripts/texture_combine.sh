#!/bin/bash
#provide the name of the directory as 2nd argument
# assumes name format <nameofobj>uv<TEXTURENUM>_dil.png

SRC_DIR=$1

echo "start texture combination script"

LAM_DIR="../.."
cd $LAM_DIR

MONTAGE_COMMAND="montage "

for filename in /${SRC_DIR}/*uv*_dil.png; do
	MONTAGE_COMMAND="${MONTAGE_COMMAND} ${filename} "
done

MONTAGE_COMMAND="${MONTAGE_COMMAND} -geometry +0+0 ${SRC_DIR}/combined_texture.png"

$MONTAGE_COMMAND