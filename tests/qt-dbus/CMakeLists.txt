
project (dbus-test)
cmake_minimum_required (VERSION 2.6)

include (FindQt4)
find_package (Qt4 4.5 COMPONENTS QtCore QtDBus REQUIRED)

if (NOT QT4_FOUND)
    message (FATAL_ERROR "Qt4 not found")
endif (NOT QT4_FOUND)

include(${QT_USE_FILE})

# cmake_module_path (.)             # where cmake modules are located
# cmake_install_prefix (/usr)       # where make install points
# cmake_build_type ()               # debug, release, ...
# build_shared_libs ()              # switch between static/shared

# add_subdirectory (.)              # recurse into subdirs

include_directories (.)             # Add include directories to the build.
link_directories (.)                # Specify directories in which the linker will look for libraries.

add_definitions (-DQT_NO_KEYWORDS)          # Adds -D define flags to the compilation of source files.
# add_dependencies (main dep)       # Add a dependency between top-level targets.

qt4_generate_moc (entity.h entity_moc.cpp)
qt4_generate_moc (entity_adaptor.h entity_adaptor_moc.cpp)
qt4_generate_moc (service.h service_moc.cpp)
qt4_generate_moc (service_adaptor.h service_adaptor_moc.cpp)
qt4_generate_moc (main.h main_moc.cpp)

add_executable (dbus-test 
    main.cpp 
    main_moc.cpp
    entity_moc.cpp 
    entity_adaptor.cpp 
    entity_adaptor_moc.cpp 
    service_moc.cpp 
    service_adaptor.cpp
    service_adaptor_moc.cpp)
    
target_link_libraries (dbus-test ${QT_LIBRARIES})
