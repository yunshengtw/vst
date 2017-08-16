#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "usage: $0 <FTL>"
    exit 1
fi

FTL=$1
OBJ="./ftl_${FTL}/ftl.so"

if [ ! -f ${OBJ} ]; then
    echo "FTL object not exists"
    exit 1
fi

OPFILE=./output/${FTL}.out

rm ${OPFILE}
for t in ../traces/*.trace
do
    { time ./vst-jasmine ${t} ${OBJ} -a 2>&1 | tee -a ${OPFILE} ; } 2>>${OPFILE}
    echo "" | tee -a ${OPFILE}
done

