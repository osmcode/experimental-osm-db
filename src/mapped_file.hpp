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

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <system_error>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

class MappedFile {

    int m_fd = -1;
    std::size_t m_size = 0;
    void* m_ptr = nullptr;
    std::string m_filename;

public:

    MappedFile(const std::string& filename);

    void close();

    ~MappedFile();

    typedef unsigned char* ucp;
    unsigned char* data() const {
        return ucp(m_ptr);
    }

    std::size_t size() const {
        return m_size;
    }

}; // class MappedFile

