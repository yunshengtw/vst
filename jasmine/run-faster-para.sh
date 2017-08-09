#!/bin/bash

parallel -j4 "./vst-jasmine {} ./ftl_faster/ftl.so -b 109951162777600" ::: ../traces/*.trace 2>&1 | tee -a faster-para.out

