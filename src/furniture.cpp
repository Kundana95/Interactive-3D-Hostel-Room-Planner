// furniture.cpp — Fixed-function furniture and bedroom decor.
#include <GL/freeglut.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <initializer_list>
#include <limits>
#include "furniture.h"
#include "room.h"

namespace {
const float oak[3]       = {0.62f, 0.42f, 0.25f};
const float oakLight[3]  = {0.76f, 0.58f, 0.37f};
const float oakDark[3]   = {0.39f, 0.25f, 0.15f};
const float white[3]     = {0.94f, 0.92f, 0.87f};
const float navy[3]      = {0.055f, 0.10f, 0.24f};
const float navyLight[3] = {0.12f, 0.20f, 0.37f};
const float gold[3]      = {0.88f, 0.67f, 0.30f};
const float metal[3]     = {0.73f, 0.67f, 0.53f};

struct FurnitureSpec {
    const char* name;
    float pivotX, pivotY, pivotZ;
    float minX, maxX, minZ, maxZ, maxY; // bounds relative to the pivot
};
struct FurnitureTransform { float x, y, z, angle, scale, scaleY; };
struct Bounds2 { float minX, maxX, minZ, maxZ; };

const std::array<FurnitureSpec, 8> specs = {{
    {"Bed",       -1.35f, 0.f,  1.05f, -0.65f, 0.575f, -0.96f, 0.96f, 0.91f},
    {"Study desk", 0.50f, 0.f, -1.82f, -0.67f, 0.67f,  -0.30f, 0.30f, 1.28f},
    {"Chair",      0.50f, 0.f, -1.15f, -0.21f, 0.21f,  -0.21f, 0.21f, 1.03f},
    {"Wardrobe",   1.66f, 0.f, -1.63f, -0.40f, 0.33f,  -0.49f, 0.49f, 1.83f},
    {"Floor mat",  0.40f, 0.f,  0.35f, -0.65f, 0.65f,  -0.625f, 0.625f, 0.04f},
    {"Door",      -1.35f, 0.f, -2.25f, -0.45f, 0.45f, -0.03f, 0.03f, 2.00f},
    {"Window",     0.50f, 1.f, -2.25f, -0.70f, 0.70f, -0.03f, 0.03f, 2.00f},
    {"Bookshelf", -1.965f,2.03f, 1.20f, 0.f, 0.30f, -0.52f, 0.52f, 0.72f}
}};
const std::array<FurnitureTransform, 8> initialTransforms = {{
    {-1.35f,0.f,  1.05f, 0.f, 1.f, 1.f},
    { 0.50f,0.f, -1.82f, 0.f, 1.f, 1.f},
    { 0.50f,0.f, -1.15f, 0.f, 1.f, 1.f},
    { 1.66f,0.f, -1.63f, 0.f, 1.f, 1.f},
    { 0.40f,0.f,  0.35f, 0.f, 1.f, 1.f},
    {-1.35f,0.f,  0.f, 0.f, 1.f, 1.f},
    { 0.50f,1.f,  0.f, 0.f, 1.f, 1.f},
    {-1.965f,2.03f,1.20f,0.f,1.f,1.f}
}};
std::array<FurnitureTransform, 8> transforms = initialTransforms;
int selectedIndex = -1;
bool pickingDraw = false;
int pickColourIndex = -1;

int indexOf(FurnitureId id) { return static_cast<int>(id); }

Bounds2 worldBounds(int index, const FurnitureTransform& t)
{
    const FurnitureSpec& s = specs[index];
    const float a = t.angle * 3.14159265358979323846f / 180.f;
    const float c = std::cos(a) * t.scale, sn = std::sin(a) * t.scale;
    Bounds2 b{std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(),
              std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()};
    for (float x : {s.minX, s.maxX}) for (float z : {s.minZ, s.maxZ}) {
        const float wx = t.x + c * x + sn * z;
        const float wz = t.z - sn * x + c * z;
        b.minX = std::min(b.minX, wx); b.maxX = std::max(b.maxX, wx);
        b.minZ = std::min(b.minZ, wz); b.maxZ = std::max(b.maxZ, wz);
    }
    return b;
}

// Rotation and growth can push an object through a wall even when its pivot
// remains in the room. Nudge it back just enough to keep its full bounds inside.
bool fitInsideRoom(int index, FurnitureTransform& t)
{
    constexpr float roomMinX = -2.0f, roomMaxX = 2.0f;
    constexpr float roomMinZ = -2.25f, roomMaxZ = 2.25f;
    const Bounds2 b = worldBounds(index, t);
    if (b.maxX - b.minX > roomMaxX - roomMinX ||
        b.maxZ - b.minZ > roomMaxZ - roomMinZ) return false;
    if (b.minX < roomMinX) t.x += roomMinX - b.minX;
    if (b.maxX > roomMaxX) t.x -= b.maxX - roomMaxX;
    if (b.minZ < roomMinZ) t.z += roomMinZ - b.minZ;
    if (b.maxZ > roomMaxZ) t.z -= b.maxZ - roomMaxZ;
    return true;
}

bool overlaps(const Bounds2& a, const Bounds2& b)
{
    constexpr float clearance = 0.005f;
    return a.minX < b.maxX - clearance && a.maxX > b.minX + clearance &&
           a.minZ < b.maxZ - clearance && a.maxZ > b.minZ + clearance;
}

bool validTransform(int index, const FurnitureTransform& candidate)
{
    if (index == 5 || index == 6) {
        const float halfExtent = (index == 5 ? 0.45f : 0.70f) * candidate.scale;
        const float extra = index == 5 ? 0.055f : 0.36f; // frame / curtain rod
        const float left = candidate.x - halfExtent;
        const float right = candidate.x + halfExtent;
        if (left - extra < -1.94f || right + extra > 1.94f) return false;
        if (index == 5) {
            const float width = 0.90f * candidate.scale;
            const float height = 2.00f * candidate.scaleY;
            if (width < 0.55f || width > 1.30f || height < 1.40f || height > 2.45f)
                return false;
            const float windowLeft = transforms[6].x - 0.70f * transforms[6].scale;
            const float windowRight = transforms[6].x + 0.70f * transforms[6].scale;
            // Reserve room for the frame/curtain and keep either fixture's
            // saved reset position available after the other is edited.
            const float resetWindowLeft = 0.50f - 0.70f;
            const float resetWindowRight = 0.50f + 0.70f;
            const bool clearCurrent = right + 0.40f <= windowLeft || left - 0.40f >= windowRight;
            const bool clearReset = right + 0.40f <= resetWindowLeft ||
                                    left - 0.40f >= resetWindowRight;
            return clearCurrent && clearReset;
        }

        const float width = 1.40f * candidate.scale;
        const float height = 1.00f * candidate.scaleY;
        if (width < 0.80f || width > 2.00f || height < 0.65f || height > 1.55f ||
            candidate.y < 0.35f || candidate.y + height > 2.60f) return false;
        const float doorLeft = transforms[5].x - 0.45f * transforms[5].scale;
        const float doorRight = transforms[5].x + 0.45f * transforms[5].scale;
        const bool clearCurrent = right + 0.40f <= doorLeft || left - 0.40f >= doorRight;
        const bool clearReset = right + 0.40f <= -1.35f - 0.45f ||
                                left - 0.40f >= -1.35f + 0.45f;
        return clearCurrent && clearReset;
    }

    if (index == 7) {
        const Bounds2 b = worldBounds(index, candidate);
        return candidate.x == specs[7].pivotX &&
               b.minX >= -2.005f && b.maxX <= -1.60f &&
               b.minZ >= -2.255f && b.maxZ <= 2.255f &&
               candidate.y >= 1.45f && candidate.y + specs[7].maxY * candidate.scale <= 2.80f &&
               candidate.scale >= 0.65f && candidate.scale <= 1.10f;
    }

    const Bounds2 b = worldBounds(index, candidate);
    // A few millimetres of tolerance accommodate the bed headboard touching the wall.
    if (b.minX < -2.005f || b.maxX > 2.005f ||
        b.minZ < -2.255f || b.maxZ > 2.255f) return false;

    // Keep the entry lane open for furniture; a thin floor mat may cross it.
    if (index < 5 && index != indexOf(FurnitureId::Rug)) {
        const float doorHalfWidth = 0.45f * transforms[5].scale + 0.05f;
        const Bounds2 entry{transforms[5].x - doorHalfWidth,
                            transforms[5].x + doorHalfWidth, -2.255f, -1.15f};
        if (overlaps(b, entry)) return false;
    }

    for (int other = 0; other < 5; ++other)
        if (other != index && index != indexOf(FurnitureId::Rug) &&
            other != indexOf(FurnitureId::Rug) && overlaps(b, worldBounds(other, transforms[other])))
            return false;
    return true;
}

void setPickColour(int index)
{
    switch (index) {
    case 0: glColor3ub(255, 0, 0); break;       // bed
    case 1: glColor3ub(0, 255, 0); break;       // desk
    case 2: glColor3ub(0, 0, 255); break;       // chair
    case 3: glColor3ub(255, 255, 0); break;     // wardrobe
    case 4: glColor3ub(255, 0, 255); break;     // floor mat
    case 5: glColor3ub(0, 255, 255); break;     // door
    case 6: glColor3ub(255, 255, 255); break;   // window
    case 7: glColor3ub(255, 128, 0); break;     // bookshelf
    default: glColor3ub(0, 0, 0); break;
    }
}
void colour(const float c[3])
{
    if (pickingDraw) setPickColour(pickColourIndex);
    else glColor3f(c[0], c[1], c[2]);
}
void colour(float r, float g, float b)
{
    if (pickingDraw) setPickColour(pickColourIndex);
    else glColor3f(r, g, b);
}

void sphere(float x, float y, float z, float radius)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glutSolidSphere(radius, 10, 8);
    glPopMatrix();
}

