# Regel: Neue C++-Projekte

Diese Regel gilt für alle neuen C++-Projekte unter `~/coding`. Referenzprojekt:
`~/coding/sonstiges/test_cppm`. Aufbau, Toolchain und Dateien werden von dort übernommen.

## 0. Geltungsbereich

Diese Datei gilt **ausschließlich für C++-Projekte**. Ein Projekt ist genau dann ein
C++-Projekt, wenn mindestens eines zutrifft:

- es gibt eine `CMakeLists.txt` mit `project(... LANGUAGES CXX)`
- es gibt `.cpp`-, `.cppm`- oder `.hpp`-Dateien
- der Nutzer sagt ausdrücklich, dass es ein C++-Projekt ist

Für **alle anderen Sprachen und Projektarten** (Go, Rust, Python, JavaScript/TypeScript,
Shell, Web, …) gilt diese Datei **nicht**. Dort werden insbesondere **keine** Regeln
zu CMake, Ninja, `clang++-22`, clangd, `.zed/settings.json` / `.vscode/settings.json`
mit Modul-Flags oder `src/*.cppm` angewandt.

Unabhängig von der Sprache gelten weiterhin die Regeln aus der globalen
`~/.config/opencode/AGENTS.md`:

- Pakete/Abhängigkeiten niemals eigenständig installieren – vorher fragen
- Python und pip ausschließlich in `~/coding/.pyvenv_agent`
- kompilierte Programme und Node.js-Anwendungen nicht selbstständig starten

Bei einem Go-Projekt werden die sprachspezifischen Konventionen des Projekts befolgt
bzw. – wenn noch keine existieren – vor dem Anlegen kurz mit dem Nutzer abgestimmt.
Diese Datei liefert dafür keine Vorgaben.

## 1. Stack (verbindlich)

Jedes neue C++-Projekt wird exakt mit diesem Stack aufgebaut – keine Alternativen,
keine Abweichungen ohne ausdrückliche Freigabe:

- **CMake** ≥ 3.28 (getestet mit 4.2.3), wegen `FILE_SET CXX_MODULES`
- **Ninja** als Generator
- **Clang 22** als Compiler: `clang++-22` (der unversionierte `clang++` ist NICHT im PATH)
- **clangd 22**: `clangd-22`, mit `--experimental-modules-support`
- **C++20** mit echten C++20-Modulen (`.cppm` + `import`)
- **libstdc++** (System-Standardbibliothek). Kein `libc++`, kein `-stdlib=libc++`.
- Standard-Header werden per `#include` im **Global Module Fragment** eingebunden.

Niemals `make`, niemals `clang++` ohne Versionsnummer, niemals C++17-Fallback.

## 2. Projektstruktur (verbindlich)

```
PROJEKTNAME/
├── CMakeLists.txt
├── README.md
├── .zed/
│   └── settings.json
├── .vscode/
│   └── settings.json
├── src/
│   ├── main.cpp          # Einstiegspunkt, konsumiert Module per import
│   ├── <modul>.cppm      # Module Interface Unit(s)
│   └── ...
├── build/                # generiert, nicht versionieren
└── .cache/               # clangd-Index, generiert, nicht versionieren
```

Regeln:
- Modul-Interface-Units enden auf `.cppm` und beginnen mit `export module <name>;`
- `main.cpp` und andere Konsumenten bleiben `.cpp`
- Jedes Modul = eine Datei, ein Modulname, klein geschrieben
- Header-Dateien nur, wenn zwingend nötig – bevorzugt Module

## 3. CMakeLists.txt (Vorlage, exakt übernehmen)

```cmake
cmake_minimum_required(VERSION 3.28)
project(PROJEKTNAME LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(PROJEKTNAME
  src/main.cpp
  src/MODUL1.cppm
  src/MODUL2.cppm
)
target_sources(PROJEKTNAME PRIVATE
  FILE_SET CXX_MODULES FILES
    src/MODUL1.cppm
    src/MODUL2.cppm
)
```

Wichtig: `CMAKE_EXPORT_COMPILE_COMMANDS ON` ist Pflicht – davon lebt clangd.
Die `.cppm`-Dateien stehen sowohl in `add_executable` als auch im `FILE_SET CXX_MODULES`.

## 4. .zed/settings.json (Pflicht in JEDEM Projekt)

Damit Zed die C++20-Module korrekt interpretieren kann, wird **immer** diese Datei
angelegt – ohne Ausnahme:

`.zed/settings.json`
```json
{
  "lsp": {
    "clangd": {
      "binary": {
        "path": "clangd-22",
        "arguments": [
          "--experimental-modules-support",
          "--compile-commands-dir=build"
        ]
      }
    }
  }
}
```

Beide Argumente sind zwingend:
- `--experimental-modules-support` → clangd versteht `.cppm` / `import`
- `--compile-commands-dir=build` → nutzt `build/compile_commands.json` als Grundlage

