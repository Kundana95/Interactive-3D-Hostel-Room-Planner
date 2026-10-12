// room.cpp — Room shell geometry for the Interactive 3D Hostel Room Planner.
//
// World coordinate system (1 unit = 1 metre):
//   X  → right      room: X ∈ [-2.00,  +2.00]   (4.0 m wide)
//   Y  ↑ up         room: Y ∈ [ 0.00,  +2.80]   (2.8 m high)
//   Z  ↗ toward     room: Z ∈ [-2.25,  +2.25]   (4.5 m deep)
//
// Walls:
//   Back  wall  Z = -2.25  (cream, has door opening)
//   Left  wall  X = -2.00  (sage green accent, has window)
//   Right wall  X = +2.00  (cream, solid)
//   Front wall  Z = +2.25  intentionally OMITTED for camera visibility
//
// Normals are emitted per face so GL_LIGHTING (Person 2) works without change.

#include <GL/freeglut.h>
#include "room.h"

// ─── Room dimension constants ────────────────────────────────────────────────

static const float RX = 2.00f;   // X half-width    (wall at  ±RX)
static const float RZ = 2.25f;   // Z half-depth    (wall at  ±RZ)
static const float RH = 2.80f;   // room height     (ceiling at RH)

// Door opening — back wall (Z = -RZ), hinge on left jamb at X = DOOR_X1
static const float DOOR_X1 = -1.80f;   // left edge of door opening
static const float DOOR_X2 = -0.90f;   // right edge of door opening
static const float DOOR_H  =  2.00f;   // door height

// Window opening — left wall (X = -RX)
static const float WIN_Z1 = -1.20f;   // back  (−Z) edge of window
static const float WIN_Z2 =  0.20f;   // front (+Z) edge of window
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

static void drawBackWall()
{
    // Door opening: X ∈ [DOOR_X1, DOOR_X2], Y ∈ [0, DOOR_H]
    // Wall is drawn in three sections around the opening:
    //   1. Left strip   (X: -RX → DOOR_X1), full height
    //   2. Right strip  (X: DOOR_X2 → +RX), full height
    //   3. Transom      (X: DOOR_X1 → DOOR_X2, Y: DOOR_H → RH)

    const float z = -RZ;
    glNormal3f(0.f, 0.f, 1.f);         // inward normal (+Z)
    glColor3f(0.94f, 0.90f, 0.82f);    // warm cream

    glBegin(GL_QUADS);

    // Left strip
    glVertex3f(-RX,     0.f,    z);
    glVertex3f(DOOR_X1, 0.f,    z);
    glVertex3f(DOOR_X1, RH,     z);
    glVertex3f(-RX,     RH,     z);

    // Right strip
    glVertex3f(DOOR_X2, 0.f,    z);
    glVertex3f( RX,     0.f,    z);
    glVertex3f( RX,     RH,     z);
    glVertex3f(DOOR_X2, RH,     z);

    // Transom above door
    glVertex3f(DOOR_X1, DOOR_H, z);
    glVertex3f(DOOR_X2, DOOR_H, z);
    glVertex3f(DOOR_X2, RH,     z);
    glVertex3f(DOOR_X1, RH,     z);

    glEnd();

    // --- Door frame ---
    // Three oak-coloured strips (top bar, left jamb, right jamb)
    // that protrude fd metres into the room.
    const float fw = 0.055f;          // frame bar thickness
    const float fd = 0.09f;           // frame depth into room (+Z from back wall)
    const float fz = z + fd * 0.5f;  // box centre Z (=  -2.25 + 0.045 = -2.205)
    const float dw = DOOR_X2 - DOOR_X1;   // door width = 0.90 m
    const float dx = (DOOR_X1 + DOOR_X2) * 0.5f;  // door centre X = -1.35

    glColor3f(0.58f, 0.44f, 0.28f);   // dark oak

    drawBox(dx,              DOOR_H,       fz, dw + fw * 2.f, fw,            fd);  // top bar
    drawBox(DOOR_X1 - fw * 0.5f, 0.f,    fz, fw,            DOOR_H + fw,   fd);  // left jamb
    drawBox(DOOR_X2 + fw * 0.5f, 0.f,    fz, fw,            DOOR_H + fw,   fd);  // right jamb

    // --- Door panel (closed, slightly inside the opening) ---
    const float pw = dw - 0.025f;     // panel width (slightly less than opening)
    const float pd = 0.04f;           // panel thickness
    const float pz = z + pd * 0.5f + 0.006f;  // centre Z of panel

    glColor3f(0.82f, 0.68f, 0.50f);   // mid oak
    drawBox(dx, 0.f, pz, pw, DOOR_H - 0.012f, pd);

    // Raised panel details — two rectangles on the door face for realism
    const float detailZ = pz + pd * 0.5f + 0.004f;   // in front of panel face
    glColor3f(0.70f, 0.56f, 0.38f);   // slightly darker oak rim

    drawBox(dx, 1.08f, detailZ, pw * 0.74f, 0.72f, 0.012f);   // upper panel
    drawBox(dx, 0.22f, detailZ, pw * 0.74f, 0.70f, 0.012f);   // lower panel

    // Door handle — small brass-tone knob on the right side of the panel
    glColor3f(0.80f, 0.75f, 0.55f);
    drawBox(DOOR_X2 - 0.14f, 0.98f, pz + pd * 0.5f + 0.022f,
            0.038f, 0.038f, 0.038f);
}

// ─── Left wall (X = -RX) — sage-green accent wall with window ────────────────

