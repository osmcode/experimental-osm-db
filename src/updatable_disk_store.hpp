#ifndef UPDATABLE_DISK_STORE_HPP
#define UPDATABLE_DISK_STORE_HPP

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

#include <cstddef>

#include <osmium/handler.hpp>
#include <osmium/index/map.hpp>
#include <osmium/io/detail/read_write.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/memory/item_iterator.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/osm/types.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/visitor.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#if 0
namespace osmium {

    namespace handler {
#endif

        /**
         *
         * Note: This handler will only work if either all object IDs are
         *       positive or all object IDs are negative.
         */
        class UpdatableDiskStore : public osmium::handler::Handler {

            typedef osmium::index::map::Map<osmium::unsigned_object_id_type, size_t> offset_index_type;

            size_t m_offset = 0;
            int m_data_fd;

            offset_index_type& m_node_index;
            offset_index_type& m_way_index;
            offset_index_type& m_relation_index;

        public:

            explicit UpdatableDiskStore(int data_fd, offset_index_type& node_index, offset_index_type& way_index, offset_index_type& relation_index) :
                m_data_fd(data_fd),
                m_node_index(node_index),
                m_way_index(way_index),
                m_relation_index(relation_index) {
                struct stat s;
                int result = ::fstat(m_data_fd, &s);
                if (result != 0) {
                    throw std::system_error{errno, std::system_category(), "stat on db file failed"};
                }
                m_offset = s.st_size;
            }

            UpdatableDiskStore(const UpdatableDiskStore&) = delete;
            UpdatableDiskStore& operator=(const UpdatableDiskStore&) = delete;

            ~UpdatableDiskStore() noexcept = default;

            void node(const osmium::Node& node) {
                m_node_index.set(node.positive_id(), m_offset);
                m_offset += node.byte_size();
            }

            void way(const osmium::Way& way) {
                m_way_index.set(way.positive_id(), m_offset);
                m_offset += way.byte_size();
            }

            void relation(const osmium::Relation& relation) {
                m_relation_index.set(relation.positive_id(), m_offset);
                m_offset += relation.byte_size();
            }

            // XXX
            void operator()(const osmium::memory::Buffer& buffer) {
                osmium::io::detail::reliable_write(m_data_fd, buffer.data(), buffer.committed());

                osmium::apply(buffer.begin(), buffer.end(), *this);
            }

        }; // class DiskStore

#if 0
    } // namespace handler

} // namespace osmium
#endif

#endif // UPDATABLE_DISK_STORE_HPP
