#ifndef EODB_HPP
#define EODB_HPP

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

enum return_code : int {
    okay      = 0,
    not_found = 1,
    error     = 2,
    fatal     = 3
};

#define DEFAULT_EODB_NAME "test.eodb"
#define DEFAULT_DATA_FILE "/data.osr"

#include <string>

inline std::string index_name(const std::string& database, const std::string& index, bool dense) {
    std::string name = database + "/" + index + ".";
    name += (dense ? "dense" : "sparse");
    name += ".idx";
    return name;
}

inline std::string map_name(const std::string& database, const std::string& map) {
    return database + "/" + map + ".map";
}


#endif // EODB_HPP