static void drawLeftWall()
{
    // Window opening: Z ∈ [WIN_Z1, WIN_Z2], Y ∈ [WIN_Y1, WIN_Y2]
    // Wall drawn in four sections around the opening:
    //   1. Back  strip  (Z: -RZ → WIN_Z1), full height
    //   2. Front strip  (Z: WIN_Z2 → +RZ), full height
    //   3. Below window (Z: WIN_Z1 → WIN_Z2, Y: 0 → WIN_Y1)  — sill zone
    //   4. Above window (Z: WIN_Z1 → WIN_Z2, Y: WIN_Y2 → RH) — lintel zone

    const float x = -RX;
    glNormal3f(1.f, 0.f, 0.f);         // inward normal (+X)
    glColor3f(0.60f, 0.72f, 0.62f);    // sage green

    glBegin(GL_QUADS);

    // Back strip
    glVertex3f(x, 0.f,    -RZ);
    glVertex3f(x, 0.f,    WIN_Z1);
    glVertex3f(x, RH,     WIN_Z1);
    glVertex3f(x, RH,     -RZ);

    // Front strip
    glVertex3f(x, 0.f,    WIN_Z2);
    glVertex3f(x, 0.f,     RZ);
    glVertex3f(x, RH,      RZ);
    glVertex3f(x, RH,     WIN_Z2);

    // Below window (sill zone)
    glVertex3f(x, 0.f,    WIN_Z1);
    glVertex3f(x, 0.f,    WIN_Z2);
    glVertex3f(x, WIN_Y1, WIN_Z2);
    glVertex3f(x, WIN_Y1, WIN_Z1);

    // Above window (lintel zone)
    glVertex3f(x, WIN_Y2, WIN_Z1);
    glVertex3f(x, WIN_Y2, WIN_Z2);
    glVertex3f(x, RH,     WIN_Z2);
    glVertex3f(x, RH,     WIN_Z1);

    glEnd();

    // --- Window frame ---
    // Thin bars around the opening, protruding fd metres into the room (+X).
    const float fw  = 0.045f;               // frame bar thickness
    const float fd  = 0.09f;                // depth protrusion into room
    const float fcx = x + fd * 0.5f;        // box centre X = -1.955
    const float ww  = WIN_Z2 - WIN_Z1;      // window width  = 1.40 m
    const float wh  = WIN_Y2 - WIN_Y1;      // window height = 1.00 m
    const float wz  = (WIN_Z1 + WIN_Z2) * 0.5f;   // window centre Z = -0.50

    glColor3f(0.88f, 0.85f, 0.80f);    // light grey-white frame

    // Outer frame bars
    drawBox(fcx, WIN_Y2 - fw, wz, fd, fw, ww);             // top rail
    drawBox(fcx, WIN_Y1,      wz, fd, fw, ww);             // bottom sill bar
    drawBox(fcx, WIN_Y1, WIN_Z1 + fw * 0.5f, fd, wh, fw); // left stile
    drawBox(fcx, WIN_Y1, WIN_Z2 - fw * 0.5f, fd, wh, fw); // right stile

    // Cross dividers (give the window a 4-pane appearance)
    const float midY = (WIN_Y1 + WIN_Y2) * 0.5f;
    drawBox(fcx, midY - fw * 0.5f, wz,           fd, fw, ww); // horizontal mid-rail
    drawBox(fcx, WIN_Y1,           wz,            fd, wh, fw); // vertical mid-stile

    // Outer sill ledge — slightly thicker slab protruding below the window
    glColor3f(0.84f, 0.81f, 0.76f);
    drawBox(fcx, WIN_Y1 - 0.04f, wz, fd * 1.6f, 0.04f, ww + 0.08f);

    // --- Window glass — semi-transparent sky-blue pane ---
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);   // don't occlude geometry behind the glass

    glNormal3f(1.f, 0.f, 0.f);
    glColor4f(0.68f, 0.84f, 0.94f, 0.42f);

    const float gx = x + 0.055f;   // glass sits 5.5 cm inside the wall
    glBegin(GL_QUADS);
        glVertex3f(gx, WIN_Y1 + fw, WIN_Z1 + fw);
        glVertex3f(gx, WIN_Y1 + fw, WIN_Z2 - fw);
        glVertex3f(gx, WIN_Y2 - fw, WIN_Z2 - fw);
        glVertex3f(gx, WIN_Y2 - fw, WIN_Z1 + fw);
    glEnd();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // --- Curtain rod (thin cylindrical bar approximated as a box) ---
    glColor3f(0.68f, 0.62f, 0.50f);
    drawBox(x + 0.045f, WIN_Y2 + 0.045f, wz,
            0.030f, 0.030f, ww + 0.64f);   // rod extends 0.32 m beyond each side

    // --- Curtain panels (two sage-green fabric panels, one on each side) ---
    // Hang from the rod down to the floor, sitting to the side of the window.
    glColor3f(0.52f, 0.66f, 0.55f);   // slightly darker sage than the wall
    const float cW = 0.22f;           // each panel width in Z
    const float cH = WIN_Y2 + 0.05f;  // panel height (floor to just above window top)
    const float cX = x + 0.035f;      // panel sits close to the wall face

    drawBox(cX, 0.f, WIN_Z1 - cW * 0.5f, 0.048f, cH, cW);   // back (left)  panel
    drawBox(cX, 0.f, WIN_Z2 + cW * 0.5f, 0.048f, cH, cW);   // front (right) panel
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

void drawRoom()
{
    drawFloor();
    drawCeiling();
    drawBackWall();
    drawLeftWall();
    drawRightWall();
    // Front wall (Z = +RZ) is intentionally omitted for camera visibility.
    // Person 2: if a skybox or exterior is added, the front wall can be drawn here.
}