void cone(float x, float y, float z, float radius, float height)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glutSolidCone(radius, height, 8, 5);
    glPopMatrix();
}

void bed()
{
    // The long side runs parallel to the navy wall (along Z); the headboard
    // is flush against its X=-2.0 m plane.
    // The bed frame now sits 7.5 cm off the accent wall and is fully inside
    // the room. Keep the headboard against the wall, overlapping the frame edge.
    const float x = -1.35f, z = 1.05f, length = 1.92f, width = 1.15f;
    colour(oakDark);
    drawBox(x, 0.08f, z, width, 0.16f, length);
    for (float px : {-0.47f, 0.47f})
        for (float pz : {-0.84f, 0.84f})
            drawBox(x + px, 0.f, z + pz, 0.10f, 0.10f, 0.10f);
    colour(oakLight);
    drawBox(x, 0.24f, z, width - 0.06f, 0.10f, length - 0.06f);
    colour(white);
    drawBox(x, 0.34f, z, width - 0.10f, 0.23f, length - 0.10f);

    // Navy blanket covers the foot half (+Z), with layered pillows at the head.
    colour(navy);
    drawBox(x, 0.47f, z + 0.38f, width - 0.14f, 0.045f, 1.12f);
    colour(white);
    drawBox(x, 0.47f, z - 0.60f, 0.78f, 0.09f, 0.42f);
    colour(white);
    drawBox(x - 0.20f, 0.49f, z - 0.57f, 0.30f, 0.08f, 0.30f);
    drawBox(x + 0.20f, 0.49f, z - 0.57f, 0.30f, 0.08f, 0.30f);

    colour(oak);
    // Outer headboard face is exactly on the accent-wall plane X = -2.0 m.
    drawBox(-1.965f, 0.24f, z,
            0.07f, 0.66f, width + 0.08f);
}

