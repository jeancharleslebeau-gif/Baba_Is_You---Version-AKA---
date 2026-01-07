/*
===============================================================================
  task_game.cpp — Boucle de jeu principale (40 FPS)
-------------------------------------------------------------------------------
  Rôle :
    - Exécuter la logique du jeu selon l’état courant (GameMode).
    - Appeler :
        * game_show_title()   → écran de titre
        * game_update()       → logique de déplacement + règles
        * game_draw()         → rendu de la grille
    - Gérer les transitions :
        * Title → Playing
        * Playing → Win / Dead
        * Win / Dead → Restart
        * Menu → retour vers Playing
    - Maintenir une cadence stable (~40 FPS).

  Notes :
    - Les entrées sont lues dans task_input.cpp et stockées dans g_keys.
    - Cette tâche tourne sur le core 1 (défini dans app_main.cpp).
    - Le moteur audio est indépendant (I2S + DMA).

  Auteur : Jean-Charles LEBEAU
  Date   : Janvier 2026
===============================================================================
*/

#include <cstdio>               // pour printf, sprintf
#include "task_game.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "core/input.h"
#include "core/graphics.h"
#include "core/audio.h"

#include "game/game.h"   // game_state(), game_mode(), game_update(), game_draw(), game_load_level()

namespace baba {

// -----------------------------------------------------------------------------
//  Variables internes pour gérer les transitions et les appuis de touches
// -----------------------------------------------------------------------------
static GameMode s_prevMode = GameMode::Title;
static Keys     s_prevKeys{};

// Fonctions utilitaires pour détecter les appuis (front montant)
static inline bool pressed_A(const Keys& now)    { return now.A    && !s_prevKeys.A; }
static inline bool pressed_B(const Keys& now)    { return now.B    && !s_prevKeys.B; }
static inline bool pressed_MENU(const Keys& now) { return now.MENU && !s_prevKeys.MENU; }

static inline bool pressed_UP(const Keys& now)    { return now.up    && !s_prevKeys.up; }
static inline bool pressed_DOWN(const Keys& now)  { return now.down  && !s_prevKeys.down; }
static inline bool pressed_LEFT(const Keys& now)  { return now.left  && !s_prevKeys.left; }
static inline bool pressed_RIGHT(const Keys& now) { return now.right && !s_prevKeys.right; }


// -----------------------------------------------------------------------------
//  Actions à effectuer lors de l’entrée dans un nouvel état
// -----------------------------------------------------------------------------
static void on_enter_mode(GameMode m)
{
    switch (m)
    {
        case GameMode::Title:
            // Affiche l’écran de titre une seule fois
            gfx_clear(COLOR_BLACK);
            game_show_title();
            gfx_text_center(200, "Press A to start", COLOR_WHITE);
            gfx_flush();
            break;

        case GameMode::Playing:
            // Nettoie l’écran avant de dessiner la grille
            gfx_clear(COLOR_BLACK);
            gfx_flush();
            break;

		case GameMode::Win:
			gfx_clear(COLOR_BLACK);
			gfx_text_center(100, "YOU WIN!", COLOR_WHITE);
			gfx_text_center(140, "Press A to restart", COLOR_WHITE);
			gfx_flush();
			fade_in();   // effet d’apparition, pas de fade_out
			break;

		case GameMode::Dead:
			gfx_clear(COLOR_BLACK);
			game_show_title(); // affiche le titre en arrière-plan
			gfx_text_center(100, "YOU DIED!", COLOR_RED);
			gfx_text_center(140, "Press A to return to title", COLOR_WHITE);
			gfx_flush();
			fade_in();
			break;

        case GameMode::Menu:
            // Transition vers le menu (affichage géré dans la boucle)
            fade_out();
            break;
    }
}

// -----------------------------------------------------------------------------
//  Tâche de jeu principale (cadence ~40 FPS)
// -----------------------------------------------------------------------------
void task_game(void*)
{
    printf("[GameTask] Démarrage de la boucle de jeu.\n");

    // Démarre en mode Title
    game_mode() = GameMode::Title;
    on_enter_mode(game_mode());
    s_prevMode = game_mode();

    while (true)
    {
        Keys k = g_keys;

        // Détecte changement d’état et applique les actions d’entrée
        if (game_mode() != s_prevMode) {
            on_enter_mode(game_mode());
            s_prevMode = game_mode();
        }

        // Routage selon l’état courant
        switch (game_mode())
        {
            case GameMode::Title:
                // Attente appui sur A pour lancer le jeu
                if (pressed_A(k)) {
                    game_load_level(0);
                    game_mode() = GameMode::Playing;
                }
                break;

            case GameMode::Playing:
                // Logique de jeu
                game_update();

                if (game_state().hasWon) {
                    game_mode() = GameMode::Win;
                    break;
                }
                if (game_state().hasDied) {
                    game_mode() = GameMode::Dead;
                    break;
                }

                if (pressed_MENU(k)) {
                    fade_out();
                    game_mode() = GameMode::Menu;
                    break;
                }
                break;

            case GameMode::Win:
                // Dans Win/Dead, accepte A en niveau (pas besoin de front montant)
                if (k.A) {
                    // Avance le niveau ou relance celui-ci (selon ta logique interne)
                    game_win_continue();
                    game_mode() = GameMode::Playing;
                }
                break;

			case GameMode::Dead:
				if (k.A) {
					// Retour à l’écran de titre
					game_mode() = GameMode::Title;
					// Pas de reload direct du niveau
				}
				break;

            case GameMode::Menu:
            {
                static int cursor = 0;

                // Navigation
                if (pressed_UP(k))   cursor = (cursor + 2) % 3;
                if (pressed_DOWN(k)) cursor = (cursor + 1) % 3;

                // Actions reliées au moteur audio
                if (cursor == 0 && pressed_A(k)) {
                    g_audio_settings.music_enabled = !g_audio_settings.music_enabled;
                }
                if (cursor == 1 && pressed_LEFT(k)  && g_audio_settings.music_volume > 0)
                    g_audio_settings.music_volume--;
                if (cursor == 1 && pressed_RIGHT(k) && g_audio_settings.music_volume < 255)
                    g_audio_settings.music_volume++;
                if (cursor == 2 && pressed_LEFT(k)  && g_audio_settings.sfx_volume > 0)
                    g_audio_settings.sfx_volume--;
                if (cursor == 2 && pressed_RIGHT(k) && g_audio_settings.sfx_volume < 255)
                    g_audio_settings.sfx_volume++;

                // Affichage du menu
                gfx_clear(COLOR_BLACK);
                gfx_text_center(40, "OPTIONS", COLOR_WHITE);

                gfx_text_center(90,
                    g_audio_settings.music_enabled ? "Music: ON" : "Music: OFF",
                    cursor == 0 ? COLOR_YELLOW : COLOR_WHITE);

                char buf1[32];
                sprintf(buf1, "Music Volume: %d", g_audio_settings.music_volume);
                gfx_text_center(120, buf1, cursor == 1 ? COLOR_YELLOW : COLOR_WHITE);

                char buf2[32];
                sprintf(buf2, "SFX Volume: %d", g_audio_settings.sfx_volume);
                gfx_text_center(150, buf2, cursor == 2 ? COLOR_YELLOW : COLOR_WHITE);

                gfx_text_center(200, "Press B to return", COLOR_WHITE);
                gfx_flush();

                if (pressed_B(k)) {
                    fade_out();
                    game_mode() = GameMode::Playing;
                }
                break;
            }
        } // fin du switch

        // Rendu uniquement en mode Playing
        if (game_mode() == GameMode::Playing) {
            game_draw();
            gfx_flush(); // assure que le rendu de la grille est affiché à chaque frame
        }

        // Sauvegarde des touches pour détection front montant
        s_prevKeys = k;

        // Cadence stable (~40 FPS)
        vTaskDelay(pdMS_TO_TICKS(25));
    }
}

} // namespace baba
