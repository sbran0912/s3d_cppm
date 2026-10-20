module;

#include <array>
#include <cmath>
#include <limits>
#include <vector>

export module vehicle;

import bodies;
import math_core;
import mesh;

// ====================================================================
// FUTTER
//
// Food ist eine Menge von Futterpositionen. Das gemeinsame Mesh wird nicht
// hier verwaltet, daher reichen die Positionen.
// ====================================================================
export using Food = std::vector<Vec3>;

// ====================================================================
// VEHICLE - Fahrzeug mit Steering-Behavior (Seek) und DNA
//
// Ein Vehicle ist ein Body (Mesh + Position/Rotation) mit zusaetzlichem
// Physik-Zustand (Geschwindigkeit/Beschleunigung), einer Gesundheit und einer
// DNA. Die DNA steuert, wie stark Gift bzw. gutes Futter angesteuert wird und
// in welchem Radius es ueberhaupt wahrgenommen wird.
// ====================================================================
export struct Vehicle {
    Body body;
    Vec3 vel{};
    Vec3 accel{};
    Vec3 heading{};
    double health = 1.0;
    std::array<double, 4> dna{};

    // Vehicle erzeugt ein Fahrzeug aus einem Body mit zufaelliger DNA.
    Vehicle(const Body& b);

    // alignToVelocity passt die Richtung des Bodys an die Geschwindigkeit an.
    void alignToVelocity();

    // applyForce wendet eine Kraft an (akkumuliert in accel).
    void applyForce(Vec3 force);

    // update ist das Physik-Update: Geschwindigkeit aus Beschleunigung,
    // Position aus Geschwindigkeit. Die Geschwindigkeit wird auf [0.5, 2.0]
    // begrenzt.
    void update();

    // seek ist das Seek-Verhalten. Die gewuenschte Richtung wird je nach
    // Futter-Art mit dem passenden DNA-Wert skaliert (dna[0] fuer Gift, dna[1]
    // fuer gutes Futter).
    void seek(Vec3 target, bool isBadFood);

    // eatFood sucht das naechste Futter im DNA-Radius und frisst es, wenn es
    // nah genug ist. Gutes Futter -> Health +0.1, Gift -> Health -0.1.
    // Ansonsten wird es angesteuert (seek). Rueckgabe: ggf. gekuerztes
    // Food-Array.
    Food eatFood(Food food, bool isBadFood);

    // boundary kehrt die Geschwindigkeit an den Raendern um (Bounce).
    void boundary();

    // isDead gibt true zurueck, wenn das Vehicle tot ist (Health < 0).
    bool isDead() const;
};

// ====================================================================
// HILFSFUNKTIONEN
// ====================================================================

// NewFood erzeugt count Futterpositionen an Zufallspositionen.
export Food NewFood(int count);

// Respawn fuellt das Futter-Array auf: sind weniger als min vorhanden, kommen
// count neue dazu.
export Food Respawn(Food food, int min, int count);

// CenterMeshY verschiebt ein Mesh entlang Y. MakePyramidMesh legt die Basis
// bei y=0 (Spitze bei y=height); fuer ein um die Position zentriertes Fahrzeug
// wird das Mesh um -height/2 verschoben.
export Mesh CenterMeshY(const Mesh& mesh, double dy);

// RandomFoodPos liefert eine zufaellige Futterposition in [-100, 100).
export Vec3 RandomFoodPos();

// ====================================================================
// IMPLEMENTIERUNG
// ====================================================================

Vehicle::Vehicle(const Body& b)
    : body{b},
      dna{
          RandomFloat(-1.0, 1.0),  // dna[0]: Kraft Richtung Gift
          RandomFloat(-1.0, 1.0),  // dna[1]: Kraft Richtung gutes Futter
          RandomFloat(20.0, 60.0), // dna[2]: Radius fuer Gift
          RandomFloat(20.0, 60.0), // dna[3]: Radius fuer gutes Futter
      } {}

void Vehicle::alignToVelocity() {
    const double mag = vel.Length();
    if (mag < 0.0001) {
        return;
    }

    const double magXZ = std::sqrt(vel.x * vel.x + vel.z * vel.z);

    // vel.y/mag muss in [-1,1] liegen (Schutz vor NaN durch Float-Rundung).
    const double cosY = Constrain(vel.y / mag, -1.0, 1.0);
    body.rotX = std::acos(cosY);
    if (magXZ < 0.0001) {
        body.rotY = 0.0;
    } else {
        body.rotY = std::atan2(vel.x / magXZ, vel.z / magXZ);
    }
    body.rotZ = 0.0;

    // Heading als normalisierte Richtung ableiten.
    heading = vel.Normalize();
}

