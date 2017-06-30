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

#include <leveldb/db.h>
#include "leveldb/write_batch.h"

// boost
#include <boost/program_options.hpp>

// osmium
#include <osmium/io/any_input.hpp>
#include <osmium/handler/disk_store.hpp>
#include <osmium/handler/object_relations.hpp>

// osmium indexes
#include <osmium/index/map/dense_file_array.hpp>
#include <osmium/index/map/dense_mem_array.hpp>
#include <osmium/index/map/dense_mmap_array.hpp>
#include <osmium/index/map/dummy.hpp>
#include <osmium/index/map/sparse_file_array.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/index/map/sparse_mem_map.hpp>
#include <osmium/index/map/sparse_mem_table.hpp>
#include <osmium/index/map/sparse_mmap_array.hpp>

#include <osmium/index/node_locations_map.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>

// eodb
#include "eodb.hpp"
#include "options.hpp"

typedef osmium::index::map::Map<osmium::unsigned_object_id_type, size_t> offset_index_type;
typedef osmium::index::map::Map<osmium::unsigned_object_id_type, osmium::Location> location_index_type;

class Options : public OptionsBase {

    std::string m_index_type{"sparse_mem_array"};
    bool m_use_dense_index{false};

public:

    void parse(int argc, char* argv[]) {
        std::set<std::string> index_types = {
            "dense_file_array",
            "dense_mem_array",
            "dense_mmap_array",
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
                ("index,i", po::value<std::string>(), "Use this node/way/relation index type")
                ("location,l", po::value<std::string>(), "Use this location index type (default: no location index)")
                ("maps,m", "Create maps")
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

            check_version_option("eodb_create_leveldb");

            if (vm.count("help")) {
                std::cout << "Usage: eodb_create_leveldb [OPTIONS] OSM-FILE...\n";
                std::cout << "Create new database and import OSM files into it.\n\n";
                std::cout << cmdline << "\n";
                std::cout << "Index types:\n";
                for (const auto& index_type : index_types) {
                    std::cout << "  " << index_type << "\n";
                }
                std::exit(return_code::okay);
            }

            if (vm.count("index")) {
                m_index_type = vm["index"].as<std::string>();
                if (index_types.count(m_index_type) == 0) {
                    std::cerr << "Unknown index type: '" << m_index_type << "'\n";
                    std::exit(return_code::fatal);
                }

                if (m_index_type.substr(0, 5) == "dense") {
                    m_use_dense_index = true;
                }
            }
        } catch (const boost::program_options::error& e) {
            std::cerr << "Error parsing command line: " << e.what() << '\n';
            std::exit(return_code::fatal);
        }
    }

    const std::string& index_type() const {
        return m_index_type;
    }

    bool file_based_index() const {
        return m_index_type == "dense_file_array" || m_index_type == "sparse_file_array";
    }

    std::string location_index_type() const {
        if (vm.count("location") == 0) {
            return "";
        }
        return vm["location"].as<std::string>();
    }

    bool use_dense_index() const {
        return m_use_dense_index;
    }

    bool create_maps() const {
        return vm.count("maps") > 0;
    }

}; // class Options

template <class TIndex>
void write_index_file(const std::string& database, const std::string& name, TIndex& index, bool dense) {
    const std::string index_file{index_name(database, name, dense)};
    const int fd = ::open(index_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        std::cerr << "Can't open index file '" << index_file << "': " << std::strerror(errno) << "\n";
        std::exit(return_code::fatal);
    }

    if (dense) {
        index.dump_as_array(fd);
    } else {
        index.dump_as_list(fd);
    }

    close(fd);
}

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);

    Options options;
    options.parse(argc, argv);

#ifndef _WIN32
    int result = ::mkdir(options.database().c_str(), 0777);
#else
    int result = mkdir(options.database().c_str());
