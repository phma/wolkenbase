find_path(LASzip_INCLUDE_DIR laszip_api.h PATH_SUFFIXES laszip)
find_library(LASzip_LIBRARY NAMES laszip_api)
if (LASzip_INCLUDE_DIR AND LASzip_LIBRARY)
  set(LASzip_FOUND TRUE)
endif ()

if (LASzip_FOUND)
  if (NOT LASzip_FIND_QUIETLY)
    message(STATUS "Found LASzip: ${LASzip_LIBRARY}")
  endif ()
else ()
  if (LASzip_FIND_REQUIRED)
    message(FATAL_ERROR "Could not find LASzip")
  endif ()
endif ()