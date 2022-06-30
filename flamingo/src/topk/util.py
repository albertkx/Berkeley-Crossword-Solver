# $Id: util.py 4786 2009-11-21 20:14:53Z rares $
#
# Copyright (C) 2007 by The Regents of the University of California
#
# Redistribution of this file is permitted under the terms of the BSD
# license
#
# Date: 08/28/2008
#
# Author: Rares Vernica <rares (at) ics.uci.edu>
#

import pickle
import numpy

datasets = {
    'IMDB' : {
    'filename'  : '/data/imdb/actors.rank.shuf.csv', 
    'separator' : '\t', 
    }, 
    'WebCorpus' : {
    'filename'  : '/data/webcorpus/webcorpus.shuf.2.4m.txt', 
    'separator' : '\t', 
    }, 
    'Bio' : {
    'filename'  : '/data/topk/bio/CHEMDB_SAMPLE_UNCOMP_100K.csv', 
    'separator' : ',', 
    }, 
    }

def data_freq(name):
    dataset = datasets[name]
    filename = dataset['filename']
    separator = dataset['separator']

    freq = {}
    for line in open(filename):
        f = int(line[:-1].split(separator)[1])
        try:
            freq[f] += 1
        except:
            freq[f] = 1

    pickle.dump(freq, open(filename[:filename.rfind('.') + 1] + 'freq.p', 'wb'))

    val = freq.keys()
    val.sort()
    for v in val:
        print v, freq[v]

def token_freq(name):
    dataset = datasets[name]
    filename = dataset['filename']
    separator = dataset['separator']

    freq = {}
    for line in open(filename):
        for token in line.split(separator):
            f = int(token)
            try:
                freq[f] += 1
            except:
                freq[f] = 1

    # pickle.dump(freq, open(filename[:filename.rfind('.') + 1] + 'freq.p', 'wb'))

    # val = freq.keys()
    # val.sort()
    # for v in val:
    #     print v, freq[v]

    a = numpy.array(freq.values())
    print len(a)
    print a.mean()
    print a.std()
            
def length_freq(name):
    dataset = datasets[name]
    filename = dataset['filename']
    separator = dataset['separator']

    freq = {}
    total = count = 0
    for line in open(filename):
        f = len(line.split(separator))
        total += f
        count += 1
        # try:
        #     freq[f] += 1
        # except:
        #     freq[f] = 1

    # for v in freq.keys():
    #     print v, freq[v]

    print float(total) / count

token_freq('Bio')

# a = numpy.array([1, 2, 3])
# print a.std()
# print a.mean()
