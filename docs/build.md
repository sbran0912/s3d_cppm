# Build-Prozess

Dieses Projekt ist ein C++20-Modules-Demo mit **CMake + Ninja + Clang 22**.
Alle Bibliotheksfunktionen liegen als C++20-Module (`.cppm`) in `lib/`,
die Apps (und der Test) importieren sie per `import` und werden direkt
gelinkt – es gibt keine separate statische Bibliothek mehr.

> Für die API-Referenz siehe [`docs/library.md`](library.md).

## Verzeichnisstruktur

```
.
├── CMakeLists.txt              Build-Konfiguration (CMake)
├── .zed/settings.json          clangd-Konfiguration (Zed)
├── lib/                        C++20-Module der Bibliothek
│   ├── color.cppm              → module color;
│   ├── math_core.cppm          → module math_core;
│   ├── mesh.cppm               → module mesh;
│   ├── mesh_io.cppm            → module mesh_io;
│   ├── shader.cppm             → module shader;
│   ├── bodies.cppm             → module bodies;
│   ├── draw.cppm               → module draw;
│   └── renderer.cppm           → module renderer;  (ca. 1100 Zeilen)
├── apps/                       Anwendungen
│   ├── boids/                  main.cpp + vehicle.cppm (Flocking, Module)
│   ├── vehic/                  main.cpp + vehicle.cppm (DNA + Instancing)
│   ├── rocket/                 main.cpp + vehicle.cppm + rocket.obj
│   ├── ray/main.cpp
│   ├── vorlage/main.cpp
│   ├── beispiel/main.cpp
│   └── instanzen/main.cpp
├── tests/
│   └── lib3d_test.cpp
├── build/                      Build-Ergebnisse (von CMake/Ninja)
└── docs/                       Dokumentation
```

## Voraussetzungen

- **CMake** ≥ 3.28 (für `FILE_SET CXX_MODULES`)
- **Ninja**
- **Clang** (getestet mit Clang 22.1.2)
- **GLFW3** + **OpenGL** (`libglfw3-dev`, `libgl-dev`)

## Build

```bash
# Einmalig: Projekt konfigurieren
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++-22

# Nach Quellcode-Änderungen (inkrementeller Build)
cmake --build build

# Einzelne App bauen
cmake --build build --target rocket
```

Der Compiler muss explizit gesetzt werden, da `clang++` (ohne Version) nicht
im System-PATH liegt – nur die versionierte Variante `clang++-22`.

## Ausführen

Die Binaries liegen in `build/`. Am besten aus diesem Verzeichnis starten:

```bash
cd build && ./beispiel
cd build && ./boids
cd build && ./rocket      # sucht rocket.obj im selben Verzeichnis
cd build && ./vehic
cd build && ./ray
cd build && ./instanzen
cd build && ./vorlage
```

> **Wichtig:** Die App `rocket` lädt `rocket.obj` zur Laufzeit. Das Binary
> sucht die Datei relativ zum Arbeitsverzeichnis – der Build kopiert sie
> nach `build/`. Starte `rocket` daher am besten mit `cd build && ./rocket`.

## Test

```bash
cd build && ./lib3d_test
```

Oder über CMake:

```bash
cmake --build build --target lib3d_test && ./build/lib3d_test
```

## Aufräumen

```bash
# Build-Artefakte löschen (Konfiguration bleibt erhalten)
cmake --build build --target clean

# Komplett (inkl. Konfiguration)
rm -rf build
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++-22
```

## Wie Module gebaut werden

CMake 4.x erkennt `.cppm`-Dateien über `FILE_SET CXX_MODULES` und steuert
den korrekten Build-Ablauf über `clang-scan-deps`:

1. **Scannen** – `clang-scan-deps` ermittelt die Abhängigkeiten zwischen
   den Modulen (wer importiert wen).
2. **Interface kompilieren** – Jedes `.cppm` wird einmal als Modul-Interface
   übersetzt (erzeugt `.pcm`-Datei für andere Module und den LSP).
3. **Abhängige Dateien kompilieren** – Apps/Tests werden mit
   `-fmodule-file=<mod>=<pfad>.pcm` übersetzt.
4. **Linken** – Alle Objektdateien einer App werden zum Binary gelinkt.

Da Module pro App-Target isoliert sind, können mehrere Apps ein Modul
gleichen Namens (z. B. `module vehicle;` in `boids`, `rocket`, `vehic`)
verwenden, ohne sich gegenseitig zu stören.

## Erweitern

### Neues Bibliotheks-Modul

1. `.cppm` in `lib/` anlegen (mit `export module <name>;`).
2. Den Dateinamen in `CXXMODS` in `CMakeLists.txt` eintragen.
3. Von Apps/anderen Modulen per `import <name>;` nutzbar.

### Neue App

1. Ordner `apps/<name>/` mit `main.cpp` anlegen.
2. In `CMakeLists.txt` mit `add_s3d_app(name apps/<name>/main.cpp)` eintragen.
3. Bei Bedarf ein app-eigenes Modul (`<name>.cppm`) als weiteres Argument
   übergeben – CMake registriert `.cppm`-Dateien automatisch über das
   `FILE_SET CXX_MODULES`.

### Externe Meshes (`*.obj`)

OBJ-Dateien werden wie `rocket.obj` behandelt: per `add_custom_command`
nach `build/` kopieren, damit die App sie zur Laufzeit findet.

## Hinweise

- `build/` und `*.o` sind über `.gitignore` von der Versionsverwaltung
  ausgenommen.
- Zed/LSP-Konfiguration siehe `.zed/settings.json` (clangd-22 mit
  `--experimental-modules-support`); `compile_commands.json` wird durch
  `CMAKE_EXPORT_COMPILE_COMMANDS ON` in `build/` erzeugt.