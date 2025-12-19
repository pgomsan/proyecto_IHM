cmake_minimum_required(VERSION 3.19)
project(proyecto_IHM LANGUAGES CXX)

# Añadimos Svg y SvgWidgets
find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Widgets
    Sql
    Svg
    SvgWidgets
)

qt_standard_project_setup()

qt_add_executable(proyecto_IHM
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    logindialog.cpp
    logindialog.h
    logindialog.ui
    registerdialog.cpp
    registerdialog.h
    registerdialog.ui
    profiledialog.cpp
    profiledialog.h
    profiledialog.ui
    historydialog.cpp
    historydialog.h
    historydialog.ui
    helpdialog.cpp
    helpdialog.h
    questiondialog.cpp
    questiondialog.h
    questionbankdialog.cpp
    questionbankdialog.h
    useragent.cpp
    useragent.h
    navdb/navigation.cpp
    navdb/navigationdao.cpp
    tool.cpp
    tool.h
    dibujos.cpp
    dibujos.h
)

target_include_directories(proyecto_IHM
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/navdb/lib/include
)

qt_add_resources(proyecto_IHM_resources
    resources/resources.qrc
)

target_sources(proyecto_IHM
    PRIVATE
        ${proyecto_IHM_resources}
)

# Enlazamos también contra Svg y SvgWidgets
target_link_libraries(proyecto_IHM
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Sql
        Qt::Svg
        Qt::SvgWidgets
)

set(NAVDB_FILE "${CMAKE_CURRENT_SOURCE_DIR}/navdb/navdb.sqlite")

add_custom_command(TARGET proyecto_IHM POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${NAVDB_FILE}"
        "$<TARGET_FILE_DIR:proyecto_IHM>/navdb.sqlite"
    VERBATIM
)

include(GNUInstallDirs)

install(TARGETS proyecto_IHM
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES
    "${NAVDB_FILE}"
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
    TARGET proyecto_IHM
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
