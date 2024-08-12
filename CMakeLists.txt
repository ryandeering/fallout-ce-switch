cmake_minimum_required(VERSION 3.13)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

set(EXECUTABLE_NAME fallout-ce)

if(NOT DEFINED ENV{F1_VERSION})
  set(F1_VERSION "0.0.0")
else()
  set(F1_VERSION $ENV{F1_VERSION})
endif()

project(${EXECUTABLE_NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

set(SDL2_INCLUDE_DIR "/opt/devkitpro/portlibs/switch/include/SDL2")
set(SDL2_LIBRARY "/opt/devkitpro/portlibs/switch/lib/libSDL2.a")
set(SDL2_TTF_LIBRARY "/opt/devkitpro/portlibs/switch/lib/libSDL2_ttf.a")
set(SDL2_MIXER_LIBRARY "/opt/devkitpro/portlibs/switch/lib/libSDL2_mixer.a")
set(SDL2_IMAGE_LIBRARY "/opt/devkitpro/portlibs/switch/lib/libSDL2_image.a")

set(ZLIB_INCLUDE_DIR "/opt/devkitpro/portlibs/switch/include")
set(ZLIB_LIBRARY "/opt/devkitpro/portlibs/switch/lib/libz.a")

set(EGL_INCLUDE_DIR "/opt/devkitpro/portlibs/switch/include/EGL")
set(EGL_LIBRARY "/opt/devkitpro/portlibs/switch/lib/libEGL.a")

set(NOUVEAU_INCLUDE_DIR "/opt/devkitpro/portlibs/switch/include/nouveau")
set(NOUVEAU_LIBRARY "/opt/devkitpro/portlibs/switch/lib/libdrm_nouveau.a")

set(GLAPI_LIBRARY "/opt/devkitpro/portlibs/switch/lib/libglapi.a")
set(GLES_LIBRARY "/opt/devkitpro/portlibs/switch/lib/libGLESv2.a")

set(NX_LIBS "/opt/devkitpro/libnx/lib/libnx.a")

set(FREETYPE_LIBRARY "/opt/devkitpro/portlibs/switch/lib/libfreetype.a")
set(HARFBUZZ_LIBRARY "/opt/devkitpro/portlibs/switch/lib/libharfbuzz.a")

include_directories(${SDL2_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ${EGL_INCLUDE_DIR} ${NOUVEAU_INCLUDE_DIR} /opt/devkitpro/libnx/include)
link_directories(/opt/devkitpro/portlibs/switch/lib /opt/devkitpro/libnx/lib)

include_directories(${CMAKE_SOURCE_DIR}/third_party/fpattern)

add_definitions(-D__SWITCH__)

if(NX)
    add_executable(${EXECUTABLE_NAME}
    "src/game/actions.cc"
    "src/game/actions.h"
    "src/game/amutex.cc"
    "src/game/amutex.h"
    "src/game/anim.cc"
    "src/game/anim.h"
    "src/game/art.cc"
    "src/game/art.h"
    "src/game/automap.cc"
    "src/game/automap.h"
    "src/game/bmpdlog.cc"
    "src/game/bmpdlog.h"
    "src/game/cache.cc"
    "src/game/cache.h"
    "src/game/combat_defs.h"
    "src/game/combat.cc"
    "src/game/combat.h"
    "src/game/combatai.cc"
    "src/game/combatai.h"
    "src/game/config.cc"
    "src/game/config.h"
    "src/game/credits.cc"
    "src/game/credits.h"
    "src/game/critter.cc"
    "src/game/critter.h"
    "src/game/cycle.cc"
    "src/game/cycle.h"
    "src/game/display.cc"
    "src/game/display.h"
    "src/game/editor.cc"
    "src/game/editor.h"
    "src/game/elevator.cc"
    "src/game/elevator.h"
    "src/game/endgame.cc"
    "src/game/endgame.h"
    "src/game/fontmgr.cc"
    "src/game/fontmgr.h"
    "src/game/game_vars.h"
    "src/game/game.cc"
    "src/game/game.h"
    "src/game/gconfig.cc"
    "src/game/gconfig.h"
    "src/game/gdebug.cc"
    "src/game/gdebug.h"
    "src/game/gdialog.cc"
    "src/game/gdialog.h"
    "src/game/gmemory.cc"
    "src/game/gmemory.h"
    "src/game/gmouse.cc"
    "src/game/gmouse.h"
    "src/game/gmovie.cc"
    "src/game/gmovie.h"
    "src/game/graphlib.cc"
    "src/game/graphlib.h"
    "src/game/gsound.cc"
    "src/game/gsound.h"
    "src/game/heap.cc"
    "src/game/heap.h"
    "src/game/intface.cc"
    "src/game/intface.h"
    "src/game/inventry.cc"
    "src/game/inventry.h"
    "src/game/item.cc"
    "src/game/item.h"
    "src/game/light.cc"
    "src/game/light.h"
    "src/game/lip_sync.cc"
    "src/game/lip_sync.h"
    "src/game/loadsave.cc"
    "src/game/loadsave.h"
    "src/game/main.cc"
    "src/game/main.h"
    "src/game/mainmenu.cc"
    "src/game/mainmenu.h"
    "src/game/map_defs.h"
    "src/game/map.cc"
    "src/game/map.h"
    "src/game/message.cc"
    "src/game/message.h"
    "src/game/moviefx.cc"
    "src/game/moviefx.h"
    "src/game/object_types.h"
    "src/game/object.cc"
    "src/game/object.h"
    "src/game/options.cc"
    "src/game/options.h"
    "src/game/palette.cc"
    "src/game/palette.h"
    "src/game/party.cc"
    "src/game/party.h"
    "src/game/perk_defs.h"
    "src/game/perk.cc"
    "src/game/perk.h"
    "src/game/pipboy.cc"
    "src/game/pipboy.h"
    "src/game/protinst.cc"
    "src/game/protinst.h"
    "src/game/proto_types.h"
    "src/game/proto.cc"
    "src/game/proto.h"
    "src/game/queue.cc"
    "src/game/queue.h"
    "src/game/reaction.cc"
    "src/game/reaction.h"
    "src/game/roll.cc"
    "src/game/roll.h"
    "src/game/scripts.cc"
    "src/game/scripts.h"
    "src/game/select.cc"
    "src/game/select.h"
    "src/game/selfrun.cc"
    "src/game/selfrun.h"
    "src/game/sfxcache.cc"
    "src/game/sfxcache.h"
    "src/game/sfxlist.cc"
    "src/game/sfxlist.h"
    "src/game/skill_defs.h"
    "src/game/skill.cc"
    "src/game/skill.h"
    "src/game/skilldex.cc"
    "src/game/skilldex.h"
    "src/game/stat_defs.h"
    "src/game/stat.cc"
    "src/game/stat.h"
    "src/game/textobj.cc"
    "src/game/textobj.h"
    "src/game/tile.cc"
    "src/game/tile.h"
    "src/game/trait.cc"
    "src/game/trait.h"
    "src/game/version.cc"
    "src/game/version.h"
    "src/game/wordwrap.cc"
    "src/game/wordwrap.h"
    "src/game/worldmap_walkmask.cc"
    "src/game/worldmap_walkmask.h"
    "src/game/worldmap.cc"
    "src/game/worldmap.h"
    "src/int/audio.cc"
    "src/int/audio.h"
    "src/int/audiof.cc"
    "src/int/audiof.h"
    "src/int/datafile.cc"
    "src/int/datafile.h"
    "src/int/dialog.cc"
    "src/int/dialog.h"
    "src/int/export.cc"
    "src/int/export.h"
    "src/int/intlib.cc"
    "src/int/intlib.h"
    "src/int/intrpret.cc"
    "src/int/intrpret.h"
    "src/int/memdbg.cc"
    "src/int/memdbg.h"
    "src/int/mousemgr.cc"
    "src/int/mousemgr.h"
    "src/int/movie.cc"
    "src/int/movie.h"
    "src/int/nevs.cc"
    "src/int/nevs.h"
    "src/int/pcx.cc"
    "src/int/pcx.h"
    "src/int/region.cc"
    "src/int/region.h"
    "src/int/share1.cc"
    "src/int/share1.h"
    "src/int/sound.cc"
    "src/int/sound.h"
    "src/int/support/intextra.cc"
    "src/int/support/intextra.h"
    "src/int/widget.cc"
    "src/int/widget.h"
    "src/int/window.cc"
    "src/int/window.h"
    "src/plib/assoc/assoc.cc"
    "src/plib/assoc/assoc.h"
    "src/plib/color/color.cc"
    "src/plib/color/color.h"
    "src/plib/db/db.cc"
    "src/plib/db/db.h"
    "src/plib/db/lzss.cc"
    "src/plib/db/lzss.h"
    "src/plib/gnw/button.cc"
    "src/plib/gnw/button.h"
    "src/plib/gnw/debug.cc"
    "src/plib/gnw/debug.h"
    "src/plib/gnw/dxinput.cc"
    "src/plib/gnw/dxinput.h"
    "src/plib/gnw/grbuf.cc"
    "src/plib/gnw/grbuf.h"
    "src/plib/gnw/input.cc"
    "src/plib/gnw/input.h"
    "src/plib/gnw/gnw_types.h"
    "src/plib/gnw/gnw.cc"
    "src/plib/gnw/gnw.h"
    "src/plib/gnw/intrface.cc"
    "src/plib/gnw/intrface.h"
    "src/plib/gnw/kb.cc"
    "src/plib/gnw/kb.h"
    "src/plib/gnw/memory.cc"
    "src/plib/gnw/memory.h"
    "src/plib/gnw/mouse.cc"
    "src/plib/gnw/mouse.h"
    "src/plib/gnw/rect.cc"
    "src/plib/gnw/rect.h"
    "src/plib/gnw/svga_types.h"
    "src/plib/gnw/svga.cc"
    "src/plib/gnw/svga.h"
    "src/plib/gnw/text.cc"
    "src/plib/gnw/text.h"
    "src/plib/gnw/vcr.cc"
    "src/plib/gnw/vcr.h"
    "src/plib/gnw/winmain.cc"
    "src/plib/gnw/winmain.h"
    "src/movie_lib.cc"
    "src/movie_lib.h"
    "src/sound_decoder.cc"
    "src/sound_decoder.h"
    "src/audio_engine.cc"
    "src/audio_engine.h"
    "src/fps_limiter.cc"
    "src/fps_limiter.h"
    "src/platform_compat.cc"
    "src/platform_compat.h"
    "src/pointer_registry.cc"
    "src/pointer_registry.h"
    "src/plib/gnw/touch.cc"
    "src/plib/gnw/touch.h"
    "third_party/fpattern/fpattern.c"
    "third_party/fpattern/fpattern.h"
    "src/logger.cc"
    "src/logger.h"
    )
else()
    message(FATAL_ERROR "This CMakeLists.txt is configured to build for Switch only.")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if(NX)
    set(CMAKE_PREFIX_PATH "/opt/devkitpro/portlibs/switch")

    include_directories("${CMAKE_SOURCE_DIR}/src")
    include_directories(/opt/devkitpro/portlibs/switch/include)
    link_directories(/opt/devkitpro/portlibs/switch/lib /opt/devkitpro/libnx/lib)

    find_package(SDL2 REQUIRED)
    find_package(ZLIB REQUIRED)

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -specs=/opt/devkitpro/libnx/switch.specs")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -specs=/opt/devkitpro/libnx/switch.specs")

    target_link_libraries(${EXECUTABLE_NAME}
        ${SDL2_LIBRARY}
        ${SDL2_TTF_LIBRARY}
        ${SDL2_MIXER_LIBRARY}
        ${SDL2_IMAGE_LIBRARY}
        ${ZLIB_LIBRARY}
        ${EGL_LIBRARY}
        ${NOUVEAU_LIBRARY}
        ${GLAPI_LIBRARY}
        ${GLES_LIBRARY}
        ${NX_LIBS}
        ${FREETYPE_LIBRARY}
        ${HARFBUZZ_LIBRARY}
        png
        bz2
        z
        m
    )

    add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
        COMMAND /opt/devkitpro/tools/bin/nacptool --create "fallout-ce" "Interplay / alexbatalov / ryandeering" "${F1_VERSION}" ${CMAKE_BINARY_DIR}/fallout-ce.nacp
        COMMAND /opt/devkitpro/tools/bin/elf2nso ${EXECUTABLE_NAME} ${EXECUTABLE_NAME}.nso
        COMMAND /opt/devkitpro/tools/bin/elf2nro ${EXECUTABLE_NAME} ${EXECUTABLE_NAME}.nro --nacp=${CMAKE_BINARY_DIR}/fallout-ce.nacp
    )

endif()