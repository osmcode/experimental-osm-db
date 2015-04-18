# EODB - Experimental OSM Database

Code for an experimental OSM database. The idea is to explore the possibilities
of building some kind of OSM database on top of Libosmium. The database can
be created from a planet file and later updated from change files. It can then
be used to derive other data formats that can be used for rendering etc.

This code is EXPERIMENTAL, it is not intended to be used for anything beyond
trying out different approaches to the problem. It is very preliminary, poorly
documented and will probably not work for you. :-)


## Prerequisites

You'll need Libosmium (http://osmcode.org/libosmium) and its dependencies
installed first.


## Building

This code uses CMake for its builds. For Unix/Linux systems a simple
Makefile wrapper is provided to make the build even easier.

To build just type `make`. Results will be in the `build` subdirectory.

Or you can go the long route explicitly calling CMake as follows:

    mkdir build
    cd build
    cmake ..
    make


## Running

There are several programs implementing different parts of the "database". Use
`eodb_create` to create a "database" from any OSM data file. Use `eodb_export`
to write (part of) the database into an OSM file. Use `eodb_dump` to dump
indexes and maps to stdout, and use `eodb_lookup` to do index and map lookups.


## Database Format

This code uses a simple database format. Each "database" is a directory with
the following files:

* `data.osr`: OSM data itself in Osmium internal binary format.
* `nodes.sparse.idx`, `ways.sparse.idx`, and `relations.sparse.idx`: Index
  mapping object ID to offset in `data.osr`. Instead of `sparse` it can
  also be called `dense`.
* `node2way.map`: Index mapping node IDs to the IDs of ways that contain those
  nodes.
* `node2relation.map`, `way2relation.map`, and `relation2relation.map`. Index
  mapping member IDs to the IDs of the relations with those members.
* `locations.sparse.idx` or `locations.dense.idx`: Node locations indexed
  by node ID.


## Index Formats

Osmium DB supports two different types of indexes called *dense* and *sparse*.

* The *dense* index is used for large datasets, such as the whole planet or
  continent-sized extracts. The data is stored in a large array and can be
  directly looked up by ID. The size of the index only depends on the largest
  ID used and the size of the value to be looked up.
* The *sparse* index is used for small datasets, such as city- or country-sized
  extracts. The data is stored as pairs of ID and Value ordered by ID. The
  lookup is by binary search. The size depends on the number of IDs used
  and the size of ID and value.

File names in the database directory will reflect the type of index used.


## License

This software is released unter the GPL v3. See LICENSE.txt for details.


## Author

Jochen Topf (http://jochentopf.com/)


