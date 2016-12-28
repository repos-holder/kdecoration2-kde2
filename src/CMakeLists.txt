set(skeleton_decoration_SOURCES
    skeleton.cpp
)

add_library(kde2_decoration MODULE ${skeleton_decoration_SOURCES})

target_link_libraries(kde2_decoration
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    KF5::CoreAddons
    KDecoration2::KDecoration
)

install(TARGETS kde2_decoration DESTINATION ${PLUGIN_INSTALL_DIR}/org.kde.kdecoration2)

