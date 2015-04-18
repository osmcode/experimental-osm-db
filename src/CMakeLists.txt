#----------------------------------------------------------------------
#
#  CMake Config
#
#  EODB - Experimental OSM Database
#
#----------------------------------------------------------------------

add_executable(eodb_create eodb.hpp eodb_create.cpp any_index.hpp)
add_executable(eodb_dump   eodb.hpp eodb_dump.cpp any_index.hpp)
add_executable(eodb_export eodb.hpp eodb_export.cpp mapped_file.cpp)
add_executable(eodb_lookup eodb.hpp eodb_lookup.cpp)
add_executable(eodb_update eodb.hpp eodb_update.cpp any_index.hpp)
add_executable(osm2osr      eodb.hpp osm2osr.cpp)
add_executable(osr2osm      eodb.hpp osr2osm.cpp mapped_file.cpp)

foreach(_prog eodb_create eodb_dump eodb_export eodb_lookup eodb_update osm2osr osr2osm)
    target_link_libraries(${_prog} ${Boost_LIBRARIES} ${OSMIUM_LIBRARIES})
    install(TARGETS ${_prog} DESTINATION bin)
endforeach()

add_executable(eodb_create_leveldb eodb.hpp eodb_create_leveldb.cpp)
add_executable(eodb_dump_leveldb   eodb.hpp eodb_dump_leveldb.cpp)

foreach(_prog eodb_create_leveldb eodb_dump_leveldb)
    target_link_libraries(${_prog} ${Boost_LIBRARIES} ${OSMIUM_LIBRARIES} leveldb)
    install(TARGETS ${_prog} DESTINATION bin)
endforeach()

#----------------------------------------------------------------------
