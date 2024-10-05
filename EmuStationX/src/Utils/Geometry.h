#pragma once

#include "Base/Base.h"
#include "Core/IRenderer.h"

namespace esx {

    // Funzione di utilità per calcolare il prodotto scalare tra due vettori 2D
    I32 dotProduct(const Vertex& a, const Vertex& b);

    // Funzione di utilità per calcolare la normale ad un lato del triangolo
    Vertex getEdgeNormal(const Vertex& a, const Vertex& b);

    // Funzione di utilità per proiettare un triangolo lungo un asse
    void projectTriangle(const Vertex& a, const Vertex& b, const Vertex& c, const Vertex& axis, I32& min, I32& max);

    // Funziotne per verificare la sovrapposizione tra due intervalli (proiezioni)
    bool overlapOnAxis(I32 minA, I32 maxA, I32 minB, I32 maxB);

    // Funzione principale per verificare la sovrapposizione tra due triangoli
    bool checkOverlap(const Vertex& triA_a, const Vertex& triA_b, const Vertex& triA_c, const Vertex& triB_a, const Vertex& triB_b, const Vertex& triB_c);
}