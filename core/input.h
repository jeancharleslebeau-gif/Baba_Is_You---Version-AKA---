/*
===============================================================================
  input.h — Interface du système d’entrée
-------------------------------------------------------------------------------
  Rôle :
    - Définir la structure Keys (état complet des entrées).
    - Fournir les fonctions d’initialisation et de polling.
    - Offrir un helper pour détecter les pressions longues.

  Auteur : Jean-Charles LEBEAU
  Date   : Janvier 2026
===============================================================================
*/

#pragma once
#include <cstdint>

// -----------------------------------------------------------------------------
//  Structure Keys : état complet des entrées
// -----------------------------------------------------------------------------
struct Keys {
    uint32_t raw;       // état brut des touches (bitmask)
    uint32_t pressed;   // touches pressées ce frame
    uint32_t released;  // touches relâchées ce frame

    int joxx, joxy;     // joystick analogique

    bool up, down, left, right;
    bool A, B, C, D;
    bool RUN, MENU, R1, L1;
	int joyX; // -1, 0, +1 selon le joystick horizontal 
	int joyY; // -1, 0, +1 selon le joystick vertical
};

// État global des touches (mis à jour par input_poll)
extern Keys g_keys;

// -----------------------------------------------------------------------------
//  Fonctions principales
// -----------------------------------------------------------------------------
void input_init();
void input_poll(Keys& k);

// Détection d’une pression longue (~1 seconde)
bool isLongPress(const Keys& k, int key);
