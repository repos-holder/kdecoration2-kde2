set(skeleton_decoration_SOURCES
    skeleton.cpp
)

add_library(skeleton_decoration MODULE ${skeleton_decoration_SOURCES})

target_link_libraries(skeleton_decoration
    Qt5::Core
    Qt5::Gui
    KF5::CoreAddons
    KDecoration2::KDecoration
)

install(TARGETS skeleton_decoration DESTINATION ${PLUGIN_INSTALL_DIR}/org.kde.kdecoration2)

