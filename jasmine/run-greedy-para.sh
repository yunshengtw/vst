#!/bin/bash

FTL=greedy
OPFILE=${FTL}-para.out

# 100 TB write
rm ${OPFILE}
parallel -j4 "./vst-jasmine {} ./ftl_${FTL}/ftl.so -b 109951162777600" ::: ../traces/*.trace 2>&1 | tee -a ${OPFILE}

