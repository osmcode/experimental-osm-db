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
#include <sys/types.h>
#include <vector>

#ifdef _MSC_VER
# include <direct.h>
#endif

// boost
#include <boost/program_options.hpp>

// osmium
#include <osmium/io/any_input.hpp>

// eodb
#include "options.hpp"
#include "eodb.hpp"

class Options : public OptionsBase {

public:

    void parse(int argc, char* argv[]) {
        try {
            namespace po = boost::program_options;

            po::options_description cmdline("Allowed options");
            cmdline.add_options()
                ("help,h", "Print this help message")
                ("version", "Show version")
                ("output,o", po::value<std::string>(), "Output file")
                ("overwrite,O", "Overwrite existing output file")
                ("append,a", "Append to output file")
            ;

            po::options_description hidden("Hidden options");
            hidden.add_options()
                ("input-filenames", po::value<std::vector<std::string>>(), "Input files")
            ;

            po::options_description desc;
            desc.add(cmdline).add(hidden);

            po::positional_options_description positional;
            positional.add("input-filenames", -1);

            po::store(po::command_line_parser(argc, argv).options(desc).positional(positional).run(), vm);
            po::notify(vm);

            check_version_option("osm2osr");

            if (vm.count("help")) {
                std::cout << "Usage: osm2osr [OPTIONS] OSM-FILE...\n";
                std::cout << "Write OSM data to raw data file.\n\n";
                std::cout << cmdline << "\n";
                exit(return_code::okay);
            }

            if (vm.count("append") && vm.count("overwrite")) {
                std::cerr << "Can not use --append,-a and --overwrite,-O together.\n";
                exit(return_code::fatal);
            }

            if (!vm.count("output")) {
                std::cerr << "Missing --output option\n";
                exit(return_code::fatal);
            }

        } catch (boost::program_options::error& e) {
            std::cerr << "Error parsing command line: " << e.what() << std::endl;
            exit(return_code::fatal);
        }
    }

    bool append() const {
        return !! vm.count("append");
    }

    bool overwrite() const {
        return vm.count("overwrite") != 0;
    }

    const std::string output_filename() const {
        return vm["output"].as<std::string>();
    }

}; // class Options

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);

    Options options;
    options.parse(argc, argv);

    int open_flags = O_WRONLY | O_CREAT;
    if (options.overwrite()) {
        open_flags |= O_TRUNC;
    } else if (options.append()) {
        open_flags |= O_APPEND;
    } else {
        open_flags |= O_EXCL;
    }

    int data_fd = ::open(options.output_filename().c_str(), open_flags, 0666);
    if (data_fd < 0) {
        std::cerr << "Can't open data file '" << options.output_filename() << "': " << strerror(errno) << "\n";
        exit(return_code::fatal);
    }

    for (const auto& fn : options.input_filenames()) {
        osmium::io::Reader reader(fn);

        while (osmium::memory::Buffer buffer = reader.read()) {
            int len = ::write(data_fd, buffer.data(), buffer.committed());
            if (len == -1) {
                std::cerr << "Write error: " << strerror(errno) << "\n";
                exit(return_code::fatal);
            }
        }

        reader.close();
    }

    return return_code::okay;
}

