#!/bin/csh -f
# script creates 100 copies of the sample image in /tmp
# then uses the jrcl client to send them to the server
# for resizing in batch mode
# 
# $IMG is the image to be processed
# $TMP is the scratch filesystem to use for input and output
#    NOTE: for best performance this should be a tmpfs or SSD
# $NIM is the number of copies to process in batch
# $OSIZE is the percentage to rescale the image
# $SERVER is the system running the jrd resizer server
#    NOTE: assumes jrd is already running...if not
#    start it with "jrd --threads 16" before running
#    this script
# 

set IMG=image058r
set TMP=/tmp
set NIM=100
set OSIZE=50
set SERVER=localhost

# this loop creates 100 copies of the image
set i=1
set FILES=""
echo "" >${TMP}/XJOBS
while ( $i <= $NIM )
  cp ${IMG}.jpg ${TMP}/${IMG}-${i}.jpg
  set FILES="$FILES ${TMP}/${IMG}-${i}.jpg"
  echo "${TMP}/${IMG}-${i}.jpg ${TMP}/${IMG}-${i}_resized.jpg" >>${TMP}/XJOBS
  @ i++
end

# and this resizes the files created above
echo "resizing ${IMG}.jpg to $OSIZE per cent"
set CMD="jrcl --server $SERVER --scale ${OSIZE} --threads 12 --batch ${TMP}/XJOBS"
echo $CMD
time $CMD
set NOUT=`ls ${TMP}/${IMG}-*_resized.jpg | wc -l`
echo "resized ${IMG}.jpg $NOUT times"
echo ""



