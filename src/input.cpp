// input.cpp — Camera modes, navigation, and on-screen controls.
#include <GL/freeglut.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include "furniture.h"
#include "input.h"
#include "room.h"

namespace {
constexpr float PI = 3.14159265358979323846f;
CameraMode mode = CameraMode::Overview;
float orbitYaw = -5.0f;
float orbitPitch = 13.0f;
float orbitDistance = 8.2f;
float interiorDistance = 5.4f;
float firstX = 0.0f, firstY = 1.65f, firstZ = 1.75f;
float firstYaw = 0.0f, firstPitch = -5.0f;
bool dragging = false;
bool haveMouse = false;
bool leftPressed = false;
bool controlAtPress = false;
CameraMode modeAtPress = CameraMode::Overview;
int pressX = 0, pressY = 0;
int lastMouseX = 0, lastMouseY = 0;

float radians(float degrees) { return degrees * PI / 180.0f; }

void zoom(int direction)
{
    if (mode == CameraMode::Overview)
        orbitDistance = std::clamp(orbitDistance - direction * 0.55f, 4.6f, 14.0f);
    else if (mode == CameraMode::Interior)
        interiorDistance = std::clamp(interiorDistance - direction * 0.35f, 3.8f, 7.0f);
    glutPostRedisplay();
}

void moveFirstPerson(unsigned char key)
{
    const float step = 0.16f;
    const float yaw = radians(firstYaw);
    const float forwardX = std::sin(yaw), forwardZ = -std::cos(yaw);
    const float rightX = std::cos(yaw), rightZ = std::sin(yaw);
    float dx = 0.0f, dz = 0.0f;
    switch (key) {
    case 'w': case 'W': dx = forwardX; dz = forwardZ; break;
    case 's': case 'S': dx = -forwardX; dz = -forwardZ; break;
    case 'a': case 'A': dx = -rightX; dz = -rightZ; break;
    case 'd': case 'D': dx = rightX; dz = rightZ; break;
    default: return;
    }
    firstX = std::clamp(firstX + dx * step, -1.84f, 1.84f);
    firstZ = std::clamp(firstZ + dz * step, -2.08f, 2.08f);
}

const char* modeName()
{
    switch (mode) {
    case CameraMode::Overview: return "Overview";
    case CameraMode::TopDown: return "Top-down";
    case CameraMode::Interior: return "Closer interior";
    case CameraMode::FirstPerson: return "First-person";
    }
    return "Overview";
}

void bitmapText(int x, int y, const char* text)
{
    glRasterPos2i(x, y);
    while (*text) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *text++);
}
}

CameraMode currentCameraMode() { return mode; }

