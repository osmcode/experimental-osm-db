/*

EODB -- An experimental OSM database based on Libosmium.

Copyright (C) 2015  Jochen Topf <jochen@topf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

// c++
#include <cerrno>
#include <cstring>
#include <getopt.h>
#include <iostream>
#include <set>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

#ifdef _MSC_VER
# include <direct.h>
#endif

// boost
#include <boost/program_options.hpp>

// osmium
#include <osmium/io/any_input.hpp>

// osmium indexes
#include <osmium/index/map/dense_file_array.hpp>
#include <osmium/index/map/dummy.hpp>
#include <osmium/index/map/sparse_file_array.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/index/map/sparse_mem_map.hpp>
#include <osmium/index/map/sparse_mem_table.hpp>
#include <osmium/index/map/sparse_mmap_array.hpp>

// eodb
#include "eodb.hpp"
#include "options.hpp"
#include "any_index.hpp"
#include "updatable_disk_store.hpp"

typedef osmium::index::map::Map<osmium::unsigned_object_id_type, size_t> offset_index_type;

#ifdef OSMIUM_HAS_INDEX_MAP_DENSE_FILE_ARRAY
    REGISTER_MAP(osmium::unsigned_object_id_type, size_t, osmium::index::map::DenseFileArray, dense_file_array);
#endif

#ifdef OSMIUM_HAS_INDEX_MAP_SPARSE_FILE_ARRAY
    REGISTER_MAP(osmium::unsigned_object_id_type, size_t, osmium::index::map::SparseFileArray, sparse_file_array);
#endif

#ifdef OSMIUM_HAS_INDEX_MAP_SPARSE_MEM_ARRAY
    REGISTER_MAP(osmium::unsigned_object_id_type, size_t, osmium::index::map::SparseMemArray, sparse_mem_array);
#endif

#ifdef OSMIUM_HAS_INDEX_MAP_SPARSE_MEM_MAP
    REGISTER_MAP(osmium::unsigned_object_id_type, size_t, osmium::index::map::SparseMemMap, sparse_mem_map);
#endif

#ifdef OSMIUM_HAS_INDEX_MAP_SPARSE_MEM_TABLE
    REGISTER_MAP(osmium::unsigned_object_id_type, size_t, osmium::index::map::SparseMemTable, sparse_mem_table);
#endif

#ifdef OSMIUM_HAS_INDEX_MAP_SPARSE_MMAP_ARRAY
    REGISTER_MAP(osmium::unsigned_object_id_type, size_t, osmium::index::map::SparseMmapArray, sparse_mmap_array);
#endif

class Options : public OptionsBase {

public:

    void parse(int argc, char* argv[]) {
        std::set<std::string> index_types = {
            "dense_file_array",
            "sparse_file_array",
            "sparse_mem_array",
            "sparse_mem_map",
            "sparse_mem_table",
            "sparse_mmap_array"
        };

        try {
            namespace po = boost::program_options;

            po::options_description cmdline{"Allowed options"};
            cmdline.add_options()
                ("help,h", "Print this help message")
                ("version", "Show version")
                ("database,d", po::value<std::string>()->default_value(DEFAULT_EODB_NAME), "Database directory")
            ;

            po::options_description hidden{"Hidden options"};
            hidden.add_options()
                ("input-filenames", po::value<std::vector<std::string>>(), "Input files")
            ;

            po::options_description desc;
            desc.add(cmdline).add(hidden);

            po::positional_options_description positional;
            positional.add("input-filenames", -1);

            po::store(po::command_line_parser(argc, argv).options(desc).positional(positional).run(), vm);
            po::notify(vm);

            check_version_option("eodb_create");

            if (vm.count("help")) {
                std::cout << "Usage: eodb_update [OPTIONS] OSM-CHANGE-FILE...\n";
                std::cout << "Update database from OSM change files.\n\n";
                std::cout << cmdline << "\n";
                std::cout << "Index types:\n";
                for (const auto& index_type : index_types) {
                    std::cout << "  " << index_type << "\n";
                }
                std::exit(return_code::okay);
            }

        } catch (const boost::program_options::error& e) {
            std::cerr << "Error parsing command line: " << e.what() << '\n';
            std::exit(return_code::fatal);
        }
    }

}; // class Options

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);

    Options options;
    options.parse(argc, argv);

    const int data_fd = ::open(options.data_file_name().c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (data_fd < 0) {
        std::cerr << "Can't open data file '" << options.data_file_name() << "': " << std::strerror(errno) << "\n";
        std::exit(return_code::fatal);
    }

    bool dense = true; // only works for dense currently
    std::string index_type{(dense ? "dense_file_array" : "sparse_file_array")};

    const auto& map_factory = osmium::index::MapFactory<osmium::unsigned_object_id_type, size_t>::instance();
    std::unique_ptr<offset_index_type> node_index     = map_factory.create_map(index_type + "," + index_name(options.database(), "nodes", dense));
    std::unique_ptr<offset_index_type> way_index      = map_factory.create_map(index_type + "," + index_name(options.database(), "ways", dense));
    std::unique_ptr<offset_index_type> relation_index = map_factory.create_map(index_type + "," + index_name(options.database(), "relations", dense));
    UpdatableDiskStore disk_store_handler{data_fd, *node_index, *way_index, *relation_index};

    for (const auto& fn : options.input_filenames()) {
        osmium::io::Reader reader{fn};

        while (osmium::memory::Buffer buffer = reader.read()) {
            disk_store_handler(buffer);
        }

        reader.close();
    }

    return return_code::okay;
}

