/*

EODB -- An experimental OSM database based on Libosmium.

Copyright (C) 2015-2017  Jochen Topf <jochen@topf.org>

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
#include <iostream>
#include <string>

// boost
#include <boost/program_options.hpp>

// osmium
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/location.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/types.hpp>
#include <osmium/index/map/dense_file_array.hpp>
#include <osmium/index/map/sparse_file_array.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/index/node_locations_map.hpp>

// eodb
#include "any_index.hpp"
#include "mapped_file.hpp"
#include "options.hpp"
#include "eodb.hpp"

typedef osmium::index::map::Map<osmium::unsigned_object_id_type, size_t> offset_index_type;
typedef osmium::index::map::Map<osmium::unsigned_object_id_type, osmium::Location> location_index_type;
typedef osmium::index::map::SparseFileArray<osmium::unsigned_object_id_type, osmium::Location> sparse_location_index_type;
typedef osmium::index::map::DenseFileArray<osmium::unsigned_object_id_type, osmium::Location> dense_location_index_type;

enum class operation_type {
    create,
    dump,
    lookup
};

class Options : public OptionsBase {

public:

    std::string index_type;
    operation_type operation = operation_type::create;
    osmium::unsigned_object_id_type id;

    void parse(int argc, char* argv[]) {
        try {
            namespace po = boost::program_options;

            po::options_description cmdline{"Allowed options"};
            cmdline.add_options()
                ("help,h", "Print this help message")
                ("version", "Show version")
                ("database", po::value<std::string>()->default_value(DEFAULT_EODB_NAME), "Database directory")
                ("index-type,i", po::value<std::string>(), "Index type ('sparse' or 'dense')")
                ("create,c", "Create index (default)")
                ("dump,d", "Dump index")
                ("lookup,l", po::value<uint64_t>(), "Lookup ID in index")
            ;

            po::options_description desc;
            desc.add(cmdline);

            po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
            po::notify(vm);

            check_version_option("eodb_locations_cache");

            if (vm.count("help")) {
                std::cout << "Usage: eodb_locations_cache XXX\n";
                std::cout << cmdline << "\n";
                std::exit(return_code::okay);
            }

            if (vm.count("index-type")) {
                index_type = vm["index-type"].as<std::string>();
                if (index_type != "sparse" && index_type != "dense") {
                    std::cerr << "Error: index-type has to be 'sparse' or 'dense'\n";
                    std::exit(return_code::fatal);
                }
            }

            if (vm.count("create") + vm.count("dump") + vm.count("lookup") > 1) {
                std::cerr << "Error: Only one of the options -c/--create, -d/--dump, and -l/--lookup allowed\n";
                std::exit(return_code::okay);
            }

            if (vm.count("create")) {
                operation = operation_type::create;
            }
            if (vm.count("dump")) {
                operation = operation_type::dump;
            }
            if (vm.count("lookup")) {
                operation = operation_type::lookup;
                id = vm["lookup"].as<osmium::unsigned_object_id_type>();
            }

        } catch (const boost::program_options::error& e) {
            std::cerr << "Error parsing command line: " << e.what() << '\n';
            std::exit(return_code::fatal);
        }
    }

    std::string locations_cache_file_name(const std::string& type = "") {
        std::string name = database().
            append("/locations.cache").
            append(".");
        if (!type.empty()) {
            index_type = type;
        }
        name.append(index_type);
        return name;
    }

}; // class Options

Options options;

std::string index_type_desc() {
    std::string s;
    s.append(options.index_type);
    s.append(",");
    s.append(options.locations_cache_file_name());
    return s;
}

int try_name(const std::string& name) {
    struct stat s;
    return ::stat(name.c_str(), &s);
}

int detect_index_type() {
    int fd = try_name(options.locations_cache_file_name("sparse"));

    if (fd == -1) {
        fd = try_name(options.locations_cache_file_name("dense"));
    }

    if (fd == -1) {
        std::cerr << "Can't find locations cache file\n";
        std::exit(return_code::fatal);
    }

    return fd;
}

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);

    options.parse(argc, argv);

    if (options.index_type.empty()) {
        options.index_type = detect_index_type();
    }

    if (options.operation == operation_type::create) {
        try {
            MappedFile mf{options.data_file_name()};

            osmium::memory::Buffer buffer{mf.data(), mf.size()};

            const auto& map_factory = osmium::index::MapFactory<osmium::unsigned_object_id_type, osmium::Location>::instance();
            std::unique_ptr<location_index_type> index = map_factory.create_map(index_type_desc());
            auto b = buffer.begin<osmium::Node>();
            auto e = buffer.end<osmium::Node>();
            for (auto it = b; it != e; ++it) {
                index->set(it->id(), it->location());
            }

            mf.close();
        } catch (const std::system_error& e) {
            std::cerr << e.what() << '\n';
            return return_code::fatal;
        }
    } else if (options.operation == operation_type::dump) {
        if (options.index_type == "sparse") {
            std::cerr << "dump sparse\n";
            const int fd = ::open(options.locations_cache_file_name("sparse").c_str(), O_RDWR);
            if (fd < 0) {
                std::cerr << "Can not open locations cache file: " << options.locations_cache_file_name("sparse") << ": " << std::strerror(errno) << "\n";
                std::exit(return_code::fatal);
            }
            sparse_location_index_type index(fd);
            for (const auto& p : index) {
                std::cout << p.first << " " << p.second << "\n";
            }
            close(fd);
        } else {
            std::cerr << "dump dense\n";
            int fd = ::open(options.locations_cache_file_name("dense").c_str(), O_RDWR);
            if (fd < 0) {
                std::cerr << "Can not open locations cache file: " << options.locations_cache_file_name("dense") << ": " << std::strerror(errno) << "\n";
                std::exit(return_code::fatal);
            }
            dense_location_index_type index(fd);
            int i = 0;
            for (const auto& p : index) {
                if (p) {
                    std::cout << i++ << " " << p << "\n";
                }
            }
            close(fd);
        }
    } else if (options.operation == operation_type::lookup) {
    }

    return return_code::okay;
}