bool selectFurnitureAtScreen(int x, int y, int width, int height)
{
    if (width <= 0 || height <= 0) {
        selectFurnitureById(-1);
        return false;
    }

    // Render the exact transformed furniture into unique flat colors. The room
    // writes depth only, so the pixel read identifies the visible surface at
    // this exact viewport coordinate, including overlap and wall occlusion.
    GLdouble projection[16];
    GLint viewport[4];
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDrawBuffer(GL_BACK);
    glReadBuffer(GL_BACK);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixd(projection);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    applyCamera();

    glDisable(GL_LIGHTING);
    glDisable(GL_BLEND);
    glDisable(GL_DITHER);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    const RoomOpenings openings = editableRoomOpenings();
    drawRoom(mode != CameraMode::TopDown, &openings);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    drawFurniture(true);
    glFinish();

    GLubyte pixel[3] = {0, 0, 0};
    const GLint readX = viewport[0] + x;
    const GLint readY = viewport[1] + viewport[3] - 1 - y;
    if (readX >= viewport[0] && readX < viewport[0] + viewport[2] &&
        readY >= viewport[1] && readY < viewport[1] + viewport[3])
        glReadPixels(readX, readY, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();

    int objectId = -1;
    if (pixel[0] == 255 && pixel[1] == 0 && pixel[2] == 0) objectId = 0;
    else if (pixel[0] == 0 && pixel[1] == 255 && pixel[2] == 0) objectId = 1;
    else if (pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 255) objectId = 2;
    else if (pixel[0] == 255 && pixel[1] == 255 && pixel[2] == 0) objectId = 3;
    else if (pixel[0] == 255 && pixel[1] == 0 && pixel[2] == 255) objectId = 4;
    else if (pixel[0] == 0 && pixel[1] == 255 && pixel[2] == 255) objectId = 5;
    else if (pixel[0] == 255 && pixel[1] == 255 && pixel[2] == 255) objectId = 6;
    else if (pixel[0] == 255 && pixel[1] == 128 && pixel[2] == 0) objectId = 7;
    selectFurnitureById(objectId);
    return objectId >= 0;
}

void applyCamera()
{
    switch (mode) {
    case CameraMode::Overview: {
        const float yaw = radians(orbitYaw), pitch = radians(orbitPitch);
        const float horizontal = orbitDistance * std::cos(pitch);
        const float eyeX = std::sin(yaw) * horizontal;
        const float eyeY = 0.85f + std::sin(pitch) * orbitDistance;
        const float eyeZ = std::cos(yaw) * horizontal;
        gluLookAt(eyeX, eyeY, eyeZ, 0.0, 0.85, 0.0, 0.0, 1.0, 0.0);
        break;
    }
    case CameraMode::TopDown:
        // The room ceiling is hidden in this mode so the furniture remains visible.
        gluLookAt(0.0, 8.0, 0.01, 0.0, 0.0, 0.0, 0.0, 0.0, -1.0);
        break;
    case CameraMode::Interior:
        gluLookAt(0.0, 2.05, interiorDistance, 0.0, 0.85, -0.45, 0.0, 1.0, 0.0);
        break;
    case CameraMode::FirstPerson: {
        const float yaw = radians(firstYaw), pitch = radians(firstPitch);
        const float dx = std::sin(yaw) * std::cos(pitch);
        const float dy = std::sin(pitch);
        const float dz = -std::cos(yaw) * std::cos(pitch);
        gluLookAt(firstX, firstY, firstZ,
                  firstX + dx, firstY + dy, firstZ + dz,
                  0.0, 1.0, 0.0);
        break;
    }
    }
}

void drawCameraHud(int width, int height)
{
    if (width < 1 || height < 1) return;
    const char* cameraControls = mode == CameraMode::Overview
        ? "Drag: orbit   Click: select   Wheel: zoom   1-4: views"
        : mode == CameraMode::FirstPerson
            ? "WASD: move   Mouse: look   Ctrl+click: select   1-4: views"
            : "Click: select   Wheel: zoom (interior)   1-4: views";

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, width, 0, height);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    const GLboolean hadLighting = glIsEnabled(GL_LIGHTING);
    const GLboolean hadBlend = glIsEnabled(GL_BLEND);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.025f, 0.04f, 0.08f, 0.78f);
    glBegin(GL_QUADS);
        glVertex2i(12, height - 116); glVertex2i(830, height - 116);
        glVertex2i(830, height - 10); glVertex2i(12, height - 10);
    glEnd();
    glColor3f(1.0f, 0.96f, 0.86f);
    std::string title = "Camera: ";
    title += modeName();
    title += "   Edit: ";
    title += (selectedFurnitureId() < 0) ? "OFF" : "ON";
    title += "   Selected: ";
    title += selectedFurnitureName();
    char status[48];
    if (selectedFurnitureId() < 0)
        std::snprintf(status, sizeof(status), "Scale: --");
    else if (selectedFurnitureId() == 5 || selectedFurnitureId() == 6)
        std::snprintf(status, sizeof(status), "W x%.2f  H x%.2f",
                      selectedFurnitureScale(), selectedFurnitureScaleY());
    else
        std::snprintf(status, sizeof(status), "Scale: %.2fx", selectedFurnitureScale());
    title += "   ";
    title += status;
    bitmapText(22, height - 28, title.c_str());
    bitmapText(22, height - 51, cameraControls);
    const int selectedId = selectedFurnitureId();
    const char* editControls = selectedId == 7
        ? "Left/Right: move along wall   Up/Down: height   =/+ grow   - shrink"
        : selectedId == 5
        ? "Left/Right: move   Q/E: swing   =/+ grow, - shrink width   [ / ]: height"
        : selectedId == 6
            ? "Arrows: move on wall   =/+ grow, - shrink width   [ / ]: height"
            : "Arrows: move   Q/E: rotate   =/+ grow   - shrink";
    bitmapText(22, height - 74, editControls);
    bitmapText(22, height - 97, "R: reset selected   Shift+R: reset all   Esc: quit");
    if (hadBlend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (hadLighting) glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void handleCameraKey(unsigned char key, int modifiers)
{
    if (key >= '1' && key <= '4') {
        mode = static_cast<CameraMode>(key - '1');
        dragging = false;
        haveMouse = false;
        if (mode == CameraMode::FirstPerson) glutSetCursor(GLUT_CURSOR_NONE);
        else glutSetCursor(GLUT_CURSOR_INHERIT);
    } else if ((key == 'r' || key == 'R') && (modifiers & GLUT_ACTIVE_SHIFT)) {
        resetAllFurniture();
    } else if (key == 'r' || key == 'R') {
        resetSelectedFurniture();
    } else if (key == 'q' || key == 'Q') {
        rotateSelectedFurniture(-15.0f);
    } else if (key == 'e' || key == 'E') {
        rotateSelectedFurniture(15.0f);
    } else if ((key == '[' || key == ']') &&
               (selectedFurnitureId() == 5 || selectedFurnitureId() == 6)) {
        scaleSelectedFurnitureY(key == '[' ? -0.05f : 0.05f);
    } else if (key == '[' || key == '-') {
        scaleSelectedFurniture(-0.05f);
    } else if (key == ']' || key == '=' || key == '+') {
        scaleSelectedFurniture(0.05f);
    } else if (mode == CameraMode::FirstPerson) {
        moveFirstPerson(key);
    }
    glutPostRedisplay();
}

void handleCameraMouse(int button, int state, int x, int y, int width, int height,
                       int modifiers)
{
    if (button != GLUT_LEFT_BUTTON) return;
    if (state == GLUT_DOWN) {
        leftPressed = true;
        controlAtPress = (modifiers & GLUT_ACTIVE_CTRL) != 0;
        modeAtPress = mode;
        pressX = lastMouseX = x;
        pressY = lastMouseY = y;
        dragging = mode == CameraMode::Overview;
        return;
    }
    if (state == GLUT_UP && leftPressed) {
        const bool click = std::abs(x - pressX) <= 4 && std::abs(y - pressY) <= 4;
        const bool selectionAllowed = modeAtPress != CameraMode::FirstPerson || controlAtPress;
        if (click && selectionAllowed && mode == modeAtPress)
            selectFurnitureAtScreen(x, y, width, height);
        leftPressed = false;
        dragging = false;
    }
}

void handleCameraMotion(int x, int y)
{
    if (mode == CameraMode::Overview && dragging) {
        orbitYaw += (x - lastMouseX) * 0.35f;
        orbitPitch = std::clamp(orbitPitch + (lastMouseY - y) * 0.25f, 4.0f, 48.0f);
        lastMouseX = x;
        lastMouseY = y;
        glutPostRedisplay();
    } else if (mode == CameraMode::FirstPerson) {
        // Ctrl+click is reserved for selection; do not let its pointer motion
        // also change the view while the user is trying to target an object.
        if (leftPressed && controlAtPress) {
            lastMouseX = x;
            lastMouseY = y;
            return;
        }
        if (haveMouse) {
            firstYaw += (x - lastMouseX) * 0.22f;
            firstPitch = std::clamp(firstPitch + (lastMouseY - y) * 0.22f, -70.0f, 70.0f);
            glutPostRedisplay();
        }
        lastMouseX = x;
        lastMouseY = y;
        haveMouse = true;
    }
}

void handleCameraWheel(int /*wheel*/, int direction, int /*x*/, int /*y*/)
{
    zoom(direction);
}

void handleCameraSpecial(int key)
{
    switch (key) {
    case GLUT_KEY_LEFT:  moveSelectedFurniture(-0.10f, 0.f); break;
    case GLUT_KEY_RIGHT: moveSelectedFurniture( 0.10f, 0.f); break;
    case GLUT_KEY_UP:    moveSelectedFurniture(0.f, -0.10f); break;
    case GLUT_KEY_DOWN:  moveSelectedFurniture(0.f,  0.10f); break;
    default: break;
    }
}
