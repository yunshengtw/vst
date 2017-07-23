#!/bin/bash
FTL=dac
OP_FILE=run-${FTL}.out

rm ${OP_FILE}
for t in ../traces/*.trace
do
    ./vst-jasmine ${t} ./ftl_${FTL}/ftl.so -a 2>&1 | tee -a ${OP_FILE}
    echo "" | tee -a ${OP_FILE}
done

