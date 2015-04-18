#!/bin/sh
#
#  classic_vs_leveldb.sh
#

DATA=...

time ./eodb_create         -d /tmp/classic.eodb -m -i sparse_file_array $DATA
time ./eodb_create_leveldb -d /tmp/leveldb.eodb                         $DATA

./eodb_dump         -d /tmp/classic.eodb -m n2w >classic.n2w.dump
./eodb_dump_leveldb -d /tmp/leveldb.eodb -m n2w | sort -k1n -k2n >leveldb.n2w.dump

