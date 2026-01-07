/*
===============================================================================
  movement.cpp — Implémentation du moteur de déplacement
-------------------------------------------------------------------------------
  Rôle :
    - Déplacer tous les objets ayant la propriété YOU.
    - Gérer les chaînes de PUSH (ex : YOU → ROCK → ROCK → EMPTY).
    - Respecter STOP (bloque le mouvement).
    - Autoriser la superposition avec les objets non‑STOP (ex : FLAG).
    - Appliquer les effets post‑mouvement (WIN, KILL, SINK).
-------------------------------------------------------------------------------
  Stratégie corrigée :
    1. Snapshot : collecter toutes les positions des objets YOU au début de la frame.
    2. Pour chaque YOU :
         - Construire la chaîne PUSH devant lui (boucle tail→head).
         - Vérifier faisabilité (case après chaîne libre + dans zone jouable).
         - Si faisable → pousser la chaîne d’une case.
         - Vérifier STOP dans la case cible.
         - Si pas STOP → Baba peut entrer (même si FLAG ou autre non‑PUSH).
    3. Déplacer YOU d’une case (toujours un seul pas).
    4. Appliquer les effets (WIN, KILL, SINK) par superposition.
===============================================================================
*/

#include "movement.h"
#include <algorithm>

namespace baba {

// ============================================================================
//  Helper : tente de pousser une chaîne d’objets d’une case
// ============================================================================
static bool try_push_chain(Grid& grid, const PropertyTable& props,
                           int startX, int startY, int dx, int dy)
{
    int cx = startX;
    int cy = startY;

    std::vector<std::pair<int,int>> chain;

    // Construire la chaîne PUSH devant YOU
    while (grid.in_bounds(cx, cy) && grid.in_play_area(cx, cy)) {
        Cell& c = grid.cell(cx, cy);
        if (c.objects.empty()) break;

        bool allPush = true;
        for (auto& obj : c.objects) {
            const Properties& pr = props[(int)obj.type];
            if (!pr.isPush) { allPush = false; break; }
            if (pr.isStop) return false; // STOP bloque immédiatement
        }

        if (!allPush) break;

        chain.emplace_back(cx, cy);
        cx += dx;
        cy += dy;
    }

    // Case après la chaîne doit être libre et jouable
    if (!grid.in_bounds(cx, cy) || !grid.in_play_area(cx, cy)) return false;
    if (!grid.cell(cx, cy).objects.empty()) return false;

    // Déplacer la chaîne d’une case (tail → head)
    for (int i = (int)chain.size() - 1; i >= 0; --i) {
        int fromX = chain[i].first;
        int fromY = chain[i].second;
        int toX   = fromX + dx;
        int toY   = fromY + dy;

        Cell& from = grid.cell(fromX, fromY);
        Cell& to   = grid.cell(toX, toY);

        for (auto& obj : from.objects) {
            if (props[(int)obj.type].isPush) {
                to.objects.push_back(obj);
            }
        }
        from.objects.erase(
            std::remove_if(from.objects.begin(), from.objects.end(),
                           [&](const Object& o){ return props[(int)o.type].isPush; }),
            from.objects.end()
        );
    }

    return true;
}

// ============================================================================
//  step() — Applique un déplacement dx/dy à tous les objets YOU
// ============================================================================
MoveResult step(Grid& grid, const PropertyTable& props, int dx, int dy)
{
    MoveResult result;

    // 1) Snapshot des positions YOU au début
    struct YouPos { int x, y; };
    std::vector<YouPos> yous;
    yous.reserve(grid.width * grid.height);

    for (int y = 0; y < grid.height; ++y) {
        for (int x = 0; x < grid.width; ++x) {
            const Cell& c = grid.cell(x, y);
            for (const auto& obj : c.objects) {
                if (props[(int)obj.type].isYou) {
                    yous.push_back({x, y});
                }
            }
        }
    }

    // 2) Pour chaque YOU, tenter de pousser la chaîne devant lui
    for (const auto& yp : yous) {
        int nx = yp.x + dx;
        int ny = yp.y + dy;

        // Bloquer hors grille / hors zone jouable
        if (!grid.in_bounds(nx, ny)) continue;
        if (!grid.in_play_area(nx, ny)) continue;

        // Vérifier STOP dans la case cible
        bool blocked = false;
        for (auto& obj : grid.cell(nx, ny).objects) {
            if (props[(int)obj.type].isStop) { blocked = true; break; }
        }
        if (blocked) continue;

        // Essayer de pousser la chaîne devant (si PUSH)
        try_push_chain(grid, props, nx, ny, dx, dy);

        // 3) Déplacer YOU d’une case (superposition autorisée)
        Cell& src = grid.cell(yp.x, yp.y);
        for (auto it = src.objects.begin(); it != src.objects.end(); ) {
            if (props[(int)it->type].isYou) {
                grid.cell(nx, ny).objects.push_back(*it);
                it = src.objects.erase(it);
            } else {
                ++it;
            }
        }
    }

    // 4) Effets post-mouvement par superposition
    for (auto& cell : grid.cells) {
        bool hasYou = false, hasWin = false, hasKill = false, hasSink = false;
        for (auto& obj : cell.objects) {
            const Properties& pr = props[(int)obj.type];
            if (pr.isYou)  hasYou = true;
            if (pr.isWin)  hasWin = true;
            if (pr.isKill) hasKill = true;
            if (pr.isSink) hasSink = true;
        }
        if (hasYou && hasWin) result.hasWon = true;
        if (hasYou && (hasKill || hasSink)) result.hasDied = true;
    }

    return result;
}

} // namespace baba