void desk()
{
    // Desk is directly under the back-wall window, facing it along -Z.
    const float x = 0.50f, z = -1.82f;
    const float topY = 0.76f, width = 1.34f, depth = 0.60f;
    colour(oakLight);
    drawBox(x, topY, z, width, 0.07f, depth);
    colour(oak);
    const float legX = width * 0.5f - 0.07f;
    const float legZ = depth * 0.5f - 0.055f;
    for (float sx : {-1.f, 1.f})
        for (float sz : {-1.f, 1.f})
            drawBox(x + sx * legX, 0.f, z + sz * legZ, 0.075f, topY, 0.075f);
    colour(oakDark);
    drawBox(x, 0.57f, z + 0.16f, 0.72f, 0.18f, 0.24f);
    colour(metal);
    drawBox(x, 0.64f, z + 0.29f, 0.045f, 0.018f, 0.025f);

    // A small closed book and a potted plant on the desktop.
    colour(navyLight);
    drawBox(0.12f, 0.835f, -1.72f, 0.30f, 0.035f, 0.24f);
    colour(white);
    drawBox(0.12f, 0.872f, -1.72f, 0.28f, 0.025f, 0.22f);
    colour(oakDark);
    drawBox(0.92f, 0.835f, -1.72f, 0.16f, 0.12f, 0.16f); // planter
    colour(0.20f, 0.42f, 0.25f);
    cone(0.92f, 0.955f, -1.72f, 0.13f, 0.25f);
    sphere(0.92f, 1.12f, -1.72f, 0.075f);
}

