// main.cpp — Interactive 3D Hostel Room Planner
#include <GL/freeglut.h>
#include <cstdlib>
#include "room.h"
#include "furniture.h"
#include "input.h"

static int windowWidth = 1200;
static int windowHeight = 800;

static void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    applyCamera();
    const RoomOpenings openings = editableRoomOpenings();
    drawRoom(currentCameraMode() != CameraMode::TopDown, &openings);
    drawFurniture();
    drawCameraHud(windowWidth, windowHeight);
    glutSwapBuffers();
}

static void reshape(int w, int h)
{
    if (h == 0) h = 1;
    windowWidth = w;
    windowHeight = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(55.0, static_cast<double>(w) / h, 0.1, 50.0);
    glMatrixMode(GL_MODELVIEW);
}

static void keyboard(unsigned char key, int, int)
{
    if (key == 27) std::exit(0);
    handleCameraKey(key, glutGetModifiers());
}

static void mouse(int button, int state, int x, int y)
{
    handleCameraMouse(button, state, x, y, windowWidth, windowHeight, glutGetModifiers());
}

static void motion(int x, int y)
{
    handleCameraMotion(x, y);
}

static void wheel(int wheelNumber, int direction, int x, int y)
{
    handleCameraWheel(wheelNumber, direction, x, y);
}

static void special(int key, int, int)
{
    handleCameraSpecial(key);
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Interactive 3D Hostel Room Planner");

    // Sky visible through the intentionally open front wall.
    glClearColor(0.53f, 0.75f, 0.87f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutPassiveMotionFunc(motion);
    glutMouseWheelFunc(wheel);
    glutSpecialFunc(special);

    glutMainLoop();
    return 0;
}
