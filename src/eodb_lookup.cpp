/*

EODB -- An experimental OSM database based on Libosmium.

Copyright (C) 2015-2018  Jochen Topf <jochen@topf.org>

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
#include <osmium/osm/location.hpp>
#include <osmium/osm/types.hpp>

// eodb
#include "options.hpp"
#include "eodb.hpp"

class Options : public OptionsBase {

public:

    void parse(int argc, char* argv[]) {
        try {
            namespace po = boost::program_options;

            po::options_description cmdline{"Allowed options"};
            cmdline.add_options()
                ("help,h", "Print this help message")
                ("version", "Show version")
                ("database,d", po::value<std::string>()->default_value(DEFAULT_EODB_NAME), "Database directory")
                ("index,i", po::value<std::string>(), "Name of index")
                ("map,m", po::value<std::string>(), "Name of map")
            ;

            po::options_description hidden{"Hidden options"};
            hidden.add_options()
                ("ids", po::value<std::vector<osmium::unsigned_object_id_type>>(), "IDs to lookup")
            ;

            po::options_description desc;
            desc.add(cmdline).add(hidden);

            po::positional_options_description positional;
            positional.add("ids", -1);

            po::store(po::command_line_parser(argc, argv).options(desc).positional(positional).run(), vm);
            po::notify(vm);

            check_version_option("eodb_lookup");

            if (vm.count("help")) {
                std::cout << "Usage: eodb_lookup [OPTIONS] ID...\n";
                std::cout << "Look up data in the database indexes or maps.\n\n";
                std::cout << desc << "\n";
                std::cout << "Indexes: n(odes), w(ays), r(elations), l(ocations)\n";
                std::cout << "Maps: node2way, node2relation, way2relation, relation2relation\n";
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

            if (vm.count("ids") == 0) {
                std::cerr << "Need at least one Id to search for on command line\n";
                std::exit(return_code::fatal);
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

    std::vector<osmium::unsigned_object_id_type> search_ids() const {
        return vm["ids"].as<std::vector<osmium::unsigned_object_id_type>>();
    }

}; // class Options

template <class TIndex>
bool lookup_id_in_index_dense(const TIndex& index, const osmium::unsigned_object_id_type id) {
    try {
        auto value = index.get(id);
        std::cout << id << " " << value << '\n';
    } catch (...) {
        std::cout << id << " not found\n";
        return false;
    }

    return true;
}

template <class TIndex>
bool lookup_id_in_index_sparse(const TIndex& index, const osmium::unsigned_object_id_type id) {
    typedef typename TIndex::element_type element_type;

    element_type elem {id, typename TIndex::value_type()};
    auto positions = std::equal_range(index.begin(), index.end(), elem, [](const element_type& lhs, const element_type& rhs) {
        return lhs.first < rhs.first;
    });

    if (positions.first == positions.second) {
        std::cout << id << " not found\n";
        return false;
    }

    for (auto& it = positions.first; it != positions.second; ++it) {
        std::cout << it->first << " " << it->second << "\n";
    }

    return true;
}

template <class T>
bool lookup_index_dense(int fd, const std::vector<osmium::unsigned_object_id_type>& ids) {
    typedef typename osmium::index::map::DenseFileArray<osmium::unsigned_object_id_type, T> dense_index_type;
    dense_index_type index{fd};

    bool found_all = true;
    for (const auto id : ids) {
        if (!lookup_id_in_index_dense<dense_index_type>(index, id)) {
            found_all = false;
        }
    }

    return found_all;
}

template <class T>
bool lookup_index_sparse(int fd, const std::vector<osmium::unsigned_object_id_type>& ids) {
    typedef typename osmium::index::map::SparseFileArray<osmium::unsigned_object_id_type, T> sparse_index_type;
    sparse_index_type index{fd};

    bool found_all = true;
    for (const auto id : ids) {
        if (!lookup_id_in_index_sparse<sparse_index_type>(index, id)) {
            found_all = false;
        }
    }

    return found_all;
}
bool lookup_index(const std::string& database, const std::string& index_name, const std::vector<osmium::unsigned_object_id_type>& ids) {
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
            return lookup_index_dense<osmium::Location>(fd, ids);
        } else {
            return lookup_index_sparse<osmium::Location>(fd, ids);
        }
    } else {
        if (dense) {
            return lookup_index_dense<size_t>(fd, ids);
        } else {
            return lookup_index_sparse<size_t>(fd, ids);
        }
    }

    return return_code::okay;
}

bool lookup_map(const std::string& database, const std::string& map_name, const std::vector<osmium::unsigned_object_id_type>& ids) {
    std::string filename{database + "/" + map_name + ".map"};
    const int fd = ::open(filename.c_str(), O_RDWR);

    if (fd == -1) {
        std::cerr << "Can't open " << map_name << " map file\n";
    }

    bool found_all = true;

    typedef typename osmium::index::multimap::SparseFileArray<osmium::unsigned_object_id_type, osmium::unsigned_object_id_type> sparse_map_type;
    sparse_map_type map(fd);

    for (const auto id : ids) {
        if (!lookup_id_in_index_sparse<sparse_map_type>(map, id)) {
            found_all = false;
        }
    }

    return found_all;
}

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);

    Options options;
    options.parse(argc, argv);

    bool found_all = false;
    if (options.do_index()) {
        found_all = lookup_index(options.database(), options.index(), options.search_ids());
    } else {
        found_all = lookup_map(options.database(), options.map(), options.search_ids());
    }

    return found_all ? return_code::okay : return_code::not_found;
}

