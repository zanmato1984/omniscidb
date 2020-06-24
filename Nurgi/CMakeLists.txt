set(NURGI_SOURCE_FILES Nurgi.cpp Catalog.cpp)
    set(NURGI_LIBRARIES Parser Analyzer QueryEngine Shared)

        add_library(Nurgi ${NURGI_SOURCE_FILES})
            target_link_libraries(Nurgi ${QR_LIBRARIES})
