#!/bin/bash

FTL=greedy
OPFILE=${FTL}-para.out
STRESS=1099511627776

# 100 TB write
rm ${OPFILE}
parallel -j4 "./vst-jasmine {} ./ftl_${FTL}/ftl.so -b ${STRESS}" ::: ../traces/*.trace 2>&1 | tee -a ${OPFILE}

