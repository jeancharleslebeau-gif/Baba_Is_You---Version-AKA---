/*
===============================================================================
  audio.h — Interface audio (wrapper Baba vers audio_player)
-------------------------------------------------------------------------------
  Rôle :
    - Fournir une API simple et stable pour le moteur Baba.
    - Servir de pont entre les réglages du jeu (g_audio_settings)
      et la librairie générique audio_player.
    - Permettre au gameplay d’appeler des sons (move, win, lose, push)
      sans dépendre du backend.
    - Préparer l’intégration future avec le moteur audio AKA :
        * I2S + DMA
        * mixeur temps réel
        * cache SFX
        * musique PMF

  Notes :
    - L’API est volontairement minimaliste pour rester stable.
    - Les fonctions audio_play_* créent des pistes et les ajoutent au mixeur.
    - audio_update() sera appelée à chaque frame pour appliquer le volume
      et gérer le mixage.
    - L’implémentation réelle (I2S, DMA) sera ajoutée plus tard.

  Auteur : Jean-Charles LEBEAU
  Date   : Janvier 2026
===============================================================================
*/

#pragma once

#include <cstdint>

namespace baba {

// -----------------------------------------------------------------------------
//  Constantes globales
// -----------------------------------------------------------------------------
static constexpr int SAMPLE_RATE = 22050; // valeur par défaut (Hz)

// -----------------------------------------------------------------------------
//  Réglages audio globaux
// -----------------------------------------------------------------------------
struct AudioSettings {
    bool music_enabled   = true;   // musique activée/désactivée
    uint8_t music_volume = 88;     // volume musique (0–255)
    uint8_t sfx_volume   = 120;    // volume effets sonores (0–255)
    uint8_t master_volume = 140;   // volume global (0–255)
};

// Instance globale des réglages audio
extern AudioSettings g_audio_settings;

// -----------------------------------------------------------------------------
//  API publique (appelée par le moteur Baba)
// -----------------------------------------------------------------------------

// Initialisation du système audio
void audio_init();

// Effets sonores (placeholders reliés à audio_player)
void audio_play_move();
void audio_play_win();
void audio_play_lose();
void audio_play_push();

// Mise à jour du mixeur audio (appelée chaque frame)
void audio_update();

} // namespace baba
