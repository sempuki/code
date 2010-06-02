project (test)
cmake_minimum_required (VERSION 2.6)

include (../../sagase-cmake/Sagase.cmake)

include (FindQt4)
find_package (Qt4 4.6 COMPONENTS QtCore QtGui REQUIRED)

if (NOT QT4_FOUND)
    message (FATAL_ERROR "Qt4 not found")
endif (NOT QT4_FOUND)

include(${QT_USE_FILE})

#qt4_generate_moc (main.h main_moc.cpp)

add_executable (test main.cpp)
    
if (MSVC)
    target_link_libraries (test ${QT_LIBRARIES})# debug ${OGRE_DEBUG_LIBRARIES})
else ()
    target_link_libraries (test ${QT_LIBRARIES})
endif ()
