#!/bin/sh
#
#  create_and_update.sh
#

DATAFILE=...
CHANGEFILE=...

time eodb_create $DATAFILE

eodb_export -o data_export_1.osm.opl
eodb_dump -i nodes >nodes_export_1.csv
eodb_dump -i ways >ways_export_1.csv
eodb_dump -i relations >relations_export_1.csv

time eodb_update $CHANGEFILE

eodb_export -o data_export_2.osm.opl
eodb_dump -i nodes >nodes_export_2.csv
eodb_dump -i ways >ways_export_2.csv
eodb_dump -i relations >relations_export_2.csv

