/*
===============================================================================
  graphics.cpp — Façade graphique backend-agnostique
-------------------------------------------------------------------------------
  Rôle :
    - Fournir une API graphique unique pour le moteur de jeu.
    - Router les appels vers le backend sélectionné :
        * framebuffer (gfx_fb.*)
        * direct LCD (gfx_direct.*)
    - Permettre au moteur de jeu de rester indépendant du matériel.

  Pourquoi une façade ?
    - Séparer l’API publique du moteur de l’implémentation matérielle.
    - Permettre de basculer entre framebuffer et direct LCD sans modifier
      le code du jeu.
    - Éviter les conflits DMA / écritures directes.
    - Documenter clairement chaque couche.

  Backends disponibles :
    USE_FRAMEBUFFER = 1  → core/gfx_fb.*
        Pipeline moderne, propre, rapide, basé sur un framebuffer
        + DMA via lcd_refresh(). Recommandé.

    USE_FRAMEBUFFER = 0  → core/gfx_direct.*
        Pipeline direct LCD. Utile pour debug matériel.

  API utilisée par le moteur de jeu :
      gfx_init()
      gfx_clear()
      gfx_flush()
      gfx_putpixel16()
      gfx_text()
      gfx_text_center()
      gfx_drawAtlas()

  Auteur : Jean-Charles LEBEAU
  Date   : Janvier 2026
===============================================================================
*/

#include "graphics.h"
#include "game/config.h"

// Sélection du backend selon USE_FRAMEBUFFER (défini via CMake)
#if USE_FRAMEBUFFER
    #include "gfx_fb.h"
#else
    #include "gfx_direct.h"
#endif

// -----------------------------------------------------------------------------
//  Variables globales
// -----------------------------------------------------------------------------

// Couleur de texte active (utilisée par lcd_draw_str / lcd_draw_char)
uint16_t current_text_color = COLOR_WHITE;

// Ancien système graphique (compat éventuelle)
graphics_basic gfx;


// ============================================================================
//  INITIALISATION
// ============================================================================
void gfx_init() {
#if USE_FRAMEBUFFER
    printf("[gfx] init: framebuffer backend\n");
    gfx_fb_init();
#else
    printf("[gfx] init: direct backend\n");
    gfx_direct_init();
#endif
}



// ============================================================================
//  EFFACER L’ÉCRAN
// ============================================================================
void gfx_clear(uint16_t color) {
#if USE_FRAMEBUFFER
    gfx_fb_clear(color);
#else
    gfx_direct_clear(color);
#endif
}


// ============================================================================
//  RAFRAÎCHISSEMENT DE L’ÉCRAN
// ============================================================================
void gfx_flush() {
#if USE_FRAMEBUFFER
    gfx_fb_flush();
#else
    gfx_direct_flush();
#endif
}


// ============================================================================
//  TEXTE
// ============================================================================
void gfx_set_text_color(uint16_t color) {
    current_text_color = color;
}

void gfx_text(int x, int y, const char* txt, uint16_t color) {
#if USE_FRAMEBUFFER
    gfx_fb_text(x, y, txt, color);
#else
    gfx_direct_text(x, y, txt, color);
#endif
}

static const int FONT_CHAR_WIDTH = 6;

int gfx_char_width(char) {
    return FONT_CHAR_WIDTH;
}

int gfx_text_width(const char* text) {
    if (!text) return 0;
    int len = 0;
    for (const char* p = text; *p; ++p) ++len;
    return len * FONT_CHAR_WIDTH;
}

void gfx_text_center(int y, const char* text, uint16_t color) {
    int w = gfx_text_width(text);
    int x = (SCREEN_W - w) / 2;
    gfx_text(x, y, text, color);
}


// ============================================================================
//  PIXELS
// ============================================================================
void gfx_putpixel16(int x, int y, uint16_t color) {
#if USE_FRAMEBUFFER
    gfx_fb_putpixel(x, y, color);
#else
    gfx_direct_putpixel(x, y, color);
#endif
}


