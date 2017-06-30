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

#include <iostream>

#include "mapped_file.hpp"

MappedFile::MappedFile(const std::string& filename) :
    m_filename(filename) {

    m_fd = ::open(filename.c_str(), O_RDWR);
    if (m_fd == -1) {
        throw std::system_error{errno, std::system_category(),
            std::string{"Opening input file '"} + filename + "' failed"};
    }

    struct stat s;
    if (::fstat(m_fd, &s) < 0) {
        throw std::system_error{errno, std::system_category(),
            std::string{"Getting length of input file '"} + filename + "' failed"};
    }

    m_size = s.st_size;

    m_ptr = ::mmap(nullptr, m_size, PROT_READ, MAP_SHARED, m_fd, 0);
    if (m_ptr == MAP_FAILED) {
        throw std::system_error{errno, std::system_category(),
            std::string{"Mapping of input file '"} + filename + "' failed"};
    }
}

void MappedFile::close() {
    if (m_ptr && ::munmap(m_ptr, m_size) != 0) {
        throw std::system_error{errno, std::system_category(),
            std::string{"Closing of input file '"} + m_filename + "' failed"};
    }

    if (m_fd != -1 && ::close(m_fd) != 0) {
        throw std::system_error{errno, std::system_category(),
            std::string{"Closing of input file '"} + m_filename + "' failed"};
    }
}

MappedFile::~MappedFile() {
    try {
        close();
    } catch (const std::system_error&) {
        // ignore errors
    }
}

