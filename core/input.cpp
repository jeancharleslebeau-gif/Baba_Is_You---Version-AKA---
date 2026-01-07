/*
===============================================================================
  input.cpp — Gestion des entrées (boutons + joystick)
-------------------------------------------------------------------------------
  Rôle :
    - Lire l’état brut des touches via l’expander.
    - Détecter les pressions / relâchements.
    - Mapper les bits vers des booléens lisibles (A, B, UP, MENU…).
    - Lire les axes du joystick analogique.

  Notes :
    - Le module met à jour g_keys, accessible globalement.
    - input_poll() doit être appelé à chaque frame.
    - isLongPress() détecte une pression longue (~1 seconde).

  Auteur : Jean-Charles LEBEAU
  Date   : Janvier 2026
===============================================================================
*/

#include "input.h"
#include "lib/expander.h"
#include "driver/gpio.h"

static uint16_t prev = 0;   // état précédent des touches
Keys g_keys;                // état global des touches

// -----------------------------------------------------------------------------
//  Initialisation
// -----------------------------------------------------------------------------
void input_init() {
    prev = 0;
}

// -----------------------------------------------------------------------------
//  Lecture complète des entrées
// -----------------------------------------------------------------------------
void input_poll(Keys& k) {
    // Lecture brute des touches (bitmask)
    uint16_t raw = expander_read();
    k.raw = raw;

    // Détection des transitions
    k.pressed  = raw & ~prev;   // nouvelles touches pressées
    k.released = prev & ~raw;   // touches relâchées
    prev       = raw;

    // Mapping des boutons
    k.up    = raw & EXPANDER_KEY_UP;
    k.down  = raw & EXPANDER_KEY_DOWN;
    k.left  = raw & EXPANDER_KEY_LEFT;
    k.right = raw & EXPANDER_KEY_RIGHT;

    k.A     = raw & EXPANDER_KEY_A;
    k.B     = raw & EXPANDER_KEY_B;
    k.C     = raw & EXPANDER_KEY_C;
    k.D     = raw & EXPANDER_KEY_D;
    k.RUN   = raw & EXPANDER_KEY_RUN;
    k.MENU  = raw & EXPANDER_KEY_MENU;
    k.R1    = raw & EXPANDER_KEY_R1;
    k.L1    = raw & EXPANDER_KEY_L1;

    // Joystick analogique
    k.joxx = adc_read_joyx();
    k.joxy = adc_read_joyy();
	
	// Normalisation en -1 / 0 / +1
	if (k.joxx < JOYX_LOW)      k.joyX = -1;
	else if (k.joxx > JOYX_HIGH) k.joyX = +1;
	else                         k.joyX = 0;

	if (k.joxy < JOYX_LOW)      k.joyY = -1;
	else if (k.joxy > JOYX_HIGH) k.joyY = +1;
	else                         k.joyY = 0;

    // Mise à jour globale
    g_keys = k;
}

// -----------------------------------------------------------------------------
//  Détection d’une pression longue
// -----------------------------------------------------------------------------
bool isLongPress(const Keys& k, int key) {
    static int pressDuration = 0;

    if (k.raw & key) {
        pressDuration++;
        if (pressDuration > 60) { // ~1 seconde si 60 FPS
            pressDuration = 0;
            return true;
        }
    } else {
        pressDuration = 0;
    }
    return false;
}
