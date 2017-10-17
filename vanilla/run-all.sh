#!/bin/bash

for ftl in "greedy" "dac" "faster"; do
    ./run.sh ${ftl}
done