void Vehicle::applyForce(Vec3 force) {
    accel = accel.Add(force);
}

void Vehicle::update() {
    vel = vel.Add(accel);
    const double speed = vel.Length();
    vel = vel.Normalize().Scale(Constrain(speed, 0.5, 2.0));
    accel = Vec3{};
    body.pos = body.pos.Add(vel);
}

// seekForce bestimmt, wie stark das Seek-Steering pro Frame in die
// Beschleunigung eingeht. Kleiner = ruhigeres, traegeres Anfliegen; die
// Geschwindigkeit selbst bleibt davon unberuehrt.
constexpr double seekForce = 0.1;

// steerLimit ist die Obergrenze der Steuerkraft (Laenge des Steering-Vektors).
// Kleiner = sanftere, weniger scharfe Korrekturen bei grossem Kursfehler,
// waehrend kleine Korrekturen unveraendert bleiben.
constexpr double steerLimit = 1.0;

void Vehicle::seek(Vec3 target, bool isBadFood) {
    Vec3 desired = target.Sub(body.pos).Limit(3.0);
    desired = isBadFood ? desired.Scale(dna[0]) : desired.Scale(dna[1]);
    const Vec3 steer = desired.Sub(vel).Limit(steerLimit);
    applyForce(steer.Scale(seekForce));
}

Food Vehicle::eatFood(Food food, bool isBadFood) {
    double filter = dna[3];
    if (isBadFood) {
        filter = dna[2];
    }
    double mindist = std::numeric_limits<double>::infinity();
    int idx = -1;

    for (std::size_t i = 0; i < food.size(); i++) {
        const double distance = body.pos.DistanceTo(food[i]);
        if (distance < filter && distance < mindist) {
            mindist = distance;
            idx = static_cast<int>(i);
        }
    }

    if (idx > -1) {
        if (body.pos.DistanceTo(food[static_cast<std::size_t>(idx)]) < 3.0) {
            // Food aufessen: aus dem Array entfernen genuegt – es gibt keinen
            // GPU-Buffer/Refcount mehr freizugeben (nur CPU-Geometrie).
            food.erase(food.begin() + idx);
            if (isBadFood) {
                health += -0.1;
            } else {
                health += 0.1;
            }
        } else {
            seek(food[static_cast<std::size_t>(idx)], isBadFood);
        }
    }
    return food;
}

void Vehicle::boundary() {
    constexpr double minX = -130.0;
    constexpr double maxX = 130.0;
    constexpr double minY = -130.0;
    constexpr double maxY = 130.0;
    constexpr double minZ = -130.0;
    constexpr double maxZ = 130.0;

    if (body.pos.x < minX || body.pos.x > maxX) {
        vel.x *= -1.0;
    }
    if (body.pos.y < minY || body.pos.y > maxY) {
        vel.y *= -1.0;
    }
    if (body.pos.z < minZ || body.pos.z > maxZ) {
        vel.z *= -1.0;
    }
}

bool Vehicle::isDead() const { return health < 0; }

Food NewFood(int count) {
    Food food;
    food.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; i++) {
        food.push_back(RandomFoodPos());
    }
    return food;
}

Food Respawn(Food food, int min, int count) {
    if (static_cast<int>(food.size()) < min) {
        for (int i = 0; i < count; i++) {
            food.push_back(RandomFoodPos());
        }
    }
    return food;
}

Mesh CenterMeshY(const Mesh& mesh, double dy) {
    Mesh out{mesh.Triangles, mesh.Lines};
    const float d = static_cast<float>(dy);
    for (std::size_t i = 1; i < out.Triangles.size(); i += 3) {
        out.Triangles[i] += d;
    }
    for (std::size_t i = 1; i < out.Lines.size(); i += 3) {
        out.Lines[i] += d;
    }
    return out;
}

Vec3 RandomFoodPos() {
    return Vec3{
        static_cast<double>(Random(-100, 100)),
        static_cast<double>(Random(-100, 100)),
        static_cast<double>(Random(-100, 100)),
    };
}
