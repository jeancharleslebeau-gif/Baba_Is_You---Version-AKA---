/*
===============================================================================
  audio_player.h — Librairie audio générique (réutilisable)
-------------------------------------------------------------------------------
  Rôle :
    - Fournir un moteur de mixage simple (tons, bruit, WAV).
    - Être totalement indépendant du moteur de jeu (pas de dépendance à baba).
    - Permettre la réutilisation dans d’autres projets.

  Auteur : Jean-Charles LEBEAU
  Date   : Janvier 2026
===============================================================================
*/

#pragma once
#include <cstdint>
#include <cstdio>

// Constante par défaut (modifiable si besoin)
constexpr int AUDIO_PLAYER_DEFAULT_SAMPLE_RATE = 22050;

// -------------------------------------------------------------
// Base track
// -------------------------------------------------------------
struct audio_track_base {
    float volume = 1.0f;
    virtual bool is_active() const = 0;
    virtual int16_t next_sample() = 0;
    virtual ~audio_track_base() {}
};

// -------------------------------------------------------------
// Tone track
// -------------------------------------------------------------
struct audio_track_tone : public audio_track_base {
    int phase;
    int phase_inc;
    int remaining_samples;

    audio_track_tone();
    void play_tone(int freq_hz, int duration_ms);
    int16_t next_sample() override;
    bool is_active() const override { return remaining_samples > 0; }
};

// -------------------------------------------------------------
// Noise track
// -------------------------------------------------------------
struct audio_track_noise : public audio_track_base {
    int remaining_samples;
    int volume_int;

    audio_track_noise();
    void play_noise(int duration_ms, int vol);
    int16_t next_sample() override;
    bool is_active() const override { return remaining_samples > 0; }
};

// -------------------------------------------------------------
// WAV track
// -------------------------------------------------------------
constexpr int WAV_BUFFER_SAMPLES = 1024;   // ⚠️ définie AVANT la struct

struct audio_track_wav : public audio_track_base {
    FILE* file;
    int16_t buffer[WAV_BUFFER_SAMPLES];
    uint32_t samples_remaining;
    uint32_t buffer_len;
    float pos;
    bool active;

    float pitch = 1.0f;
    float target_pitch = 1.0f;
    float pitch_smooth = 0.05f;

    audio_track_wav();
    void start(FILE* f, uint32_t sample_count);
    void fill_buffer();
    int16_t next_sample() override;
    bool is_active() const override { return active; }
};

// -------------------------------------------------------------
// Audio player (mixing engine)
// -------------------------------------------------------------
constexpr int MAX_TRACKS = 8;

struct audio_player {
    audio_track_base* tracks[MAX_TRACKS];
    float master_volume = 1.0f;

    audio_player();
    void add_track(audio_track_base* t);
    int mix(int16_t* out, int count);
};
