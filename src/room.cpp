// room.cpp — Room shell geometry for the Interactive 3D Hostel Room Planner.
//
// World coordinate system (1 unit = 1 metre):
//   X  → right      room: X ∈ [-2.00,  +2.00]   (4.0 m wide)
//   Y  ↑ up         room: Y ∈ [ 0.00,  +2.80]   (2.8 m high)
//   Z  ↗ toward     room: Z ∈ [-2.25,  +2.25]   (4.5 m deep)
//
// Walls:
//   Back  wall  Z = -2.25  (cream, has door and window openings)
//   Left  wall  X = -2.00  (solid deep navy accent)
//   Right wall  X = +2.00  (cream, solid)
//   Front wall  Z = +2.25  intentionally OMITTED for camera visibility
//   Back-wall window X = [-0.20, +1.20], Y = [1.00, 2.00]
//
// Normals are emitted per face so GL_LIGHTING (Person 2) works without change.

#include <GL/freeglut.h>
#include <algorithm>
#include <array>
#include "room.h"

// ─── Room dimension constants ────────────────────────────────────────────────

static const float RX = 2.00f;   // X half-width    (wall at  ±RX)
static const float RZ = 2.25f;   // Z half-depth    (wall at  ±RZ)
static const float RH = 2.80f;   // room height     (ceiling at RH)

// Door opening — back wall (Z = -RZ), hinge on left jamb at X = DOOR_X1
static const float DOOR_X1 = -1.80f;   // left edge of door opening
static const float DOOR_X2 = -0.90f;   // right edge of door opening
static const float DOOR_H  =  2.00f;   // door height

// Window opening — back wall (Z = -RZ)
static const float WIN_X1 = -0.20f;   // left edge of window
static const float WIN_X2 =  1.20f;   // right edge of window
static const float WIN_Y1 =  1.00f;   // sill height
static const float WIN_Y2 =  2.00f;   // lintel height

// ─── drawBox ────────────────────────────────────────────────────────────────
// Solid axis-aligned box, six quads, one glNormal per face.
// cx, cz : horizontal centre;  cy : bottom (base) Y;  w/h/d in metres.
// Caller sets glColor before calling.

void drawBox(float cx, float cy, float cz,
             float w,  float h,  float d)
{
    float x1 = cx - w * 0.5f,  x2 = cx + w * 0.5f;
    float y1 = cy,              y2 = cy + h;
    float z1 = cz - d * 0.5f,  z2 = cz + d * 0.5f;

    glBegin(GL_QUADS);

    // Front face  (+Z)
    glNormal3f( 0.f, 0.f,  1.f);
    glVertex3f(x1, y1, z2);  glVertex3f(x2, y1, z2);
    glVertex3f(x2, y2, z2);  glVertex3f(x1, y2, z2);

    // Back face   (-Z)
    glNormal3f( 0.f, 0.f, -1.f);
    glVertex3f(x2, y1, z1);  glVertex3f(x1, y1, z1);
    glVertex3f(x1, y2, z1);  glVertex3f(x2, y2, z1);

    // Left face   (-X)
    glNormal3f(-1.f, 0.f,  0.f);
    glVertex3f(x1, y1, z1);  glVertex3f(x1, y1, z2);
    glVertex3f(x1, y2, z2);  glVertex3f(x1, y2, z1);

    // Right face  (+X)
    glNormal3f( 1.f, 0.f,  0.f);
    glVertex3f(x2, y1, z2);  glVertex3f(x2, y1, z1);
    glVertex3f(x2, y2, z1);  glVertex3f(x2, y2, z2);

    // Top face    (+Y)
    glNormal3f( 0.f,  1.f, 0.f);
    glVertex3f(x1, y2, z2);  glVertex3f(x2, y2, z2);
    glVertex3f(x2, y2, z1);  glVertex3f(x1, y2, z1);

    // Bottom face (-Y)
    glNormal3f( 0.f, -1.f, 0.f);
    glVertex3f(x1, y1, z1);  glVertex3f(x2, y1, z1);
    glVertex3f(x2, y1, z2);  glVertex3f(x1, y1, z2);

    glEnd();
}

// ─── Floor ───────────────────────────────────────────────────────────────────

static void drawFloor()
{
    // Base tile surface — warm beige
    glNormal3f(0.f, 1.f, 0.f);
    glColor3f(0.90f, 0.86f, 0.78f);
    glBegin(GL_QUADS);
        glVertex3f(-RX, 0.f, -RZ);
        glVertex3f( RX, 0.f, -RZ);
        glVertex3f( RX, 0.f,  RZ);
        glVertex3f(-RX, 0.f,  RZ);
    glEnd();

    // Grout grid — thin, low-contrast, drawn at Y = 0.002 to avoid Z-fighting.
    // Grid spacing 0.50 m → tiles are 0.50 × 0.50 m (realistic floor tile).
    const float gY  = 0.002f;   // slightly above the base quad
    const float gt  = 0.012f;   // half-thickness of each grout line (2.4 cm total)
    const float gs  = 0.50f;    // grid spacing in metres

    glColor3f(0.80f, 0.76f, 0.70f);   // slightly darker beige — subtle contrast
    glBegin(GL_QUADS);

    // Lines running along X (spaced along Z axis)
    for (float z = -RZ; z <= RZ + 0.001f; z += gs) {
        glVertex3f(-RX, gY, z - gt);
        glVertex3f( RX, gY, z - gt);
        glVertex3f( RX, gY, z + gt);
        glVertex3f(-RX, gY, z + gt);
    }
    // Lines running along Z (spaced along X axis)
    for (float x = -RX; x <= RX + 0.001f; x += gs) {
        glVertex3f(x - gt, gY, -RZ);
        glVertex3f(x + gt, gY, -RZ);
        glVertex3f(x + gt, gY,  RZ);
        glVertex3f(x - gt, gY,  RZ);
    }

    glEnd();
}

