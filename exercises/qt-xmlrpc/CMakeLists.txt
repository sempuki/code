project (test)
cmake_minimum_required (VERSION 2.6)

include (../../sagase-cmake/Sagase.cmake)

include (FindQt4)
find_package (Qt4 4.6 COMPONENTS QtCore QtNetwork REQUIRED)

if (NOT QT4_FOUND)
    message (FATAL_ERROR "Qt4 not found")
endif (NOT QT4_FOUND)

include(${QT_USE_FILE})

qt4_generate_moc (main.h moc_main.cpp)
qt4_generate_moc (xmlrpc.h moc_xmlrpc.cpp)
qt4_generate_moc (capabilities.h moc_capabilities.cpp)
qt4_generate_moc (llsession.h moc_llsession.cpp)

set (CPP_FILES main.cpp xmlrpc.cpp capabilities.cpp session.cpp llsession.cpp)
set (MOC_FILES moc_main.cpp moc_xmlrpc.cpp moc_capabilities.cpp moc_llsession.cpp)

add_executable (test ${CPP_FILES} ${MOC_FILES})
    
if (MSVC)
    target_link_libraries (test ${QT_LIBRARIES})# debug ${OGRE_DEBUG_LIBRARIES})
else ()
    target_link_libraries (test ${QT_LIBRARIES})
endif ()
