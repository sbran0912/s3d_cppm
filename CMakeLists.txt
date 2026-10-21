cmake_minimum_required(VERSION 3.28)
project(s3d_cppm VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_COMPILER clang++-22)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

# Modul-Quellen (einmal definiert, mehrfach verwendet)
set(CXXMODS
    lib/color.cppm
    lib/math_core.cppm
    lib/mesh.cppm
    lib/mesh_io.cppm
    lib/shader.cppm
    lib/bodies.cppm
    lib/draw.cppm
    lib/renderer.cppm
)
# App-Quellen: .cpp direkt in add_executable, .cppm als Module registrieren
function(add_s3d_app name)
    set(_sources)
    set(_module_sources)
    foreach(_src IN LISTS ARGN)
        if(_src MATCHES "\\.cppm$")
            list(APPEND _module_sources ${_src})
        else()
            list(APPEND _sources ${_src})
        endif()
    endforeach()
    add_executable(${name} ${_sources})
    target_sources(${name} PRIVATE FILE_SET CXX_MODULES FILES
        ${CXXMODS} ${_module_sources})
    target_link_libraries(${name} PRIVATE glfw OpenGL::GL)
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endfunction()

add_s3d_app(beispiel apps/beispiel/main.cpp)
add_s3d_app(boids apps/boids/main.cpp apps/boids/vehicle.cppm)
add_s3d_app(vehic apps/vehic/main.cpp apps/vehic/vehicle.cppm)
add_s3d_app(rocket apps/rocket/main.cpp apps/rocket/vehicle.cppm)
add_custom_command(TARGET rocket POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/rocket/rocket.obj
        $<TARGET_FILE_DIR:rocket>/rocket.obj
    COMMENT "Kopiere rocket.obj fuer die Laufzeit-Suche"
)
add_s3d_app(ray apps/ray/main.cpp)
add_s3d_app(instanzen apps/instanzen/main.cpp)
add_s3d_app(vorlage apps/vorlage/main.cpp)

add_executable(lib3d_test tests/lib3d_test.cpp)
target_sources(lib3d_test PRIVATE FILE_SET CXX_MODULES FILES ${CXXMODS})
target_link_libraries(lib3d_test PRIVATE glfw OpenGL::GL)
target_include_directories(lib3d_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})