Ohne diese Datei gilt das Projekt als nicht fertig eingerichtet.

## 5. .vscode/settings.json (Pflicht in JEDEM Projekt)

Damit VSCODE die C++20-Module korrekt interpretieren kann, wird **immer** diese Datei
angelegt – ohne Ausnahme:

`.vscode/settings.json`
```json
{
  "clangd.path": "/usr/bin/clangd-22"
}
```

Damit nutzt die clangd-Erweiterung in VS Code genau die Clang-22-Version, die auch
zum Bauen verwendet wird. Ohne diese Datei gilt das Projekt als nicht fertig
eingerichtet.

## 6. Quellcode-Konventionen

Modul-Datei:
```cpp
module;
#include <string>

export module MODULNAME;

export int funktion(int x) {
    return x * x;
}
```

Konsument:
```cpp
#include <iostream>

import MODULNAME;

int main() {
    std::cout << funktion(7) << "\n";
}
```

Regeln:
- `module;` steht ganz oben, danach alle `#include`s, dann `export module ...;`
- Jede öffentliche Entität explizit mit `export` markieren
- In `main.cpp` zuerst `#include`, dann `import`s, jeweils alphabetisch
- `using namespace std;` ist verboten (im Modul höchstens `using std::string;`)
- Include-Pfade relativ zu `src/`, keine `../`-Konstrukte

## 7. README.md (Pflicht, auf Deutsch)

Jedes Projekt bekommt eine README mit genau diesen Abschnitten:

1. **Titel + Kurzbeschreibung** (was das Projekt tut)
2. **Voraussetzungen** – CMake ≥ 3.28, Ninja, Clang 22 (clang++-22/clangd-22), libstdc++
3. **Projektstruktur** – Baumdiagramm mit Erklärung jeder Datei
4. **Build**
   - Einmalige Einrichtung:
     ```bash
     cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++-22
     ```
   - Tägliches Bauen:
     ```bash
     cmake --build build
     ```
   - Wann neu konfiguriert werden muss (CMakeLists geändert, Compilerwechsel, `build/` gelöscht)
   - Kurze Erklärung der Phasen: Module scannen (`clang-scan-deps`) → Module kompilieren
     (`.pcm`) → abhängige Dateien kompilieren → linken
5. **Ausführen** – `./build/PROJEKTNAME`
6. **Lizenz** – Unlicense (Public Domain)

## 8. Build-Ablauf

```bash
# einmalig konfigurieren
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++-22

# danach immer nur noch
cmake --build build

# starten
./build/PROJEKTNAME
```

- `build/` wird nie eingecheckt und nie manuell mit Dateien befüllt.
- Neu konfigurieren nur bei Änderung an `CMakeLists.txt`, Compilerwechsel oder
  gelöschtem `build/`.
- Das Programm wird **nicht selbstständig gestartet** – dem Nutzer werden Binary-Pfad
  und Startbefehl genannt, der Start erfolgt erst auf ausdrückliche Aufforderung.
- Pakete/Abhängigkeiten werden **nie** eigenständig installiert; vorher wird gefragt.

## 9. Abschluss-Checkliste

Ein Projekt ist erst fertig, wenn alle Punkte erfüllt sind:

- [ ] `CMakeLists.txt` mit C++20, `FILE_SET CXX_MODULES`, `CMAKE_EXPORT_COMPILE_COMMANDS ON`
- [ ] `src/` mit `main.cpp` und mindestens einem `.cppm`-Modul
- [ ] `.zed/settings.json` mit `clangd-22`, `--experimental-modules-support`, `--compile-commands-dir=build`
- [ ] `.vscode/settings.json` mit `"clangd.path": "/usr/bin/clangd-22"`
- [ ] `README.md` mit allen sechs Abschnitten
- [ ] Konfiguration mit `clang++-22` + Ninja erfolgreich durchgelaufen
- [ ] `cmake --build build` läuft fehlerfrei durch
- [ ] `build/compile_commands.json` existiert
- [ ] Nutzer über Binary-Pfad und Startbefehl informiert

## Anmerkungen

- Eine `.gitignore` mit `build/` und `.cache/` ist im Referenzprojekt nicht vorhanden
  (kein Git-Repo). Bei neuen Git-Projekten ist sie empfehlenswert.
- `.zed/settings.json` nutzt den relativen Pfad `build`. Das funktioniert, solange Zed
  clangd im Projektwurzelverzeichnis startet. Bei Problemen wäre
  `${ZED_WORKTREE_ROOT}/build` robuster.
- Falls die clangd-Erweiterung in VS Code die Module trotz `clangd.path` nicht erkennt,
  können die Flags zusätzlich in `.vscode/settings.json` ergänzt werden:
  ```json
  {
    "clangd.path": "/usr/bin/clangd-22",
    "clangd.arguments": [
      "--experimental-modules-support",
      "--compile-commands-dir=build"
    ]
  }
  ```
  Standard ist die obige Minimalvariante.
