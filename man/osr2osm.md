
# NAME

osr2osm - convert OSM data in Osmium binary format to OSM data files


# SYNOPSIS

**osr2osm** \[OPTIONS\] BINARY_FILE -o OSM_FILE


# DESCRIPTION

Convert OSM data from Osmium internal binary format into the usual OSM data
formats.



# OPTIONS

-h, --help
:   Show usage and list of commands.

--version
:   Show program version.

-o, --output
:   Name of OSM output file.

-O, --overwrite
:   Overwrite existing output file. If this is not set, the program will not
    write over an existing file.

-f, --output-format
:   The output format. Default is to derive the format from the suffix of the
    output file name.

--generator
:   The generator string for output file.


# SEE ALSO

* **osm2osr**(1)

