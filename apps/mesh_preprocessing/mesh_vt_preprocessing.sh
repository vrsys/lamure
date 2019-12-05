#!/bin/bash

mkdir images

for i in `ls *tif`
do

name=${i%%.*}
targetname=${i%%.*}.png

echo generating $targetname from $i...

#convert -verbose -delete 1 $i $name_tmp.tif
#mv $name_tmp.tif $i
mogrify -verbose -format png $i

if [ ! -f $targetname ]; then
echo Error could not generate $targetname from $i
exit
fi

#echo flipping $targetname...
#mogrify -flip $targetname

mv $targetname images/$targetname

done

echo executing atlas compiler...

~/svn/lamure/install/bin/lamure_vt_atlas_compiler -d ./images/ -s 1.0 -o atlas.jpg

#echo executing vt preprocessing...

~/svn/lamure/install/bin/lamure_vt_preprocessing process atlas_rgb_w32768_h32768.data rgb 32768 32768 256 256 1 atlas rgb 16

#echo update log...
sed 's/png/tif/g' atlas.log > atlas_jpg.log

~/svn/lamure/install/bin/lamure_vt_obj_processor -obj model.obj -log atlas_jpg.log -atlas atlas.atlas

#~/svn/lamure/install/bin/lamure_vt_obj_renderer -f model_vt.obj -t atlas.atlas


