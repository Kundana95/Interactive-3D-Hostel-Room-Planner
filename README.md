# Interactive 3D Hostel Room Planner

An interactive 3D hostel room modelled in C++ using the fixed-function OpenGL pipeline and FreeGLUT.
Computer Graphics mini project — MSYS2 UCRT64, g++ 14.2.0.

## Build

Open an **MSYS2 UCRT64** terminal, navigate to the project folder, then run:

```sh
mingw32-make
```

This produces `build/hostel_planner.exe`. To clean build artefacts:

```sh
mingw32-make clean
```

## Run

```sh
./build/hostel_planner.exe
```

Or double-click `build/hostel_planner.exe` from Windows Explorer.

## Controls

### Person 1 — implemented

| Key | Action |
|-----|--------|
| `ESC` | Exit |
| `1` / `2` / `3` / `4` | Select furniture item (Milestone 3) |
| `← → ↑ ↓` | Move selected furniture on the floor plane (Milestone 3) |
| `Q` / `E` | Rotate selected furniture ±10° (Milestone 3) |
| `R` | Reset all furniture to default positions (Milestone 3) |

### Person 2 — TODO

- [ ] Replace the static overview camera with first-person WASD navigation.
      See the `gluLookAt` call inside `static void display()` in `src/main.cpp`.
- [ ] Add `GL_LIGHTING` + a `GL_LIGHT0` point light at the ceiling lamp position `(0, 2.775, 0)`.
      The lamp geometry placeholder is in `drawCeiling()` in `src/room.cpp`.
      All wall/furniture normals are already set and ready for lighting.
- [ ] Add a day/night toggle key that switches `glLightfv` ambient/diffuse values.
- [ ] Add a HUD / help overlay using `glutBitmapCharacter`.

## World Coordinates

- **1 OpenGL unit = 1 metre**
- Room:  X ∈ [−2.0, +2.0] (4.0 m wide) · Y ∈ [0.0, 2.8] (2.8 m high) · Z ∈ [−2.25, +2.25] (4.5 m deep)
- Floor at Y = 0, ceiling at Y = 2.8
- Back wall (door) at Z = −2.25 · Left wall (window) at X = −2.0
- Front wall (Z = +2.25) intentionally omitted for camera visibility

## Module Overview

| File | Purpose |
|------|---------|
| `src/main.cpp` | GLUT entry point, display loop, overview camera (Person 2 replaces camera) |
| `src/room.cpp` | Floor, ceiling, walls, door opening, window, curtains |
| `src/furniture.cpp` | Furniture models and position state (Milestone 2) |
| `src/input.cpp` | Keyboard callbacks and furniture interaction (Milestone 3) |
| `include/room.h` | `drawBox()` utility + `drawRoom()` declaration |
| `include/furniture.h` | `Furniture` struct and function declarations (Milestone 2) |
| `include/input.h` | Input function declarations (Milestone 3) |

## Default Furniture Layout

| Item | Centre X | Centre Z | Size (W × H × D) |
|------|----------|----------|------------------|
| Bed | −1.55 | +0.90 | 0.9 × 0.55 × 2.0 m |
| Desk | +1.70 | −0.50 | 0.6 × 0.75 × 1.2 m |
| Chair | +0.70 | −0.50 | 0.45 × 0.9 × 0.45 m |
| Wardrobe | +1.73 | −1.80 | 0.55 × 1.8 × 0.9 m |