// ============================================================================
//  BITMAPS / SPRITE SHEETS (bas niveau)
// ============================================================================
void lcd_draw_bitmap(const uint16_t* pixels, int w, int h, int dx, int dy) {
#if USE_FRAMEBUFFER
    gfx_fb_drawSprite(dx, dy, pixels, w, h);
#else
    gfx_direct_drawSprite(dx, dy, pixels, w, h);
#endif
}

void lcd_draw_partial_bitmap(const uint16_t* pixels,
                             int sheetW, int sheetH,
                             int sx, int sy,
                             int spriteW, int spriteH,
                             int dx, int dy)
{
    // Gestion des débordements source
    if (sx < 0 || sy < 0 ||
        sx + spriteW > sheetW ||
        sy + spriteH > sheetH)
    {
        if (sx < 0) { spriteW += sx; dx -= sx; sx = 0; }
        if (sy < 0) { spriteH += sy; dy -= sy; sy = 0; }
        if (sx + spriteW > sheetW) spriteW = sheetW - sx;
        if (sy + spriteH > sheetH) spriteH = sheetH - sy;
        if (spriteW <= 0 || spriteH <= 0) return;
    }

    const uint16_t* srcBase = pixels + sy * sheetW + sx;

#if USE_FRAMEBUFFER
    for (int j = 0; j < spriteH; ++j) {
        int yy = dy + j;
        if ((unsigned)yy >= SCREEN_H) continue;

        const uint16_t* src = srcBase + j * sheetW;
        for (int i = 0; i < spriteW; ++i) {
            int xx = dx + i;
            if ((unsigned)xx >= SCREEN_W) continue;

            gfx_fb_putpixel(xx, yy, src[i]);
        }
    }
#else
    for (int j = 0; j < spriteH; ++j) {
        int yy = dy + j;
        if ((unsigned)yy >= SCREEN_H) continue;

        const uint16_t* src = srcBase + j * sheetW;
        for (int i = 0; i < spriteW; ++i) {
            int xx = dx + i;
            if ((unsigned)xx >= SCREEN_W) continue;

            gfx_direct_putpixel(xx, yy, src[i]);
        }
    }
#endif
}


// ============================================================================
//  NOUVEAU : gfx_drawAtlas() — utilisé par BabaIsU
// ============================================================================
/*
    Dessine un sprite issu d’un atlas (sprite sheet).

    Paramètres :
        atlasPixels : pointeur vers l’atlas RGB565
        atlasW/H    : dimensions de l’atlas en pixels
        sx, sy      : coordonnées source dans l’atlas
        w, h        : taille du sprite
        dx, dy      : position écran

    Implémentation :
        - Route vers lcd_draw_partial_bitmap()
        - Compatible framebuffer et direct LCD
*/
void gfx_drawAtlas(const uint16_t* atlasPixels,
                   int atlasW, int atlasH,
                   int sx, int sy,
                   int w, int h,
                   int dx, int dy)
{
    lcd_draw_partial_bitmap(
        atlasPixels,
        atlasW, atlasH,
        sx, sy,
        w, h,
        dx, dy
    );
}

// ============================================================================
//  gfx_drawBitmap() — affichage d’un bitmap complet
// ============================================================================
/*
    Affiche un bitmap RGB565 (w×h) à la position (dx, dy).

    Paramètres :
        pixels : pointeur vers les pixels source (RGB565)
        w, h   : dimensions du bitmap
        dx, dy : position écran

    Implémentation :
        - Route vers lcd_draw_bitmap()
        - Compatible framebuffer et direct LCD
*/
void gfx_drawBitmap(const uint16_t* pixels, int w, int h, int dx, int dy)
{
    lcd_draw_bitmap(pixels, w, h, dx, dy);
}

