#ifndef ANY_INDEX_HPP
#define ANY_INDEX_HPP

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
#include <cstring>
#include <memory>

// osmium
#include <osmium/index/multimap/sparse_mem_array.hpp>
#include <osmium/index/multimap/sparse_mem_multimap.hpp>

template <typename TId, typename TValue>
class AnyMultimap {

public:

    typedef osmium::index::multimap::Multimap<TId, TValue> map_type;
    typedef TId key_type;
    typedef TValue value_type;

    AnyMultimap(const std::string& map_type) {
        if (map_type == "sparse") {
            m_map.reset(new osmium::index::multimap::SparseMemArray<TId, TValue>);
        } else if (map_type == "stl") {
            m_map.reset(new osmium::index::multimap::SparseMemMultimap<TId, TValue>);
        } else {
            throw std::runtime_error{"unknown map type"};
        }
    }

    map_type& operator()() {
        return *m_map;
    }

private:

    std::unique_ptr<map_type> m_map {nullptr};

}; // class AnyMultimap

#endif // ANY_INDEX_HPP