void chair()
{
    // In front of the desk, facing it toward -Z; backrest is on the +Z side.
    // Centered on the desk; its front edge remains clear of the desk legs and
    // drawer while the backrest stays on the seated user's +Z side.
    const float x = 0.50f, z = -1.15f, seatY = 0.45f;
    const float w = 0.42f, d = 0.42f;
    colour(oak);
    drawBox(x, seatY, z, w, 0.08f, d);
    for (float sx : {-1.f, 1.f})
        for (float sz : {-1.f, 1.f})
            drawBox(x + sx * (w * 0.5f - 0.045f), 0.f,
                    z + sz * (d * 0.5f - 0.035f), 0.055f, seatY, 0.055f);
    drawBox(x, seatY + 0.08f, z + d * 0.5f - 0.035f,
            w, 0.50f, 0.07f);
}

void wardrobe()
{
    // Wardrobe is backed to the right wall, near but clear of the entrance.
    const float x = 1.66f, z = -1.63f, w = 0.66f, d = 0.98f;
    colour(oakDark);
    drawBox(x, 0.f, z, w, 0.10f, d);
    colour(oak);
    drawBox(x, 0.10f, z, w, 1.72f, d);
    const float frontX = x - w * 0.5f - 0.012f;
    colour(oakLight);
    drawBox(frontX, 0.18f, z - 0.235f, 0.025f, 1.56f, 0.43f);
    drawBox(frontX, 0.18f, z + 0.235f, 0.025f, 1.56f, 0.43f);
    colour(oakDark);
    drawBox(frontX - 0.016f, 0.10f, z, 0.018f, 1.72f, 0.018f);
    colour(metal);
    drawBox(frontX - 0.032f, 0.94f, z - 0.045f, 0.025f, 0.12f, 0.025f);
    drawBox(frontX - 0.032f, 0.94f, z + 0.045f, 0.025f, 0.12f, 0.025f);
}

void rug()
{
    // Fully within the open central floor, clear of the entry route and bed.
    colour(0.78f, 0.72f, 0.62f);
    drawBox(0.40f, 0.008f, 0.35f, 1.30f, 0.012f, 1.25f);
    colour(0.68f, 0.61f, 0.52f);
    drawBox(0.40f, 0.021f, 0.35f, 1.20f, 0.006f, 1.15f);
    colour(0.82f, 0.77f, 0.68f);
    drawBox(0.40f, 0.028f, 0.35f, 1.10f, 0.006f, 1.05f);
}

void door()
{
    const FurnitureTransform& t = transforms[5];
    const float width = 0.90f * t.scale;
    const float height = 2.00f * t.scaleY;
    const float x1 = t.x - width * 0.5f, x2 = t.x + width * 0.5f;
    const float wallZ = -2.25f, frameZ = wallZ + 0.045f;
    const float frame = 0.055f, depth = 0.09f;

    // Jambs and lintel follow the live opening, so the wall gap and oak frame
    // stay attached as the door is repositioned or resized.
    colour(oakDark);
    drawBox(t.x, height, frameZ, width + 2.f * frame, frame, depth);
    drawBox(x1 - frame * 0.5f, 0.f, frameZ, frame, height + frame, depth);
    drawBox(x2 + frame * 0.5f, 0.f, frameZ, frame, height + frame, depth);

    // Swing around the left hinge, into the room (+Z). Width and height are
    // independently editable while the panel remains inside the doorway.
    glPushMatrix();
    glTranslatef(x1, 0.f, wallZ + 0.026f);
    glRotatef(-t.angle, 0.f, 1.f, 0.f);
    colour(oakLight);
    drawBox(width * 0.5f, 0.f, 0.f, width - 0.025f, height - 0.012f, 0.04f);
    colour(oakDark);
    drawBox(width * 0.5f, 1.08f * t.scaleY, 0.026f,
            width * 0.74f, 0.72f * t.scaleY, 0.012f);
    drawBox(width * 0.5f, 0.22f * t.scaleY, 0.026f,
            width * 0.74f, 0.70f * t.scaleY, 0.012f);
    colour(metal);
    drawBox(width - 0.14f * t.scale, 0.98f * t.scaleY, 0.048f,
            0.038f * t.scale, 0.038f * t.scaleY, 0.038f);
    glPopMatrix();

    if (!pickingDraw && selectedIndex == 5) {
        const float yTop = height + frame;
        glPushAttrib(GL_CURRENT_BIT | GL_LINE_BIT | GL_ENABLE_BIT);
        glDisable(GL_LIGHTING); glColor3f(1.f, 0.78f, 0.12f); glLineWidth(3.f);
        glBegin(GL_LINE_LOOP);
            glVertex3f(x1 - frame, 0.015f, frameZ + 0.01f);
            glVertex3f(x2 + frame, 0.015f, frameZ + 0.01f);
            glVertex3f(x2 + frame, yTop, frameZ + 0.01f);
            glVertex3f(x1 - frame, yTop, frameZ + 0.01f);
        glEnd();
        glPopAttrib();
    }
}

