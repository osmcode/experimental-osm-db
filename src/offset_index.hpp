#ifndef OFFSET_INDEX_HPP
#define OFFSET_INDEX_HPP

#include <osmium/osm/types.hpp>

// osmium indexes
#include <osmium/index/map/dense_file_array.hpp>
#include <osmium/index/map/dummy.hpp>
#include <osmium/index/map/sparse_file_array.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/index/map/sparse_mem_map.hpp>
#include <osmium/index/map/sparse_mem_table.hpp>
#include <osmium/index/map/sparse_mmap_array.hpp>

using offset_index_type = osmium::index::map::Map<osmium::unsigned_object_id_type, size_t>;

#ifdef OSMIUM_HAS_INDEX_MAP_DENSE_FILE_ARRAY
    REGISTER_MAP(osmium::unsigned_object_id_type, size_t, osmium::index::map::DenseFileArray, dense_file_array)
#endif

#ifdef OSMIUM_HAS_INDEX_MAP_SPARSE_FILE_ARRAY
    REGISTER_MAP(osmium::unsigned_object_id_type, size_t, osmium::index::map::SparseFileArray, sparse_file_array)
#endif

#ifdef OSMIUM_HAS_INDEX_MAP_SPARSE_MEM_ARRAY
    REGISTER_MAP(osmium::unsigned_object_id_type, size_t, osmium::index::map::SparseMemArray, sparse_mem_array)
#endif

#ifdef OSMIUM_HAS_INDEX_MAP_SPARSE_MEM_MAP
    REGISTER_MAP(osmium::unsigned_object_id_type, size_t, osmium::index::map::SparseMemMap, sparse_mem_map)
#endif

#ifdef OSMIUM_HAS_INDEX_MAP_SPARSE_MEM_TABLE
    REGISTER_MAP(osmium::unsigned_object_id_type, size_t, osmium::index::map::SparseMemTable, sparse_mem_table)
#endif

#ifdef OSMIUM_HAS_INDEX_MAP_SPARSE_MMAP_ARRAY
    REGISTER_MAP(osmium::unsigned_object_id_type, size_t, osmium::index::map::SparseMmapArray, sparse_mmap_array)
#endif

#endif // OFFSET_INDEX_HPP
