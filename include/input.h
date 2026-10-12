#pragma once

// Camera controls shared by the GLUT callbacks in main.cpp.
enum class CameraMode { Overview, TopDown, Interior, FirstPerson };

CameraMode currentCameraMode();
void applyCamera();
void drawCameraHud(int width, int height);
void handleCameraKey(unsigned char key, int modifiers = 0);
void handleCameraMouse(int button, int state, int x, int y, int width, int height,
                       int modifiers = 0);
void handleCameraMotion(int x, int y);
void handleCameraWheel(int wheel, int direction, int x, int y);
void handleCameraSpecial(int key);
bool selectFurnitureAtScreen(int x, int y, int width, int height);
