#!/bin/bash
#dilates textures in a given folder

SRC_DIR=$1

echo "start dilation script"

LAM_DIR="../.."
cd $LAM_DIR


for filename in /${SRC_DIR}/*uv*.png; do
	time ./install/bin/lamure_texture_dilation $filename "${filename:0:${#filename}-4}_dil.png" 1000

done
