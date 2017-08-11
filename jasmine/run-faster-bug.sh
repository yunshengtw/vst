#!/bin/bash
FTL=faster_bug
OP_FILE=${FTL}.out

rm ${OP_FILE}
for t in ../traces/*.trace
do
    { time ./vst-jasmine ${t} ./ftl_${FTL}/ftl.so -a 2>&1 | tee -a ${OP_FILE} ; } 2>>${OP_FILE}
    echo "" | tee -a ${OP_FILE}
done

