set(PQ_COM_FORMAT_SOURCES
    src/pq_com_format.c
)

set(PQ_COM_FORMAT_HEADERS
    include/pq_com_format/pq_com_format.h
    include/pq_com_format/types.h
    include/pq_com_format/config.h
)

set(PQ_COM_FORMAT_PRIVATE_HEADERS
    src/internal.h
)

add_library(pq_com_format
    ${PQ_COM_FORMAT_SOURCES}
    ${PQ_COM_FORMAT_HEADERS}
    ${PQ_COM_FORMAT_PRIVATE_HEADERS}
)

target_include_directories(pq_com_format
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(pq_com_format PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${PQ_COM_FORMAT_HEADERS}"
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)


target_compile_features(pq_com_format PRIVATE c_std_99)

if(BUILD_SHARED_LIBS)
    target_compile_definitions(pq_com_format PRIVATE PQ_COM_FORMAT_BUILDING_DLL)
    target_compile_definitions(pq_com_format PUBLIC PQ_COM_FORMAT_DLL)
endif()

install(TARGETS pq_com_format
    EXPORT pq_com_format_targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/pq_com_format
)

install(EXPORT pq_com_format_targets
    FILE pq_com_format_targets.cmake
    NAMESPACE pq_com_format::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pq_com_format
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/pq_com_format_config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/pq_com_format_config.cmake
    @ONLY
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/pq_com_format_config.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pq_com_format
)
