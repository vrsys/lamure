#!/bin/bash

if test "$#" -ne 2; then
    echo "usage: $0 <in_out_name>.png <num dilation iterations>"
    exit 1
fi

for dilation_iteration in `seq 1 "$2"`;
do
  ./lamure_texture_dilation $1 $1
done