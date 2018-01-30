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
#include <algorithm>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// boost
#include <boost/program_options.hpp>

// osmium
#include <osmium/io/any_output.hpp>

// eodb
#include "mapped_file.hpp"
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
                ("generator", po::value<std::string>()->default_value("eodb_export/" EODB_VERSION), "Generator setting for file header")
                ("output,o", po::value<std::string>()->default_value("-"), "Output file")
                ("output-format,f", po::value<std::string>()->default_value(""), "Format of output file (empty: autodetect)")
                ("offset,O", po::value<size_t>()->default_value(0), "Start from offset")
                ("count,c", po::value<size_t>()->default_value(0), "Write count objects (all if count=0)")
            ;

            po::store(po::parse_command_line(argc, argv, desc), vm);
            po::notify(vm);

            check_version_option("eodb_export");

            if (vm.count("help")) {
                std::cout << "Usage: eodb_export [OPTIONS]\n";
                std::cout << "Export (part of a) database into OSM file.\n\n";
                std::cout << desc << "\n";
                std::exit(return_code::okay);
            }

            if (vm.count("output") == 0 && vm.count("output-format") == 0) {
                std::cerr << "You have to set the output file name with --output,-o or the output format with --output-format,-f\n";
                std::exit(return_code::fatal);
            }
        } catch (const boost::program_options::error& e) {
            std::cerr << "Error parsing command line: " << e.what() << '\n';
            std::exit(return_code::fatal);
        }
    }

    size_t offset() const {
        return vm["offset"].as<size_t>();
    }

    size_t count() const {
        return vm["count"].as<size_t>();
    }

    std::string output_file_name() const {
        return vm["output"].as<std::string>();
    }

    std::string output_format() const {
        return vm["output-format"].as<std::string>();
    }

    std::string generator() const {
        return vm["generator"].as<std::string>();
    }

}; // class Options

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);

    const size_t initial_extract_buffer_size = 10240;

    Options options;
    options.parse(argc, argv);

    try {
        MappedFile mf{options.data_file_name()};
        osmium::memory::Buffer buffer{mf.data(), mf.size()};

        osmium::io::File file{options.output_file_name(), options.output_format()};
        osmium::io::Header header;
        header.set("generator", options.generator());
        osmium::io::Writer writer{file, header};

        if (options.count() == 0) {
            writer(std::move(buffer));
        } else {
            osmium::memory::Buffer extract{initial_extract_buffer_size};
            std::copy_n(buffer.get_iterator(options.offset()), options.count(), std::back_inserter(extract));
            writer(std::move(extract));
        }

        writer.close();
        mf.close();
    } catch (const std::system_error& e) {
        std::cerr << e.what() << '\n';
        return return_code::fatal;
    }

    return return_code::okay;
}

