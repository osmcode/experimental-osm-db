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
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>

// boost
#include <boost/program_options.hpp>

// osmium
#include <osmium/index/map/dense_file_array.hpp>
#include <osmium/index/map/sparse_file_array.hpp>
#include <osmium/index/multimap/sparse_file_array.hpp>
#include <osmium/osm/types.hpp>
#include <osmium/osm/location.hpp>

// eodb
#include "options.hpp"
#include "eodb.hpp"

class Options : public OptionsBase {

public:

    void parse(int argc, char* argv[]) {
        try {
            namespace po = boost::program_options;

            po::options_description desc{"Allowed options"};
            desc.add_options()
                ("help,h", "Print this help message")
                ("version", "Show version")
                ("database,d", po::value<std::string>()->default_value(DEFAULT_EODB_NAME), "Database directory")
                ("index,i", po::value<std::string>(), "Name of index")
                ("map,m", po::value<std::string>(), "Name of map")
            ;

            po::store(po::parse_command_line(argc, argv, desc), vm);
            po::notify(vm);

            check_version_option("eodb_dump");

            if (vm.count("help")) {
                std::cout << "Usage: eodb_dump [OPTIONS]\n";
                std::cout << "Dump index/map data from database.\n\n";
                std::cout << desc << "\n";
                std::cout << "Indexes: n(odes), w(ays), r(elations), l(locations)\n";
                std::cout << "Maps: n(ode)2w(ay), n(ode)2r(elation), w(ay)2r(elation), r(elation)2r(elation)\n";
                std::exit(return_code::okay);
            }

            if (!!vm.count("index") == !!vm.count("map")) {
                std::cerr << "Please use exactly one of the options --index,-i or --map,-m.\n";
                std::exit(return_code::fatal);
            }

            if (vm.count("index")) {
                if (index() != "nodes" && index() != "ways" && index() != "relations" && index() != "locations") {
                    std::cerr << "Index given with --index,-i must be one of: nodes, ways, relations, locations\n";
                    std::exit(return_code::fatal);
                }
            }

            if (vm.count("map")) {
                if (map() != "node2way" && map() != "node2relation" && map() != "way2relation" && map() != "relation2relation") {
                    std::cerr << "Map given with --map,-m must be one of: node2way, node2relation, way2relation, relation2relation\n";
                    std::exit(return_code::fatal);
                }
            }

        } catch (const boost::program_options::error& e) {
            std::cerr << "Error parsing command line: " << e.what() << '\n';
            std::exit(return_code::fatal);
        }
    }

    bool do_index() const {
        return vm.count("index") != 0;
    }

    std::string index() const {
        std::string index = vm["index"].as<std::string>();
        if (index == "n") {
            index = "nodes";
        } else if (index == "w") {
            index = "ways";
        } else if (index == "r") {
            index = "relations";
        } else if (index == "l") {
            index = "locations";
        }
        return index;
    }

    std::string map() const {
        std::string map = vm["map"].as<std::string>();
        if (map == "n2w") {
            map = "node2way";
        } else if (map == "n2r") {
            map = "node2relation";
        } else if (map == "w2r") {
            map = "way2relation";
        } else if (map == "r2r") {
            map = "relation2relation";
        }
        return map;
    }

}; // class Options


template <typename T>
void dump_dense_index(int fd) {
    typedef typename osmium::index::map::DenseFileArray<osmium::unsigned_object_id_type, T> dense_index_type;
    dense_index_type index(fd);

    for (size_t i = 0; i < index.size(); ++i) {
        if (index.get(i) != T{}) {
            std::cout << i << " " << index.get(i) << "\n";
        }
    }
}

template <typename T>
void dump_sparse_index(int fd) {
    typedef typename osmium::index::map::SparseFileArray<osmium::unsigned_object_id_type, T> sparse_index_type;
    sparse_index_type index(fd);

    for (auto& element : index) {
        if (element.first != 0) {
            std::cout << element.first << " " << element.second << "\n";
        }
    }
}

int dump_index(const std::string& database, const std::string& index_name) {
    bool dense = false;
    std::string filename{database + "/" + index_name + ".sparse.idx"};
    int fd = ::open(filename.c_str(), O_RDWR);

    if (fd == -1) {
        dense = true;
        filename = database + "/" + index_name + ".dense.idx";
        fd = ::open(filename.c_str(), O_RDWR);
    }

    if (fd == -1) {
        std::cerr << "Can't open " << index_name << " index file\n";
    }

    if (index_name == "locations") {
        if (dense) {
            dump_dense_index<osmium::Location>(fd);
        } else {
            dump_sparse_index<osmium::Location>(fd);
        }
    } else {
        if (dense) {
            dump_dense_index<size_t>(fd);
        } else {
            dump_sparse_index<size_t>(fd);
        }
    }

    return return_code::okay;
}

int dump_map(const std::string& database, const std::string& map_name) {
    const std::string filename{database + "/" + map_name + ".map"};
    const int fd = ::open(filename.c_str(), O_RDWR);

    if (fd == -1) {
        std::cerr << "Can't open " << map_name << " map file\n";
    }

    typedef typename osmium::index::multimap::SparseFileArray<osmium::unsigned_object_id_type, osmium::unsigned_object_id_type> map_type;
    map_type map{fd};

    for (auto& element : map) {
        if (element.first != 0) {
            std::cout << element.first << " " << element.second << "\n";
        }
    }

    return return_code::okay;
}

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);

    Options options;
    options.parse(argc, argv);

    if (options.do_index()) {
        return dump_index(options.database(), options.index());
    } else {
        return dump_map(options.database(), options.map());
    }

    return return_code::okay;
}

