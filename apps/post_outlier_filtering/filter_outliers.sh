#!/bin/bash

if [ "$#" -ne 4 ]; then
    echo "Usage: $0 <BVH_DIRECTORY> <OUTLIER_THRESHOLD_START> <OUTLIER_THRESHOLD_END> <OUTLIER_THRESHOLD_STEP_SIZE>\n"
    exit -1
fi

bvh_directory=$1

start_outlier_threshold=$2
end_outlier_threshold=$3
step_size=$4

for outlier_threshold in $(seq $start_outlier_threshold $step_size $end_outlier_threshold); do
mkdir ${bvh_directory}/outlier_filtered_$outlier_threshold

for bvh_file in "${bvh_directory}"/*.bvh; do

echo "BVH FILE $bvh_file"

bvh_name_without_path=$(basename -- "$bvh_file")

start_ts=$(date) 
echo "\nSTART Filtering $bvh_name_without_path with threshold ${outlier_threshold} @ $start_ts"

./lamure_post_outlier_filtering -f $bvh_file -o ${bvh_directory}/outlier_filtered_${outlier_threshold}/${bvh_name_without_path} -r ${outlier_threshold}

end_ts=$(date)
echo "END Filtering $bvh_name_without_path with threshold ${outlier_threshold} @ $end_ts"

done

done
