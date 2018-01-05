# VST
This is VST is a virtual stress testing framework for discovering bugs in solid-state drives (SSDs). VST enables

* tracing and debugging SSD flash translation layers (FTL) without using real SSDs nor hardware tracers and debuggers
* stress testing SSD FTLs at a speed order(s) of magnitude faster than using real SSDs

TODO: add overview

Currently, VST has been applied to the ([Jasmine OpenSSD project](http://www.openssd-project.org/wiki/The_OpenSSD_Project)).

## VST-Jasmine
### Build Simulator

``` shell
git clone https://github.com/ssdlab-nthu/vst.git
cd vst/jasmine
make
```

### Build FTLs
There are currently three FTLs (Greedy, DAC and FASTer) in this repo.  

``` shell
cd ftl_greedy
make
```

### Run 

``` shell
./vst-jasmine <trace file> <ftl shared object> [-a]
```
A small synthetic trace file is proveded in the repo.  More trace files are available at, e.g., [MSRC](ftp://ftp.research.microsoft.com/pub/austind/MSRC-io-traces/).

Option `-a`  repeats the specified trace multiple times until the write amount reaches 1TB.

## Cite
If you use VST (or the debugged versions of the Greedy, DAC and FASTer FTLs) in your work, please cite our ICCAD’17 paper.  Thank you!

```
@inproceedings{iccad_2017_vst,
author={{R.-S. Liu and Y.-S. Chang and C.-W. Hung}},
booktitle={{IEEE/ACM International Conference on Computer-Aided Design (ICCAD)}},
title={{VST}: A virtual stress testing framework for discovering bugs in {SSD} flash-translation layers},
year={2017},
pages={283-290},
doi={10.1109/ICCAD.2017.8203790},
}
```