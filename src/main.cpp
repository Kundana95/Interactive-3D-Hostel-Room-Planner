// main.cpp — Interactive 3D Hostel Room Planner
// Person 1, Milestone 1: room geometry + static overview camera.
//
// Build : mingw32-make
// Run   : build/hostel_planner.exe
//
// Controls (Milestone 1)
//   ESC — exit
//
// Milestones 2–3 will add furniture drawing and keyboard interaction.
// Person 2 will replace the overview camera with first-person navigation.

#include <GL/freeglut.h>
#include <cstdlib>
#include "room.h"

// ---------------------------------------------------------------------------
// display — clear, place camera, draw scene, swap buffers.
// ---------------------------------------------------------------------------
static void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Static overview camera.
    // Positioned to the upper-left-front of the room so that:
    //   • the left wall (sage-green, window) is visible on the left
    //   • the back wall (door) is visible straight ahead
    //   • the floor tile grid and ceiling are clearly shown
    //
    // TODO (Person 2): replace these three lines with a first-person
    //   gluLookAt driven by the player's position and yaw angle.
    gluLookAt(-0.5, 4.5, 6.8,   // eye — upper-left-front
               0.0, 1.2, 0.0,   // look-at — room centre at chest height
               0.0, 1.0, 0.0);  // up vector

    drawRoom();
    // Milestone 2: drawFurniture() call goes here.

    glutSwapBuffers();
}

// ---------------------------------------------------------------------------
// reshape — update viewport and perspective projection on window resize.
// ---------------------------------------------------------------------------
static void reshape(int w, int h)
{
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(55.0, (double)w / h, 0.1, 50.0);
    glMatrixMode(GL_MODELVIEW);
}

// ---------------------------------------------------------------------------
// keyboard — ASCII key handler.
// Milestone 3 furniture keys (1-4, Q, E, R) will be added in input.cpp.
// ---------------------------------------------------------------------------
static void keyboard(unsigned char key, int /*x*/, int /*y*/)
{
    if (key == 27) exit(0);   // ESC — quit
    glutPostRedisplay();
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1200, 800);
    glutCreateWindow("Interactive 3D Hostel Room Planner");

    // Sky-blue background visible through the open front face (Z = +2.25).
    glClearColor(0.53f, 0.75f, 0.87f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}
