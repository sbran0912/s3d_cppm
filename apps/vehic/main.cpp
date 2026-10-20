import renderer;
import math_core;
import color;
import mesh;
import draw;
import bodies;
import vehicle;

#include <cstddef>
#include <cstdio>
#include <exception>
#include <vector>

// Importiert werden nur die tatsaechlich verwendeten std-Namen.
using std::exception;
using std::fprintf;
using std::size_t;
using std::vector;

// ====================================================================
// Vehic – Fahrzeug-Schwarm mit Steering-Behaviors und DNA
//
// Zehn Fahrzeuge (Pyramiden) steuern per Seek gutes Futter (gruen) und Gift
// (rot) an; die DNA bestimmt Staerke und Wahrnehmungsradius. Fressen veraendert
// die Gesundheit, Altern (1.5 % pro Frame) zehrt sie auf; bei Health < 0
// stirbt ein Fahrzeug.
//
// Gegenueber einem einfachen Body-Ansatz:
//   - Szene/Kamera/Nebel als Buendel (Scene/BeginScene).
//   - GPU-Instancing: alle Futterkugeln in EINEM Draw-Call (Farbe pro
//     Instanz), alle Fahrzeuge in EINEM Draw-Call (Farbe nach Health).
//   - Expliziter FlushBatch() zwischen opaker Welt und Debug-Overlays, damit
//     die transparenten DNA-Kreise weiterhin zuletzt (obenauf) gezeichnet
//     werden.
// ====================================================================

