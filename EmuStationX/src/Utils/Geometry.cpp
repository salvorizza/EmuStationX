#include "Geometry.h"

namespace esx {

    // Funzione di utilità per calcolare il prodotto scalare tra due vettori 2D
    I32 dotProduct(const Vertex& a, const Vertex& b) {
        return a.x * b.x + a.y * b.y;
    }

    // Funzione di utilità per calcolare la normale ad un lato del triangolo
    Vertex getEdgeNormal(const Vertex& a, const Vertex& b) {
        Vertex edge = { I16(b.x - a.x), I16(b.y - a.y) };
        // Calcolo della normale (perpendicolare al lato)
        return { I16(- edge.y), edge.x};
    }

    // Funzione di utilità per proiettare un triangolo lungo un asse
    void projectTriangle(const Vertex& a, const Vertex& b, const Vertex& c, const Vertex& axis, I32& min, I32& max) {
        min = max = dotProduct(a, axis);
        {
            I32 projection = dotProduct(b, axis);
            if (projection < min) {
                min = projection;
            }
            if (projection > max) {
                max = projection;
            }
        }
        {
            I32 projection = dotProduct(c, axis);
            if (projection < min) {
                min = projection;
            }
            if (projection > max) {
                max = projection;
            }
        }
    }

    // Funziotne per verificare la sovrapposizione tra due intervalli (proiezioni)
    bool overlapOnAxis(I32 minA, I32 maxA, I32 minB, I32 maxB) {
        return !(maxA < minB || maxB < minA);  // Non si sovrappongono se c'è uno spazio tra gli intervalli
    }

    // Funzione principale per verificare la sovrapposizione tra due triangoli
    BIT checkOverlap(const Vertex& triA_a, const Vertex& triA_b, const Vertex& triA_c, const Vertex& triB_a, const Vertex& triB_b, const Vertex& triB_c) {
        Vertex axes[6];

        // Calcola le normali ai lati di triA
        axes[0] = getEdgeNormal(triA_a, triA_b);
        axes[1] = getEdgeNormal(triA_b, triA_c);
        axes[2] = getEdgeNormal(triA_c, triA_a);

        // Calcola le normali ai lati di triB
        axes[3] = getEdgeNormal(triB_a, triB_b);
        axes[4] = getEdgeNormal(triB_b, triB_c);
        axes[5] = getEdgeNormal(triB_c, triB_a);

        // Controlla se esiste un asse di separazione
        for (int i = 0; i < 6; ++i) {
            I32 minA, maxA, minB, maxB;

            // Proietta entrambi i triangoli sull'asse corrente
            projectTriangle(triA_a, triA_b, triA_c, axes[i], minA, maxA);
            projectTriangle(triB_a, triB_b, triB_c, axes[i], minB, maxB);

            // Controlla se c'è una separazione lungo questo asse
            if (!overlapOnAxis(minA, maxA, minB, maxB)) {
                // C'è un asse di separazione, quindi i triangoli non si sovrappongono
                return ESX_FALSE;
            }
        }

        // Nessun asse di separazione trovato, quindi i triangoli si sovrappongono
        return ESX_TRUE;
    }
}