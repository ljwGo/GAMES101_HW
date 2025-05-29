#include <iostream>

#include "application.h"
#include "CGL/CGL.h"

// same namespace can use others directly;
CGL::Vector2D KinematicPointFn(double theta01, double theta02, double l1, double l2){
  double x = l1 * cos(theta01) + l2 * cos(theta01 + theta02);
  double y = l1 * sin(theta01) + l2 * sin(theta01 + theta02);
  CGL::Vector2D calcP(x, y);
  return calcP;
}

// theta01 and theta02 is base on radian system
double KinematicLossFunc(double theta01, double theta02, double l1, double l2, double targetPx, double targetPy){
  CGL::Vector2D calcP = KinematicPointFn(theta01, theta02, l1, l2);
  CGL::Vector2D targetP(targetPx, targetPy);

  return (calcP - targetP).norm();
}

// Loss function
double KinematicLossFuncWrapper(doubleVector& args){
  return KinematicLossFunc(args[0], args[1], args[2], args[3], args[4], args[5]);
}

namespace CGL
{

  Application::Application(AppConfig config) { this->config = config; }

  Application::~Application() {}

  void Application::init()
  {
    // Enable anti-aliasing and circular points.
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

    glPointSize(12);
    glLineWidth(4);

    glColor3f(1.0, 1.0, 1.0);

    solver = GradientSolver(10 / 180. * M_PI, 1.f, 100, KinematicLossFuncWrapper);
  }

  void Application::render()
  {
    CGL::Vector2D targetP(-50, 20);

    double l1 = 200;
    double l2 = 200;
    // first and second argument represent independent variable. l1, l2 are pole length. 
    doubleVector args{0, 0, l1, l2, targetP.x, targetP.y};
    doubleVector defineDomain{0, M_PI, 0, M_PI};
    doubleVector solve = solver.Solver(args, 2, defineDomain);

    // show pole point and connection.
    CGL::Vector2D calcP = KinematicPointFn(solve[0], solve[1], solve[2], solve[3]);
    CGL::Vector2D P01(cos(solve[0]) * l1, sin(solve[0]) * l1);
  
    glPointSize(20);
    glBegin(GL_POINTS);

    // show pole point
    glColor3f(0.0, 1.0, 0.0);
    glVertex2d(0, 0);
    glVertex2d(P01.x, P01.y);
    glVertex2d(calcP.x, calcP.y);
    glEnd();

    glPointSize(10);
    glBegin(GL_POINTS);
    // show target point
    glColor3f(1.0, 0.0, 0.0);
    glPointSize(5);
    glVertex2d(targetP.x, targetP.y);
    glEnd();


    glBegin(GL_LINES);

    glColor3f(0.0, 0.0, 1.0);
    glVertex2d(0, 0);
    glVertex2d(P01.x, P01.y);

    glVertex2d(P01.x, P01.y);
    glVertex2d(calcP.x, calcP.y);
    glEnd();

    glFlush();
  }

  void Application::resize(size_t w, size_t h)
  {
    screen_width = w;
    screen_height = h;

    float half_width = (float)screen_width / 2;
    float half_height = (float)screen_height / 2;

    glMatrixMode(GL_PROJECTION);  // PROJECTION view method. so glOrtho is projection
    glLoadIdentity();
    glOrtho(-half_width, half_width, -half_height, half_height, 1, 0);
  }

  void Application::keyboard_event(int key, int event, unsigned char mods)
  {
    switch (key)
    {
    case '-':
      break;
    case '=':
      break;
    }
  }

  string Application::name() { return "GradientSolverToKinematic"; }

  string Application::info()
  {
    // concat string convenient
    ostringstream steps;

    return steps.str();
  }
}
