
# NAME

osm2osr - convert OSM data file into binary format


# SYNOPSIS

**osm2osr** \[OPTIONS\] OSM_FILE -o BINARY_FILE


# DESCRIPTION

Convert OSM data file into Osmium internal binary format.

The format might be different for different machine architectures, operating
systems, compilers, and Osmium versions.


# OPTIONS

-h, --help
:   Show usage and list of commands.

--version
:   Show program version.

-o, --output
:   Name of binary output file

-O, --overwrite
:   Overwrite existing output file. If this is not set, the program will not
    write over an existing file.

-a, --append
:   Append to output file instead of overwriting it.


# SEE ALSO

* **osr2osm**(1)

