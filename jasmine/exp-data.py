#!/usr/bin/python

import re
import sys
import csv

def extract_time(f):
    ls = []
    for l in f:
        if re.search('real', l):
            #t = l.strip().split('\t')
            t = l.strip().split('\t')[1]
            m = t.split('m')[0]
            s = t.split('m')[1].split('s')[0]
            ls.append(str(float(m) * 60 + float(s)))
    f.seek(0)
    return ls

def extract_other(f, s, n):
    ls = []
    for l in f:
        if re.search(s, l):
            ls.append(l.strip().split(' ')[n])
    f.seek(0)
    return ls

f = open(sys.argv[1], "r")

total_read = extract_other(f, 'Total read', 3)
total_write = extract_other(f, 'Total write', 3)
time = extract_time(f)
nand_read = extract_other(f, 'Total flash read', 4)
nand_write = extract_other(f, 'Total flash write', 4)
nand_copyback = extract_other(f, 'Total flash copyback', 4)
nand_erase = extract_other(f, 'Total flash erase', 4)

data = zip(total_read, total_write, time, nand_read, nand_write, nand_copyback, nand_erase)

with open(sys.argv[2], 'wb') as ofile:
    wr = csv.writer(ofile)
    for row in data:
        wr.writerow(row)

