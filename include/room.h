#pragma once
// room.h — Room geometry declarations.
//
// drawBox is the shared primitive used by both room.cpp and furniture.cpp.
// All coordinates use 1 OpenGL unit = 1 metre.
//   Room: X in [-2.0, +2.0], Y in [0.0, 2.8], Z in [-2.25, +2.25]

// ---------------------------------------------------------------------------
// drawBox — draws a solid, colour-filled, axis-aligned box.
//
//   cx, cz : horizontal centre of the box (X and Z)
//   cy     : bottom (base) Y face of the box — i.e. cy is the floor level
//   w      : width  in metres (X direction)
//   h      : height in metres (Y direction)
//   d      : depth  in metres (Z direction)
//
// Set glColor3f / glColor4f BEFORE calling.
// Normals are emitted per face so GL_LIGHTING (Person 2) works without change.
// ---------------------------------------------------------------------------
void drawBox(float cx, float cy, float cz,
             float w,  float h,  float d);

// ---------------------------------------------------------------------------
// drawRoom — draws the complete room shell:
//   floor, ceiling, back wall (with door), left wall (with window), right wall.
//   The front wall (Z = +2.25) is intentionally omitted for camera visibility.
// ---------------------------------------------------------------------------
void drawRoom();