// ─── Ceiling ─────────────────────────────────────────────────────────────────

static void drawCeiling()
{
    // Main ceiling — off-white
    glNormal3f(0.f, -1.f, 0.f);
    glColor3f(0.97f, 0.95f, 0.90f);
    glBegin(GL_QUADS);
        glVertex3f(-RX, RH, -RZ);
        glVertex3f(-RX, RH,  RZ);
        glVertex3f( RX, RH,  RZ);
        glVertex3f( RX, RH, -RZ);
    glEnd();

    // Ceiling lamp placeholder — flush square plate at ceiling centre.
    // Person 2: add a GL_LIGHT0 point light at position (0, RH - 0.03, 0).
    glColor3f(0.93f, 0.90f, 0.82f);
    drawBox(0.0f, RH - 0.025f, 0.0f, 0.38f, 0.025f, 0.38f);
}

// ─── Back wall (Z = -RZ) with door opening ───────────────────────────────────

static void drawBackWall(const RoomOpenings& openings)
{
    const float z = -RZ;
    glNormal3f(0.f, 0.f, 1.f);
    glColor3f(0.94f, 0.90f, 0.82f);
    std::array<float, 6> xs = {{-RX,
        openings.doorCenterX - openings.doorWidth * 0.5f,
        openings.doorCenterX + openings.doorWidth * 0.5f,
        openings.windowCenterX - openings.windowWidth * 0.5f,
        openings.windowCenterX + openings.windowWidth * 0.5f, RX}};
    std::sort(xs.begin(), xs.end());
    const float doorTop = openings.doorHeight;
    const float winBottom = openings.windowSillY;
    const float winTop = winBottom + openings.windowHeight;
    std::array<float, 6> ys = {{0.f, doorTop, winBottom, winTop, RH, RH}};
    std::sort(ys.begin(), ys.end());
    for (std::size_t xi = 0; xi + 1 < xs.size(); ++xi) {
        if (xs[xi + 1] - xs[xi] < 0.0001f) continue;
        const float midX = (xs[xi] + xs[xi + 1]) * 0.5f;
        const bool inDoor = midX > openings.doorCenterX - openings.doorWidth * 0.5f &&
                            midX < openings.doorCenterX + openings.doorWidth * 0.5f;
        const bool inWindow = midX > openings.windowCenterX - openings.windowWidth * 0.5f &&
                              midX < openings.windowCenterX + openings.windowWidth * 0.5f;
        for (std::size_t yi = 0; yi + 1 < ys.size(); ++yi) {
            const float y0 = ys[yi], y1 = ys[yi + 1];
            if (y1 - y0 < 0.0001f) continue;
            const float midY = (y0 + y1) * 0.5f;
            const bool voidCell = (inDoor && midY < doorTop) ||
                                  (inWindow && midY > winBottom && midY < winTop);
            if (voidCell) continue;
            glBegin(GL_QUADS);
                glVertex3f(xs[xi], y0, z); glVertex3f(xs[xi + 1], y0, z);
                glVertex3f(xs[xi + 1], y1, z); glVertex3f(xs[xi], y1, z);
            glEnd();
        }
    }
}

// ─── Left wall (X = -RX) — solid deep navy accent wall ──────────────────────

static void drawLeftWall()
{
    const float x = -RX;
    glNormal3f(1.f, 0.f, 0.f);
    glColor3f(0.055f, 0.10f, 0.24f);   // deep navy
    glBegin(GL_QUADS);
    glVertex3f(x, 0.f, -RZ); glVertex3f(x, 0.f, RZ);
    glVertex3f(x, RH, RZ);   glVertex3f(x, RH, -RZ);
    glEnd();
}

// ─── Right wall (X = +RX) — cream, no openings ───────────────────────────────

static void drawRightWall()
{
    glNormal3f(-1.f, 0.f, 0.f);        // inward normal (-X)
    glColor3f(0.94f, 0.90f, 0.82f);    // warm cream
    const float x = RX;
    glBegin(GL_QUADS);
        glVertex3f(x, 0.f,  RZ);
        glVertex3f(x, 0.f, -RZ);
        glVertex3f(x, RH,  -RZ);
        glVertex3f(x, RH,   RZ);
    glEnd();
}

// ─── drawRoom — public entry point ───────────────────────────────────────────

void drawRoom(bool includeCeiling, const RoomOpenings* openings)
{
    const RoomOpenings defaultOpenings = {
        (DOOR_X1 + DOOR_X2) * 0.5f, DOOR_X2 - DOOR_X1, DOOR_H,
        (WIN_X1 + WIN_X2) * 0.5f, WIN_Y1, WIN_X2 - WIN_X1, WIN_Y2 - WIN_Y1};
    drawFloor();
    if (includeCeiling) drawCeiling();
    drawBackWall(openings ? *openings : defaultOpenings);
    drawLeftWall();
    drawRightWall();
    // Front wall (Z = +RZ) is intentionally omitted for camera visibility.
    // Person 2: if a skybox or exterior is added, the front wall can be drawn here.
}