void window()
{
    const FurnitureTransform& t = transforms[6];
    const float width = 1.40f * t.scale;
    const float height = 1.00f * t.scaleY;
    const float sill = t.y;
    const float frame = 0.045f, depth = 0.09f;
    const float frameZ = -2.25f + depth * 0.5f;
    glPushMatrix();
    glTranslatef(t.x, sill, 0.f);
    colour(oak);
    drawBox(0.f, height - frame, frameZ, width + 2.f * frame, frame, depth);
    drawBox(0.f, 0.f, frameZ, width + 2.f * frame, frame, depth);
    drawBox(-width * 0.5f - frame * 0.5f, 0.f, frameZ,
            frame, height + frame, depth);
    drawBox(width * 0.5f + frame * 0.5f, 0.f, frameZ,
            frame, height + frame, depth);
    drawBox(0.f, height * 0.5f - frame * 0.5f, frameZ,
            width, frame, depth);
    drawBox(0.f, 0.f, frameZ, frame, height, depth);

    colour(oakLight);
    drawBox(0.f, -0.04f, -2.25f + 0.065f,
            width + 0.08f, 0.04f, 0.13f);

    // Translucent-looking blue pane; use an opaque ID only in the picking pass.
    if (pickingDraw) {
        setPickColour(pickColourIndex);
        drawBox(0.f, frame, -2.25f + 0.055f,
                width - 2.f * frame, height - 2.f * frame, 0.008f);
    } else {
        glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glColor4f(0.68f, 0.84f, 0.94f, 0.42f);
        drawBox(0.f, frame, -2.25f + 0.055f,
                width - 2.f * frame, height - 2.f * frame, 0.008f);
        glPopAttrib();
    }

    // Keep the curtains and rod coupled to the window's current width/height.
    colour(0.82f, 0.77f, 0.67f);
    drawBox(0.f, height + 0.045f, -2.25f + 0.055f,
            width + 0.64f, 0.03f, 0.03f);
    drawBox(-width * 0.5f - 0.11f, 0.f, -2.25f + 0.045f,
            0.22f, height + 0.05f, 0.05f);
    drawBox(width * 0.5f + 0.11f, 0.f, -2.25f + 0.045f,
            0.22f, height + 0.05f, 0.05f);

    if (!pickingDraw && selectedIndex == 6) {
        glPushAttrib(GL_CURRENT_BIT | GL_LINE_BIT | GL_ENABLE_BIT);
        glDisable(GL_LIGHTING); glColor3f(1.f, 0.78f, 0.12f); glLineWidth(3.f);
        glBegin(GL_LINE_LOOP);
            glVertex3f(-width * 0.5f - frame, -0.04f, frameZ + 0.012f);
            glVertex3f( width * 0.5f + frame, -0.04f, frameZ + 0.012f);
            glVertex3f( width * 0.5f + frame, height + frame, frameZ + 0.012f);
            glVertex3f(-width * 0.5f - frame, height + frame, frameZ + 0.012f);
        glEnd();
        glPopAttrib();
    }
    glPopMatrix();
}