#endif
    if (result == -1) {
        std::cerr << "Problem creating database directory '" << options.database() << "': " << std::strerror(errno) << "\n";
        std::exit(return_code::fatal);
    }

    int data_fd = ::open(options.data_file_name().c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (data_fd < 0) {
        std::cerr << "Can't open data file '" << options.data_file_name() << "': " << std::strerror(errno) << "\n";
        std::exit(return_code::fatal);
    }

    const auto& map_factory = osmium::index::MapFactory<osmium::unsigned_object_id_type, size_t>::instance();
    std::string index_type_nodes = options.index_type();
    std::string index_type_ways = options.index_type();
    std::string index_type_relations = options.index_type();

    if (options.file_based_index()) {
        index_type_nodes += "," + index_name(options.database(), "nodes", options.use_dense_index());
        index_type_ways += "," + index_name(options.database(), "ways", options.use_dense_index());
        index_type_relations += "," + index_name(options.database(), "relations", options.use_dense_index());
    }

    std::unique_ptr<offset_index_type> node_index     = map_factory.create_map(index_type_nodes);
    std::unique_ptr<offset_index_type> way_index      = map_factory.create_map(index_type_ways);
    std::unique_ptr<offset_index_type> relation_index = map_factory.create_map(index_type_relations);

    const auto& location_index_factory = osmium::index::MapFactory<osmium::unsigned_object_id_type, osmium::Location>::instance();
    std::unique_ptr<location_index_type> location_index;

    typedef osmium::handler::NodeLocationsForWays<location_index_type> location_handler_type;
    std::unique_ptr<location_handler_type> location_handler;

    if (options.location_index_type() != "") {
        location_index = location_index_factory.create_map(options.location_index_type());
        location_handler.reset(new location_handler_type(*location_index));
    }

    osmium::handler::DiskStore disk_store_handler{data_fd, *node_index, *way_index, *relation_index};

    leveldb::DB* db_n2w;
    leveldb::DB* db_n2r;
    leveldb::DB* db_w2r;
    leveldb::DB* db_r2r;

    leveldb::Options leveldb_options;
    leveldb_options.create_if_missing = true;
    leveldb_options.write_buffer_size = 256 * 1024;
    leveldb_options.block_size = 256 * 1024;

    leveldb::Status status = leveldb::DB::Open(leveldb_options, options.database() + "/" + "n2w.leveldb", &db_n2w);
    assert(status.ok());

    status = leveldb::DB::Open(leveldb_options, options.database() + "/" + "n2r.leveldb", &db_n2r);
    assert(status.ok());

    status = leveldb::DB::Open(leveldb_options, options.database() + "/" + "w2r.leveldb", &db_w2r);
    assert(status.ok());

    status = leveldb::DB::Open(leveldb_options, options.database() + "/" + "r2r.leveldb", &db_r2r);
    assert(status.ok());

    for (const auto& fn : options.input_filenames()) {
        osmium::io::Reader reader{fn};

        while (osmium::memory::Buffer buffer = reader.read()) {
            disk_store_handler(buffer);

            if (buffer.cbegin<osmium::Way>() != buffer.cend<osmium::Way>()) {
                leveldb::WriteBatch batch;

                for (auto it = buffer.cbegin<osmium::Way>(); it != buffer.cend<osmium::Way>(); ++it) {
                    for (const auto& node_ref : it->nodes()) {
                        osmium::unsigned_object_id_type k = node_ref.positive_ref();
                        osmium::unsigned_object_id_type v = it->positive_id();
                        char key_buffer[sizeof(k) + sizeof(v)] = {};
                        memcpy(key_buffer, &k, sizeof(k));
                        memcpy(key_buffer + sizeof(k), &v, sizeof(v));
                        leveldb::Slice key { key_buffer, sizeof(key_buffer) };
                        leveldb::Slice val { reinterpret_cast<const char*>(&v), sizeof(osmium::unsigned_object_id_type) };
                        batch.Put(key, val);
                    }
                }

                status = db_n2w->Write(leveldb::WriteOptions(), &batch);
                assert(status.ok());
            }

            if (buffer.cbegin<osmium::Relation>() != buffer.cend<osmium::Relation>()) {
                leveldb::WriteBatch batch_n;
                leveldb::WriteBatch batch_w;
                leveldb::WriteBatch batch_r;

                for (auto it = buffer.cbegin<osmium::Relation>(); it != buffer.cend<osmium::Relation>(); ++it) {
                    for (const auto& member : it->members()) {
                        osmium::unsigned_object_id_type k = member.positive_ref();
                        osmium::unsigned_object_id_type v = it->positive_id();
                        char key_buffer[sizeof(k) + sizeof(v)] = {};
                        memcpy(key_buffer, &k, sizeof(k));
                        memcpy(key_buffer + sizeof(k), &v, sizeof(v));
                        leveldb::Slice key { key_buffer, sizeof(key_buffer) };
                        leveldb::Slice val { reinterpret_cast<const char*>(&v), sizeof(osmium::unsigned_object_id_type) };
                        switch (member.type()) {
                            case osmium::item_type::node:
                                batch_n.Put(key, val);
                                break;
                            case osmium::item_type::way:
                                batch_w.Put(key, val);
                                break;
                            case osmium::item_type::relation:
                                batch_r.Put(key, val);
                                break;
                            default:
                                break;
                        }
                    }
                }

                status = db_n2r->Write(leveldb::WriteOptions(), &batch_n);
                assert(status.ok());

                status = db_w2r->Write(leveldb::WriteOptions(), &batch_w);
                assert(status.ok());

                status = db_r2r->Write(leveldb::WriteOptions(), &batch_r);
                assert(status.ok());
            }

            if (location_index) {
                osmium::apply(buffer, *location_handler);
            }
        }

        reader.close();
    }

    delete db_r2r;
    delete db_w2r;
    delete db_n2r;
    delete db_n2w;

    return return_code::okay;
}

