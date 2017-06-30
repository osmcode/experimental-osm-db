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
#include <cassert>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>

#include <leveldb/db.h>

// boost
#include <boost/program_options.hpp>

// osmium
#include <osmium/osm/types.hpp>

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
                ("map,m", po::value<std::string>(), "Name of map")
            ;

            po::store(po::parse_command_line(argc, argv, desc), vm);
            po::notify(vm);

            check_version_option("eodb_dump_leveldb");

            if (vm.count("help")) {
                std::cout << "Usage: eodb_dump_leveldb [OPTIONS]\n";
                std::cout << "Dump index/map data from database.\n\n";
                std::cout << desc << "\n";
                std::cout << "Maps: n(ode)2w(ay), n(ode)2r(elation), w(ay)2r(elation), r(elation)2r(elation)\n";
                std::exit(return_code::okay);
            }

            if (vm.count("map")) {
                if (map() != "node2way" && map() != "node2relation" && map() != "way2relation" && map() != "relation2relation") {
                    std::cerr << "Map given with --map,-m must be one of: node2way, node2relation, way2relation, relation2relation\n";
                    std::exit(return_code::fatal);
                }
            } else {
                std::cout << "Need --map, -m option\n";
                std::exit(return_code::okay);
            }

        } catch (const boost::program_options::error& e) {
            std::cerr << "Error parsing command line: " << e.what() << '\n';
            std::exit(return_code::fatal);
        }
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


int dump_map(const std::string& database, const std::string& map) {
    leveldb::DB* db;
    leveldb::Options leveldb_options;
    leveldb::Status status = leveldb::DB::Open(leveldb_options, database + "/" + map + ".leveldb", &db);
    assert(status.ok());

    leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        const leveldb::Slice& k = it->key();
        const leveldb::Slice& v = it->value();
        auto ki = reinterpret_cast<const osmium::unsigned_object_id_type*>(k.data());
        auto vi = reinterpret_cast<const osmium::unsigned_object_id_type*>(v.data());

        std::cout << *ki << " " << *vi << "\n";
    }
    assert(it->status().ok());  // Check for any errors found during the scan
    delete it;

    return return_code::okay;
}

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);

    Options options;
    options.parse(argc, argv);

    return dump_map(options.database(), options.map());
}