void floatingShelves()
{
    // Two oak shelves mounted high on the accent wall above the headboard.
    colour(oakLight);
    drawBox(-1.84f, 2.03f, 1.20f, 0.25f, 0.065f, 0.94f);
    drawBox(-1.84f, 2.38f, 1.20f, 0.25f, 0.065f, 0.94f);

    // Upright books on the lower shelf.
    colour(navy);
    drawBox(-1.78f, 2.095f, 0.94f, 0.14f, 0.25f, 0.09f);
    colour(0.64f, 0.31f, 0.22f);
    drawBox(-1.78f, 2.095f, 1.06f, 0.14f, 0.25f, 0.09f);
    colour(0.86f, 0.72f, 0.48f);
    drawBox(-1.78f, 2.095f, 1.18f, 0.14f, 0.25f, 0.09f);

    // Tiny plant on the upper shelf.
    colour(oakDark);
    drawBox(-1.82f, 2.415f, 1.48f, 0.14f, 0.10f, 0.16f);
    colour(0.20f, 0.40f, 0.23f);
    cone(-1.82f, 2.515f, 1.48f, 0.12f, 0.22f);
    sphere(-1.82f, 2.66f, 1.48f, 0.065f);
}

void stringLights()
{
    // Warm bulbs and a gently sagging cord on the navy accent wall.
    glColor3f(0.28f, 0.22f, 0.15f);
    glLineWidth(2.f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= 20; ++i) {
        const float t = static_cast<float>(i) / 20.f;
        const float z = 0.25f + 1.75f * t;
        const float y = 2.53f - 0.13f * (1.f - (2.f * t - 1.f) * (2.f * t - 1.f));
        glVertex3f(-1.975f, y, z);
    }
    glEnd();
    for (int i = 0; i < 9; ++i) {
        const float t = static_cast<float>(i) / 8.f;
        const float z = 0.25f + 1.75f * t;
        const float y = 2.53f - 0.13f * (1.f - (2.f * t - 1.f) * (2.f * t - 1.f));
        colour(gold);
        sphere(-1.93f, y - 0.045f, z, 0.035f);
    }
    glLineWidth(1.f);
}

void drawSelectionOutline(int index)
{
    const FurnitureSpec& s = specs[index];
    const float x1 = s.pivotX + s.minX, x2 = s.pivotX + s.maxX;
    const float z1 = s.pivotZ + s.minZ, z2 = s.pivotZ + s.maxZ;
    const float y1 = index == 7 ? s.pivotY : 0.015f;
    const float y2 = index == 7 ? s.pivotY + s.maxY : s.maxY;
    const float v[8][3] = {
        {x1,y1,z1},{x2,y1,z1},{x2,y1,z2},{x1,y1,z2},
        {x1,y2,z1},{x2,y2,z1},{x2,y2,z2},{x1,y2,z2}
    };
    const int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},
        {0,4},{1,5},{2,6},{3,7}
    };
    glPushAttrib(GL_CURRENT_BIT | GL_LINE_BIT | GL_ENABLE_BIT);
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 0.78f, 0.12f);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    for (const auto& edge : edges) {
        glVertex3fv(v[edge[0]]);
        glVertex3fv(v[edge[1]]);
    }
    glEnd();
    glPopAttrib();
}

void drawEditable(int index, void (*drawObject)(), bool picking)
{
    if (index == 7) {
        const FurnitureSpec& s = specs[index];
        const FurnitureTransform& t = transforms[index];
        glPushMatrix();
        glTranslatef(t.x - s.pivotX, t.y - s.pivotY, t.z - s.pivotZ);
        glTranslatef(s.pivotX, s.pivotY, s.pivotZ);
        glRotatef(t.angle, 0.f, 1.f, 0.f);
        glScalef(t.scale, t.scale, t.scale);
        glTranslatef(-s.pivotX, -s.pivotY, -s.pivotZ);
        if (picking) {
            pickingDraw = true;
            pickColourIndex = index;
            setPickColour(index);
        }
        drawObject();
        if (!picking && selectedIndex == index) drawSelectionOutline(index);
        glPopMatrix();
        return;
    }
    if (index >= 5) {
        if (picking) {
            pickingDraw = true;
            pickColourIndex = index;
            setPickColour(index);
        }
        drawObject();
        return;
    }
    const FurnitureSpec& s = specs[index];
    const FurnitureTransform& t = transforms[index];
    glPushMatrix();
    glTranslatef(t.x - s.pivotX, t.y - s.pivotY, t.z - s.pivotZ);
    glTranslatef(s.pivotX, s.pivotY, s.pivotZ);
    glRotatef(t.angle, 0.f, 1.f, 0.f);
    glScalef(t.scale, t.scale, t.scale);
    glTranslatef(-s.pivotX, -s.pivotY, -s.pivotZ);
    if (picking) {
        pickingDraw = true;
        pickColourIndex = index;
        setPickColour(index);
    }
    drawObject();
    if (!picking && selectedIndex == index) drawSelectionOutline(index);
    glPopMatrix();
}
}

