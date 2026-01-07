if(USE_FRAMEBUFFER) 
	set(GFX_BACKEND_SRCS 
		core/gfx_fb.cpp 
	) 
	else() 
	set(GFX_BACKEND_SRCS 
		core/gfx_direct.cpp 
	) 
endif()

idf_component_register(
    SRCS
        app_main.cpp
        # Lib / matériel
        lib/LCD.cpp
        lib/expander.cpp
        lib/graphics_basic.cpp
        lib/sdcard.cpp
        lib/audio_sfx.cpp
        lib/pmf_player.cpp
        lib/pmf_player_esp32s3.cpp
        lib/audio_track_sfx.cpp
        lib/audio_sfx_cache.cpp
        lib/audio_player.cpp

        # Core
        core/audio.cpp
        core/input.cpp
        core/grid.cpp
        core/graphics.cpp
        core/rules.cpp
        core/movement.cpp
        core/sprites.cpp
        core/persist.cpp

        # Backend graphique sélectionné
        ${GFX_BACKEND_SRCS}

        # Game
        game/game.cpp
	game/levels_data.cpp
        game/levels.cpp   

        # Tasks
        tasks/task_game.cpp
        tasks/task_input.cpp

        # Assets
        assets/gfx/title.cpp
	assets/gfx/atlas.cpp

    INCLUDE_DIRS
        .
        core
        lib
        game
        tasks
	assets
	assets/gfx
)
