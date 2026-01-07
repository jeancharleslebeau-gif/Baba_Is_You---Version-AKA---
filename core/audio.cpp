/*
===============================================================================
  audio.cpp — Implémentation audio (wrapper Baba vers audio_player)
-------------------------------------------------------------------------------
  Rôle :
    - Fournir une API stable pour le moteur Baba (audio_init, audio_play_*).
    - Servir de pont entre les réglages du jeu (g_audio_settings)
      et la librairie générique audio_player.
    - Permettre au gameplay d’appeler des sons sans dépendre du backend.
    - Préparer l’intégration future avec le moteur audio AKA :
        * I2S + DMA
        * mixeur temps réel
        * cache SFX
        * musique PMF

  Notes :
    - Les fonctions audio_play_* créent des pistes (tone, noise, wav)
      et les ajoutent au mixeur audio_player.
    - audio_update() applique le volume global à chaque frame.
    - L’implémentation réelle (I2S, DMA) sera ajoutée plus tard.
    - Pour l’instant, les sons sont placeholders (ex. beep simple).

  Auteur : Jean-Charles LEBEAU
  Date   : Janvier 2026
===============================================================================
*/

#include "audio.h"
#include "audio_player.h"

namespace baba {

// ============================================================================
//  Variables globales
// ============================================================================
AudioSettings g_audio_settings;   // Réglages audio du moteur Baba
static audio_player player;       // Instance globale du mixeur

// ============================================================================
//  Initialisation du système audio
// ============================================================================
void audio_init() {
    // Initialisation du mixeur avec volume global
    player.master_volume = g_audio_settings.master_volume / 255.0f;
}

// ============================================================================
//  Effets sonores (placeholders reliés à audio_player)
// ============================================================================
void audio_play_move() {
    auto* tone = new audio_track_tone();
    tone->volume = g_audio_settings.sfx_volume / 255.0f;
    tone->play_tone(440, 200); // Exemple : LA 440Hz pendant 200ms
    player.add_track(tone);
}

void audio_play_win() {
    auto* tone = new audio_track_tone();
    tone->volume = g_audio_settings.sfx_volume / 255.0f;
    tone->play_tone(880, 300); // Exemple : 880Hz pendant 300ms
    player.add_track(tone);
}

void audio_play_lose() {
    auto* noise = new audio_track_noise();
    noise->volume = g_audio_settings.sfx_volume / 255.0f;
    noise->play_noise(400, 2000); // Exemple : bruit court
    player.add_track(noise);
}

void audio_play_push() {
    auto* tone = new audio_track_tone();
    tone->volume = g_audio_settings.sfx_volume / 255.0f;
    tone->play_tone(220, 150); // Exemple : basse courte
    player.add_track(tone);
}

// ============================================================================
//  Mise à jour du mixeur audio (appelée chaque frame)
// ============================================================================
void audio_update() {
    // Appliquer le volume global
    player.master_volume = g_audio_settings.master_volume / 255.0f;

    // TODO : envoyer le buffer mixé vers I2S/DMA
    // Exemple placeholder :
    // int16_t buffer[256];
    // player.mix(buffer, 256);
    // -> envoyer buffer vers backend
}

} // namespace baba
