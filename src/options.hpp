#ifndef OPTIONS_HPP
#define OPTIONS_HPP

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

#include "eodb.hpp"

// boost
#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <vector>

namespace po = boost::program_options;

class OptionsBase {

protected:

    po::variables_map vm;

public:

    OptionsBase() = default;
    ~OptionsBase() = default;

    void check_version_option(const char* program_name) const {
        if (vm.count("version")) {
            std::cout << program_name << " version " EODB_VERSION "\n"
                      << "Copyright (C) 2015  Jochen Topf <jochen@topf.org>\n"
                      << "License: GNU GENERAL PUBLIC LICENSE Version 3 <http://gnu.org/licenses/gpl.html>.\n"
                      << "This is free software: you are free to change and redistribute it.\n"
                      << "There is NO WARRANTY, to the extent permitted by law.\n";

            std::exit(return_code::okay);
        }
    }

    std::string database() const {
        return vm["database"].as<std::string>();
    }

    std::string data_file_name() const {
        return database().append(DEFAULT_DATA_FILE);
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

}; // class OptionsBase


#endif // OPTIONS_HPP
