
# generated by Buildyard, do not edit. Sets FOUND_REQUIRED if all required
# dependencies are found. Used by Buildyard.cmake
set(FIND_REQUIRED_FAILED)
find_package(Boost 1.41.0 COMPONENTS system regex date_time serialization program_options thread QUIET)
if(NOT Boost_FOUND AND NOT BOOST_FOUND)
  set(FIND_REQUIRED_FAILED "${FIND_REQUIRED_FAILED} Boost")
endif()
find_package(Lunchbox 1.9 QUIET)
if(NOT Lunchbox_FOUND AND NOT LUNCHBOX_FOUND)
  set(FIND_REQUIRED_FAILED "${FIND_REQUIRED_FAILED} Lunchbox")
endif()
find_package(MPI  QUIET)
if(NOT MPI_FOUND AND NOT MPI_FOUND)
  set(FIND_REQUIRED_FAILED "${FIND_REQUIRED_FAILED} MPI")
endif()
if(FIND_REQUIRED_FAILED)
  set(FOUND_REQUIRED FALSE)
else()
  set(FOUND_REQUIRED TRUE)
endif()