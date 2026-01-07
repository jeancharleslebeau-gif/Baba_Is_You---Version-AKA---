/*
===============================================================================
  movement.cpp — Implémentation du moteur de déplacement
-------------------------------------------------------------------------------
  Rôle :
    - Déplacer tous les objets ayant la propriété YOU.
    - Gérer les chaînes de PUSH (ex : YOU → ROCK → ROCK → EMPTY).
    - Respecter STOP (bloque le mouvement).
    - Appliquer les effets post‑mouvement (WIN, KILL, SINK).

  Stratégie :
    1. Déterminer l’ordre de parcours selon la direction (dx/dy).
       Cela évite de pousser deux fois le même objet.
    2. Pour chaque YOU :
         - Vérifier si la case cible est libre, PUSH ou STOP.
         - Résoudre récursivement les PUSH.
         - Déplacer l’objet (un seul pas).
    3. Appliquer les effets (WIN, KILL, SINK) par superposition.
===============================================================================
*/

#include "movement.h"
#include <algorithm>

namespace baba {

// ============================================================================
//  try_move_chain() — Tente de pousser une chaîne d’objets
// ============================================================================
static bool try_move_chain(Grid& grid, const PropertyTable& props,
                           int x, int y, int dx, int dy)
{
    if (!grid.in_bounds(x, y)) return false;

    Cell& c = grid.cell(x, y);
    if (c.objects.empty()) return true;

    // Vérifier STOP
    for (auto& obj : c.objects) {
        const Properties& pr = props[(int)obj.type];
        if (isStop(pr)) return false;
    }

    // Vérifier PUSH
    bool hasPush = false;
    for (auto& obj : c.objects) {
        const Properties& pr = props[(int)obj.type];
        if (isPush(pr)) {
            hasPush = true;
            // Vérifie si la case suivante peut être libérée
            if (!try_move_chain(grid, props, x + dx, y + dy, dx, dy))
                return false;
        }
    }

    // Déplacement effectif : un seul pas
    if (hasPush) {
        Cell& next = grid.cell(x + dx, y + dy);
        for (auto& obj : c.objects) {
            if (props[(int)obj.type].isPush) {
                next.objects.push_back(obj);
            }
        }
        // Retirer uniquement les objets poussés
        c.objects.erase(
            std::remove_if(c.objects.begin(), c.objects.end(),
                           [&](const Object& o){ return props[(int)o.type].isPush; }),
            c.objects.end()
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

    for (int y = 0; y < grid.height; ++y) {
        for (int x = 0; x < grid.width; ++x) {
            Cell& c = grid.cell(x, y);

            for (auto it = c.objects.begin(); it != c.objects.end(); ) {
                Object obj = *it;
                const Properties& pr = props[(int)obj.type];

                if (isYou(pr)) {
                    int nx = x + dx;
                    int ny = y + dy;

                    if (!grid.in_bounds(nx, ny)) {
                        ++it;
                        continue;
                    }

                    if (try_move_chain(grid, props, nx, ny, dx, dy)) {
                        Cell& next = grid.cell(nx, ny);
                        next.objects.push_back(obj);
                        it = c.objects.erase(it); // enlève Baba de la case source
                        continue;
                    }
                }

                ++it;
            }
        }
    }

    // Effets post-mouvement par superposition
    for (auto& cell : grid.cells) {
        bool hasYou = false, hasWin = false, hasKill = false, hasSink = false;
        for (auto& obj : cell.objects) {
            const Properties& pr = props[(int)obj.type];
            if (isYou(pr))  hasYou = true;
            if (isWin(pr))  hasWin = true;
            if (isKill(pr)) hasKill = true;
            if (isSink(pr)) hasSink = true;
        }
        if (hasYou && hasWin) result.hasWon = true;
        if (hasYou && (hasKill || hasSink)) result.hasDied = true;
    }

    return result;
}

} // namespace baba
