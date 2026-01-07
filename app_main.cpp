//===============================================================================
 // app_main.cpp — Point d’entrée du moteur BabaIsU (AKA Edition)
//-------------------------------------------------------------------------------
//  Rôle :
//    - Initialiser l’ensemble du hardware (LCD, audio, input, SD, expander).
//    - Charger les ressources globales (sprites, paramètres audio, niveaux).
//    - Lancer les tâches FreeRTOS (jeu, input, audio si nécessaire).
//    - Fournir une boucle idle propre et stable.
//
//  Contexte :
//    Ce projet est une réécriture moderne du moteur "Baba Is You" adaptée à
//    l’AKA. Il s’appuie sur :
//      - un moteur de grille multi-objets,
//      - un moteur de règles dynamique (SUBJECT IS PROPERTY),
//      - un moteur de mouvement complet,
//      - un rendu basé sur un atlas 16×16,
//      - un pipeline DMA LCD optimisé,
//      - un système audio I2S propre et indépendant du framerate.
//
//  Objectifs :
//    - Architecture claire, modulaire, extensible.
//    - Séparation stricte des responsabilités :
//        * app_main.cpp : initialisation + lancement des tâches
//        * task_game.cpp : boucle de jeu (40 FPS)
//        * core/* : moteur logique (grille, règles, mouvement)
//        * game/* : gestion des niveaux, état global
//        * assets/* : sprites, audio, données
//    - Code lisible, documenté, facile à maintenir.
//
//  Auteur : Jean-Charles LEBEAU (Jicehel)
//  
//  Création : 01/2026
//===============================================================================

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// -----------------------------------------------------------------------------
//  Hardware AKA
// -----------------------------------------------------------------------------
#include "lib/LCD.h"
#include "lib/expander.h"
#include "lib/sdcard.h"

// -----------------------------------------------------------------------------
//  Core du moteur
// -----------------------------------------------------------------------------
#include "core/graphics.h"
#include "core/input.h"
#include "core/audio.h"
#include "core/sprites.h"

// -----------------------------------------------------------------------------
//  Logique de jeu
// -----------------------------------------------------------------------------
#include "game/game.h"

// -----------------------------------------------------------------------------
//  Tâches FreeRTOS
// -----------------------------------------------------------------------------
#include "tasks/task_game.h"
#include "tasks/task_input.h"
// (La tâche audio est interne à audio_init() si nécessaire)



// extern "C" void app_main()
//{
//    // Lancer la tâche input sur le core 1
//    xTaskCreatePinnedToCore(task_input, "TaskInput", 4096, nullptr, 5, nullptr, 1);
//
//    while (true)
//    {
//        // Affiche l’état de toutes les touches
//        printf("Keys: A=%d B=%d Start=%d Menu=%d "
//               "Up=%d Down=%d Left=%d Right=%d "
//               "L=%d R=%d\n",
//               g_keys.A,
//               g_keys.B,
//               g_keys.Start,
//               g_keys.Menu,
//               g_keys.Up,
//               g_keys.Down,
//               g_keys.Left,
//               g_keys.Right,
//               g_keys.L,
//               g_keys.R);
//
//        vTaskDelay(pdMS_TO_TICKS(200)); // rafraîchissement toutes les 200 ms
//    }
//} 



// ============================================================================
//  INITIALISATION HARDWARE
// ============================================================================
static void hardware_init()
{
    printf("[BabaIsU] Initialisation hardware...\n");

    // --- Expander / GPIO externes ---
    if (expander_init() != 0) {
        printf("Erreur init expander\n");
    }

    // --- LCD + rétroéclairage ---
    lcd_init_pwm();
    lcd_update_pwm(64);     // luminosité par défaut

	// ADC (batterie + joystick) 
	adc_init();
	
    // --- initialise la façade graphique 
    gfx_init();

    // --- SD card (pour niveaux, assets, etc.) ---
    sd_init();

    // --- Input (boutons, joystick, etc.) ---
    input_init();

    // --- Audio (placeholder : moteur prêt mais pas de musique chargée) ---
    baba::audio_init();

    // --- Sprites (atlas 16×16) ---
    baba::sprites_init();

    // --- Logique de jeu ---
    baba::game_init();

    printf("[BabaIsU] Hardware OK.\n");
}


// ============================================================================
//  POINT D’ENTRÉE AKA
// ============================================================================
extern "C" void app_main(void)
{
    printf("\n=============================================\n");
    printf("  BabaIsU — Moteur Puzzle AKA Edition\n");
    printf("  (c) Jean‑Charles — Architecture modulaire\n");
    printf("=============================================\n\n");

    hardware_init();

    // -------------------------------------------------------------------------
    //  Création des tâches FreeRTOS
    // -------------------------------------------------------------------------

    // Tâche principale du jeu (40 FPS)
    xTaskCreatePinnedToCore(
        baba::task_game,
        "GameTask",
        8192,
        nullptr,
        5,
        nullptr,
        1       // Core 1 : dédié au gameplay + rendu
    );

    // Tâche input (lecture boutons à cadence stable)
    xTaskCreatePinnedToCore(
        baba::task_input,
        "InputTask",
        2048,
        nullptr,
        4,
        nullptr,
        1       // Core 1 : proche du jeu
    );

    // NOTE :
    // La tâche audio n’est PAS créée ici.
    // Elle est gérée par audio_init() si nécessaire (I2S + callback).

    printf("[BabaIsU] Tâches lancées. Entrée en idle loop.\n");

    // -------------------------------------------------------------------------
    //  Boucle idle
    //  (Laisse tourner les tâches FreeRTOS, évite le watchdog)
    // -------------------------------------------------------------------------
    while (true)
        vTaskDelay(pdMS_TO_TICKS(1000));
}
