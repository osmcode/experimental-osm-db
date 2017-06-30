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
#include <iostream>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#ifdef _MSC_VER
# include <direct.h>
#endif

// boost
#include <boost/program_options.hpp>

// osmium
#include <osmium/io/pbf_output.hpp>
#include <osmium/io/xml_output.hpp>

// eodb
#include "mapped_file.hpp"
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
                ("generator", po::value<std::string>()->default_value("eodb_raw2osm/" EODB_VERSION), "Generator setting for file header")
                ("output,o", po::value<std::string>()->default_value("-"), "Output file")
                ("overwrite,O", "Overwrite existing output file")
                ("output-format,f", po::value<std::string>()->default_value(""), "Format of output file")
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

            check_version_option("osr2osm");

            if (vm.count("help")) {
                std::cout << "Usage: osr2osm [OPTIONS] DATA-FILE...\n";
                std::cout << "Read OSM data from raw data files.\n\n";
                std::cout << cmdline << "\n";
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

    bool overwrite() const {
        return vm.count("overwrite") != 0;
    }

    std::string generator() const {
        return vm["generator"].as<std::string>();
    }

    const std::string output_filename() const {
        if (vm.count("output")) {
            return vm["output"].as<std::string>();
        } else {
            return "-";
        }
    }

    const std::string output_format() const {
        return vm["output-format"].as<std::string>();
    }

    const std::vector<std::string> input_filenames() const {
        std::vector<std::string> input_filenames;

        if (vm.count("input-filenames")) {
            input_filenames = vm["input-filenames"].as<std::vector<std::string>>();
        } else {
            input_filenames.push_back("-"); // default is stdin
        }

        return input_filenames;
    }

}; // class Options

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);

    Options options;
    options.parse(argc, argv);

    osmium::io::Header header;
    header.set("generator", options.generator());

    osmium::io::Writer writer{
        options.output_filename(),
        header,
        options.overwrite() ?
            osmium::io::overwrite::allow :
            osmium::io::overwrite::no
    };

    try {
        for (const auto& filename : options.input_filenames()) {
            MappedFile mf{filename};

            osmium::memory::Buffer buffer{mf.data(), mf.size()};
            writer(std::move(buffer));

            mf.close();
        }
    } catch (const std::system_error& e) {
        std::cerr << e.what() << '\n';
        return return_code::fatal;
    }

    writer.close();

    return return_code::okay;
}

