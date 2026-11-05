cmake_minimum_required(VERSION 3.16)
project(LuantiCreator VERSION 0.9 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Qt6 COMPONENTS Widgets REQUIRED)

# WebView (Qt WebEngine) pour la « Documentation de l'API » : mise en page, coloration du code, boutons « Copier ».
# Facultatif : sans Qt WebEngine (paquet qt6-webengine-dev), la documentation reste affichée dans l'ancienne fenêtre simple.
option(LC_WEBVIEW "Documentation de l'API dans un WebView (Qt WebEngine)" ON)
if(LC_WEBVIEW)
    find_package(Qt6 COMPONENTS WebEngineWidgets QUIET)
    if(NOT Qt6WebEngineWidgets_FOUND)
        message(WARNING "Qt6 WebEngine introuvable : documentation en mode simple (sudo apt install qt6-webengine-dev pour le WebView).")
    endif()
endif()

add_executable(LuantiCreator main.cpp editor.h mesh.h widgets.h generator.h preview.h server.h fsutil.h inflate.h zipread.h
    template.h uihooks.h plugins.h pluginpage.h corepack.h markup.h scriptrun.h lcsdk.h texturedetect.h texcompose.h formspec.h
    appcore.h codehl.h winframe.h modimport.h launcher.h apidoc.h apidocwindow.h apidocview.h splash.h uikeys.h)
target_link_libraries(LuantiCreator PRIVATE Qt6::Widgets)
if(LC_WEBVIEW AND Qt6WebEngineWidgets_FOUND)
    target_link_libraries(LuantiCreator PRIVATE Qt6::WebEngineWidgets)
    target_compile_definitions(LuantiCreator PRIVATE LC_HAVE_WEBENGINE)
    message(STATUS "Documentation de l'API : WebView (Qt WebEngine)")
endif()
target_compile_definitions(LuantiCreator PRIVATE LC_VERSION="${PROJECT_VERSION}")

# Plugins intégrés : tous les .zip du dossier plugins/ sont embarqués dans l'exécutable (ressources Qt)
# et installés au premier lancement (voir PluginHost::installBundled).
file(GLOB LC_BUNDLED_PLUGINS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/*.zip")
if(LC_BUNDLED_PLUGINS)
    qt_add_resources(LuantiCreator "lcplugins" PREFIX "/lcplugins" BASE "${CMAKE_CURRENT_SOURCE_DIR}/plugins"
        OPTIONS -no-compress FILES ${LC_BUNDLED_PLUGINS})
    foreach(z ${LC_BUNDLED_PLUGINS})
        get_filename_component(zn ${z} NAME)
        message(STATUS "Plugin intégré : ${zn}")
    endforeach()
else()
    message(STATUS "Aucun plugin intégré (déposez des .zip dans plugins/ puis relancez la compilation)")
endif()

# Un seul exécutable : libstdc++ et libgcc sont embarquées.
# Pour un binaire 100 % statique : Qt statique + -DLC_STATIC=ON (voir README.md).
target_link_options(LuantiCreator PRIVATE -static-libgcc -static-libstdc++)

option(LC_STATIC "Lier Qt statiquement (nécessite une Qt statique ; incompatible avec le WebView : -DLC_WEBVIEW=OFF)" OFF)
if(LC_STATIC)
    target_link_options(LuantiCreator PRIVATE -static)
endif()