int main() {
    Scene scene;
    scene.Cam.Pos    = {50, 20, 200};
    scene.Cam.Target = {0, 0, 0};
    scene.Cam.Up     = {0, 1, 0};
    scene.Cam.FovY   = 1.2;
    scene.Cam.Near   = 0.1;
    scene.Cam.Far    = 1000;
    scene.SunDir     = {0.5, 1.0, 0.3};
    scene.sceneFog   = {true, 100, 400, ColorState{0.25, 0.25, 0.25, 1}};

    const ShapeStyle gridStyle{
        .stroke = ColorInput("#777774"),
        .lineWidth = 1,
    };
    // Futter/Fahrzeuge: Farbe kommt beim Instancing pro Instance; der Style
    // bestimmt nur Fill+Stroke und Linienbreite.
    const ShapeStyle foodStyle{
        .lineWidth = 1,
        .mode = StyleBoth,
    };
    const ShapeStyle vehicleStyle{
        .lineWidth = 1,
        .mode = StyleBoth,
    };

    // Meshes EINMAL erzeugen und ueber alle Instanzen teilen.
    const Mesh gridMesh = MakeGridMesh(600, 24);     // Bodengitter
    const Mesh foodMesh = MakeSphereMesh(3, 16, 16); // Futter-Kugel (Radius 3)

    // Fahrzeug-Pyramide: breit und kompakt (kein duenner Spiess), damit der
    // Koerper aus jeder Blickrichtung als 3D-Form lesbar ist. Basis bei y=0 ->
    // um die Position zentrieren.
    constexpr double vehicleBase = 6.0;
    constexpr double vehicleHeight = 12.0;
    const Mesh vehicleMesh = CenterMeshY(MakePyramidMesh(vehicleBase, vehicleHeight), -vehicleHeight / 2);

    // Instanz-Farben.
    const ColorState poisonColor = ParseHex("#FF0000");
    const ColorState foodColor = ParseHex("#44ff44");
    const ColorState vehicleHealthy = ParseHex("#ffffff");
    // Schwache Fahrzeuge bewusst NICHT rot (sonst mit dem roten Gift-Futter
    // verwechselbar), sondern orange.
    const ColorState vehicleWeak = ParseHex("#ff8800");

    // 30x Gift (rot) und 30x gutes Futter (gruen).
    Food poison = NewFood(30);
    Food food = NewFood(30);

    // 10 Fahrzeuge mit zufaelliger Startgeschwindigkeit.
    vector<Vehicle> vehicles;
    vehicles.reserve(10);
    for (int i = 0; i < 10; i++) {
        Vehicle v{MakeBody(vehicleMesh, Vec3{0, 20, 100})};
        v.vel = Vec3{
            RandomFloat(-2, 2),
            RandomFloat(-2, 2),
            RandomFloat(-2, 2),
        };
        vehicles.push_back(v);
    }

    // Wiederverwendete Instanz-Puffer.
    vector<Instance> foodInstances;
    vector<Instance> vehicleInstances;
    foodInstances.reserve(60);
    vehicleInstances.reserve(10);

    /* Loop */
    auto draw = [&]() {
        BeginScene(scene);

        // Futter nachwachsen lassen (min 20, +30 pro Respawn).
        food = Respawn(food, 20, 30);
        poison = Respawn(poison, 20, 30);

        // Altern: mit 1.5% Wahrscheinlichkeit pro Frame altern alle Fahrzeuge.
        const bool getOlder = RandomFloat(0, 1) < 0.015;

        // Fahrzeuge simulieren (rueckwaerts, damit Entfernen beim Iterieren ok
        // ist).
        for (size_t i = vehicles.size(); i > 0; i--) {
            size_t idx = i-1;
            Vehicle& v = vehicles[idx];

            v.boundary();
            food = v.eatFood(food, false);   // gutes Futter
            poison = v.eatFood(poison, true); // Gift
            v.alignToVelocity();
            v.update();

            if (getOlder) {
                v.health -= 0.05;
            }

            if (v.isDead()) {
                vehicles.erase(vehicles.begin() + idx);
            }
        }

        // Bodengitter (XZ-Ebene bei y=0) aus dem geteilten Mesh.
        DrawMesh(gridMesh, gridStyle);

        // Futter-Instanzen: Gift (rot) + gutes Futter (gruen) in einem Puffer.
        foodInstances.clear();
        for (const Vec3& p : poison) {
            foodInstances.push_back(Instance{TranslateMatrix(p.x, p.y, p.z), poisonColor});
        }
        for (const Vec3& f : food) {
            foodInstances.push_back(Instance{TranslateMatrix(f.x, f.y, f.z), foodColor});
        }

        // Fahrzeug-Instanzen: Farbe nach Gesundheit (orange wenn schwach).
        vehicleInstances.clear();
        for (const Vehicle& v : vehicles) {
            const ColorState col = v.health < 0.5 ? vehicleWeak : vehicleHealthy;
            vehicleInstances.push_back(Instance{v.body.ModelMatrix(), col});
        }

        // Je EIN Instancing-Draw-Call fuer Futter bzw. Fahrzeuge.
        DrawMeshInstanced(foodMesh, foodStyle, foodInstances);
        DrawMeshInstanced(vehicleMesh, vehicleStyle, vehicleInstances);

        // Opake Welt zeichnen, BEVOR die transparenten Debug-Overlays kommen.
        FlushBatch();

        // Debug-Overlays (Batched Drawing): Heading-Pfeile + DNA-Radien.
        for (const Vehicle& v : vehicles) {
            const Vec3 hp = v.body.pos;

            // Heading-Pfeil: kurze Linie vom Fahrzeug in Fahrtrichtung (kuerzer
            // als der Koerper, damit beides nicht zu einer Linie verschmilzt).
            const Vec3 end = hp.Add(v.heading.Scale(5));
            PrimitiveStyle arrowStyle{.stroke = ColorInput("#ffffff")};
            if (v.health < 0.5) {
                arrowStyle.stroke = ColorInput("#ff4444");
            }
            DrawLine(hp.x, hp.y, hp.z, end.x, end.y, end.z, arrowStyle);

            // DNA-Radien als Kreise in der XZ-Ebene. DrawCircle(x,y,z) zeichnet
            // in der XY-Ebene – durch den Tausch (x, z, y) liegt der Kreis flach.
            DrawCircle(hp.x, hp.z, hp.y, v.dna[3],
                       ShapeStyle{.stroke = ColorInput(51, 255, 51, 64), .segments = 48});
            DrawCircle(hp.x, hp.z, hp.y, v.dna[2],
                       ShapeStyle{.stroke = ColorInput(255, 51, 51, 64), .segments = 48});
        }
    };

    /* Start */
    try {
        Init();
    } catch (const exception& e) {
        fprintf(stderr, "%s\n", e.what());
        return 1;
    }

    StartAnimation(draw);
    return 0;
}