void selectFurnitureById(int id)
{
    selectedIndex = id >= 0 && id < static_cast<int>(specs.size()) ? id : -1;
    glutPostRedisplay();
}

const char* selectedFurnitureName()
{
    return selectedIndex >= 0 ? specs[selectedIndex].name : "None";
}

int selectedFurnitureId() { return selectedIndex; }

float selectedFurnitureScale()
{
    return selectedIndex >= 0 ? transforms[selectedIndex].scale : 1.0f;
}

float selectedFurnitureScaleY()
{
    return selectedIndex >= 0 ? transforms[selectedIndex].scaleY : 1.0f;
}

RoomOpenings editableRoomOpenings()
{
    return {transforms[5].x, 0.90f * transforms[5].scale,
            2.00f * transforms[5].scaleY,
            transforms[6].x, transforms[6].y,
            1.40f * transforms[6].scale, 1.00f * transforms[6].scaleY};
}

void moveSelectedFurniture(float dx, float dz)
{
    if (selectedIndex < 0) return;
    FurnitureTransform candidate = transforms[selectedIndex];
    if (selectedIndex == 7) {
        candidate.z += dx;   // left/right follow the wall's long axis
        candidate.y -= dz;   // up/down raise/lower the wall shelf
    } else {
        candidate.x += dx;
        if (selectedIndex == 6) candidate.y += dz;
        else if (selectedIndex < 5) candidate.z += dz;
    }
    if (validTransform(selectedIndex, candidate)) transforms[selectedIndex] = candidate;
    glutPostRedisplay();
}

void rotateSelectedFurniture(float degrees)
{
    if (selectedIndex < 0) return;
    FurnitureTransform candidate = transforms[selectedIndex];
    if (selectedIndex == 6 || selectedIndex == 7) return; // wall-mounted fixtures stay aligned
    if (selectedIndex == 5) {
        candidate.angle = std::clamp(candidate.angle + degrees, 0.f, 110.f);
        transforms[selectedIndex] = candidate;
        glutPostRedisplay();
        return;
    }
    candidate.angle = std::fmod(candidate.angle + degrees + 360.f, 360.f);
    if (fitInsideRoom(selectedIndex, candidate) && validTransform(selectedIndex, candidate))
        transforms[selectedIndex] = candidate;
    glutPostRedisplay();
}

void scaleSelectedFurniture(float amount)
{
    if (selectedIndex < 0) return;
    FurnitureTransform candidate = transforms[selectedIndex];
    candidate.scale = std::clamp(candidate.scale + amount, 0.65f, 1.45f);
    if (fitInsideRoom(selectedIndex, candidate) && validTransform(selectedIndex, candidate))
        transforms[selectedIndex] = candidate;
    glutPostRedisplay();
}

void scaleSelectedFurnitureY(float amount)
{
    if (selectedIndex != 5 && selectedIndex != 6) return;
    FurnitureTransform candidate = transforms[selectedIndex];
    candidate.scaleY += amount;
    if (validTransform(selectedIndex, candidate)) transforms[selectedIndex] = candidate;
    glutPostRedisplay();
}

void resetSelectedFurniture()
{
    if (selectedIndex < 0) return;
    if (validTransform(selectedIndex, initialTransforms[selectedIndex]))
        transforms[selectedIndex] = initialTransforms[selectedIndex];
    glutPostRedisplay();
}

void resetAllFurniture()
{
    transforms = initialTransforms;
    selectedIndex = -1;
    glutPostRedisplay();
}

void drawFurniture(bool picking)
{
    pickingDraw = picking;
    drawEditable(indexOf(FurnitureId::Rug), rug, picking);
    drawEditable(indexOf(FurnitureId::Bed), bed, picking);
    drawEditable(indexOf(FurnitureId::Desk), desk, picking);
    drawEditable(indexOf(FurnitureId::Chair), chair, picking);
    drawEditable(indexOf(FurnitureId::Wardrobe), wardrobe, picking);
    drawEditable(5, door, picking);
    drawEditable(6, window, picking);
    drawEditable(7, floatingShelves, picking);
    if (picking) {
        pickingDraw = false;
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        stringLights();
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    } else {
        stringLights();
    }
}
