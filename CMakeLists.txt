# Witchaven 360: jfwhaven (JonoF's Witchaven port) for the Xbox 360, built with xsdk and SDL-xenon (README.md).
#
#   cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=$HOME/xsdk/cmake/xsdk.cmake \
#     -DSDL3_DIR=$HOME/SDL-xenon/build-xsdk -DCMAKE_BUILD_TYPE=Release
#   cmake --build build
#
# That makes build/witchaven.xex. build.sh does both.
cmake_minimum_required(VERSION 3.25)
project(witchaven360 C)

# Keep the same as WITCHAVEN360_VERSION below and xsdk_add_xex's VERSION.
set(WITCHAVEN360_VERSION "0.3.1")

find_package(SDL3 CONFIG REQUIRED)

set(ENGINE jfbuild/src)
set(AUDIOLIB jfaudiolib/src)

# The Build engine, as jfbuild's Makefile builds it with no assembly, no OpenGL and no Polymost, with the null
# network code (no multiplayer) and the startup window's stubs; not defs.c, which only the editor uses. The platform
# layer is xenon/xenonlayer.c, in place of sdlayer2.c.
set(ENGINE_SOURCES
	${ENGINE}/a-c.c
	${ENGINE}/asmprot.c
	${ENGINE}/baselayer.c
	${ENGINE}/cache1d.c
	${ENGINE}/compat.c
	${ENGINE}/crc32.c
	${ENGINE}/engine.c
	${ENGINE}/kplib.c
	${ENGINE}/mmulti_null.c
	${ENGINE}/osd.c
	${ENGINE}/pragmas.c
	${ENGINE}/scriptfile.c
	${ENGINE}/textfont.c
	${ENGINE}/talltextfont.c
	${ENGINE}/smalltextfont.c
	${ENGINE}/startwin_stub.c
	${ENGINE}/version.c)

# jfaudiolib, its SDL driver being xenon/driver_sdl3.c (SDL3) in place of driver_sdl.c (SDL2). No Vorbis.
set(AUDIOLIB_SOURCES
	${AUDIOLIB}/drivers.c
	${AUDIOLIB}/fx_man.c
	${AUDIOLIB}/cd.c
	${AUDIOLIB}/multivoc.c
	${AUDIOLIB}/mix.c
	${AUDIOLIB}/mixst.c
	${AUDIOLIB}/pitch.c
	${AUDIOLIB}/vorbis.c
	${AUDIOLIB}/music.c
	${AUDIOLIB}/midi.c
	${AUDIOLIB}/driver_nosound.c
	${AUDIOLIB}/asssys.c)

# The game, as jfwhaven's Makefile builds it, less the startup windows and their data import (datascan.c).
set(GAME_SOURCES
	src/b5compat.c
	src/config.c
	src/whani.c
	src/whaven.c
	src/whfx.c
	src/whinp.c
	src/whmenu.c
	src/whnet.c
	src/whobj.c
	src/whplr.c
	src/whsmk.c
	src/whsndmod.c
	src/whtag.c
	libsmacker/smacker.c)

# The Xbox 360's side: the platform layer, the controller and its Settings pages, the keyboard for naming saves, the
# sound effects' and the music's drivers, the version, two POSIX calls xsdk lacks, the data files kept in memory, and
# the music's OPL3 chip (Nuked OPL3, from Heretic 360).
set(PORT_SOURCES
	xenon/xenonlayer.c
	xenon/wh_pad.c
	xenon/wh_menu360.c
	xenon/wh_xam.c
	xenon/wh_devmenu.c
	xenon/driver_sdl3.c
	xenon/driver_opl.c
	xenon/version.c
	xenon/posix_shims.c
	xenon/memfiles.c
	opl/opl3.c)

add_executable(witchaven ${PORT_SOURCES} ${ENGINE_SOURCES} ${AUDIOLIB_SOURCES} ${GAME_SOURCES})
set_target_properties(witchaven PROPERTIES C_STANDARD 99 C_EXTENSIONS ON)

target_include_directories(witchaven PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/xenon"
	"${CMAKE_CURRENT_SOURCE_DIR}/opl"
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
	"${CMAKE_CURRENT_SOURCE_DIR}/jfbuild/include"
	"${CMAKE_CURRENT_SOURCE_DIR}/jfbuild/src"
	"${CMAKE_CURRENT_SOURCE_DIR}/jfaudiolib/include"
	"${CMAKE_CURRENT_SOURCE_DIR}/jfaudiolib/src"
	"${CMAKE_CURRENT_SOURCE_DIR}/libsmacker")

target_compile_definitions(witchaven PRIVATE
	WITCHAVEN360=1
	WITCHAVEN360_VERSION="${WITCHAVEN360_VERSION}"
	# The engine's SDL build, as far as the game's code is concerned (no startup window: no GTK).
	RENDERTYPESDL=1
	# jfaudiolib's driver table includes the SDL driver (xenon/driver_sdl3.c).
	HAVE_SDL=3)

# -fsigned-char: Witchaven and Build were written for x86, where char is signed; on PowerPC it isn't, unless asked.
# The code builds with warnings on PowerPC; they're shown, not fatal.
target_compile_options(witchaven PRIVATE
	-fsigned-char
	-fno-strict-aliasing
	-Wall
	-Wno-unused-parameter
	-Wno-unused-variable
	-Wno-unused-function
	-Wno-unused-but-set-variable
	-Wno-pointer-sign
	-Wno-char-subscripts)

target_link_libraries(witchaven PRIVATE SDL3::SDL3-static)

# The engine's file calls go through xenon/memfiles.c, which serves the game's data from memory.
target_link_options(witchaven PRIVATE
	-Wl,--wrap=open,--wrap=read,--wrap=lseek,--wrap=close,--wrap=fstat)

# Writes witchaven_shaders.h, with blit_vert and blit_frag.
xsdk_add_shaders(witchaven xenon/blit.vert xenon/blit.frag)
# A title ID of its own ("WH36") and the name Aurora and the dashboard show.
xsdk_add_xex(witchaven TITLE_ID 0x57483336 NAME "Witchaven" VERSION 0.3.1.0)
