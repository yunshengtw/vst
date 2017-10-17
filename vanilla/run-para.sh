#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "usage: $0 <FTL> <# jobs>"
    exit 1
fi

FTL=$1
JOB=$2
OBJ="./ftl_${FTL}/ftl.so"

if [ ! -f ${OBJ} ]; then
    echo "FTL object not exists"
    exit 1
fi

OPFILE=./output/${FTL}-para-j${JOB}.out
# 1 TB write
STRESS=1099511627776

rm ${OPFILE}
parallel --no-notice -j${JOB} "./vst-jasmine {} ${OBJ} -b ${STRESS}" ::: ../traces/*.trace 2>&1 | tee -a ${OPFILE}

