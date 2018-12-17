#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <GLFW/glfw3.h>

#ifdef MACOSX
  #include <GLUT/glut.h>
#else
  #include <GL/glut.h>
#endif

// Display list for coordinate axis
GLuint axisList;
int AXIS_SIZE= 200;
int axisEnabled= 0;

GLfloat eyex, eyey, eyez;

GLfloat gravity = -3.0;
GLboolean gravityIsOn = GL_TRUE;
GLboolean uniformColour = GL_TRUE;
GLboolean useTriangles = GL_TRUE;

#define firework_num 10
GLint numOfParticles = 1024;
GLboolean numOfParticlesHasChanged = GL_FALSE;
GLfloat max_y_speed = 25.0;
GLfloat min_y_speed = 75.0;
GLfloat fireworkLife = 0.01;

//Variables to store data for calculating the time for each frame to be rendered
double firstTime = 0;
double finalTime = 0;
GLint totalFrames = 0;

typedef struct {
  GLfloat *x;
  GLfloat *y;
  GLfloat *z;

  GLfloat *xVel;
  GLfloat *yVel;
  GLfloat *zVel;
  GLfloat *speed;

  GLfloat *r;
  GLfloat *g;
  GLfloat *b;
  GLfloat a;

  GLfloat prevY;
  GLint framesUntilLaunch;
  GLint timeAfterExplosion;
  GLfloat fireworkSize;
  GLfloat numOfP;
  GLboolean hasExploded;
} Firework;

Firework fireworkArr[firework_num];

//Return random double within range [0,1]
double myRandom() {
  return (rand()/(double)RAND_MAX);
}

//Initialise and re-initialise the fireworks once they have faded out after exploding
//Velocity initialisation is done before launch, this slows the animation while initialising,
//however once a firework launches it runs smoother as a result of already having the velocity values
void initFirework(Firework *firework) {
  float xLocation = -300 + myRandom() * 600.0;
  float yLocation = -1000.0;
  float zLocation = -300 + myRandom() * 600.0;
  float initSpeed = min_y_speed + myRandom() * max_y_speed;
  float initTheta = 2 * M_PI * myRandom();
  float initPhi = acos(1 - myRandom() * 0.3);

  //Initial firework load
  if(!numOfParticlesHasChanged) {
    firework->x = malloc(sizeof(GLfloat) * numOfParticles);
    firework->y = malloc(sizeof(GLfloat) * numOfParticles);
    firework->z = malloc(sizeof(GLfloat) * numOfParticles);

    firework->xVel = malloc(sizeof(GLfloat) * numOfParticles);
    firework->yVel = malloc(sizeof(GLfloat) * numOfParticles);
    firework->zVel = malloc(sizeof(GLfloat) * numOfParticles);

    firework->speed = malloc(sizeof(GLfloat) * numOfParticles);
    firework->r = malloc(sizeof(GLfloat) * numOfParticles);
    firework->g = malloc(sizeof(GLfloat) * numOfParticles);
    firework->b = malloc(sizeof(GLfloat) * numOfParticles);
  }
  //Reallocate memory if the user changes the number of firework particles
  else {
    if(firework->numOfP != numOfParticles) {
      firework->x = realloc(firework->x, sizeof(GLfloat) * numOfParticles);
      firework->y = realloc(firework->y, sizeof(GLfloat) * numOfParticles);
      firework->z = realloc(firework->z, sizeof(GLfloat) * numOfParticles);

      firework->xVel = realloc(firework->xVel, sizeof(GLfloat) * numOfParticles);
      firework->yVel = realloc(firework->yVel, sizeof(GLfloat) * numOfParticles);
      firework->zVel = realloc(firework->zVel, sizeof(GLfloat) * numOfParticles);

      firework->speed = realloc(firework->speed, sizeof(GLfloat) * numOfParticles);
      firework->r = realloc(firework->r, sizeof(GLfloat) * numOfParticles);
      firework->g = realloc(firework->g, sizeof(GLfloat) * numOfParticles);
      firework->b = realloc(firework->b, sizeof(GLfloat) * numOfParticles);
    }
  }

  firework->numOfP = numOfParticles;

  for(int i = 0; i < firework->numOfP; i++) {
    firework->x[i] = xLocation;
    firework->y[i] = yLocation;
    firework->prevY = yLocation;
    firework->z[i] = zLocation;
    firework->speed[i] = initSpeed;
    firework->xVel[i] = sin(initPhi) * cos(initTheta);
    firework->yVel[i] = cos(initPhi);
    firework->zVel[i] = sin(initPhi) * sin(initTheta);

    firework->r[i] = 0.2 + (float)myRandom() * 0.8;
    firework->g[i] = 0.2 + (float)myRandom() * 0.8;
    firework->b[i] = 0.2 + (float)myRandom() * 0.8;
  }

  firework->a = 1.0;
  firework->framesUntilLaunch = (int)(rand() % 200);
  firework->timeAfterExplosion = 0;
  firework->fireworkSize = 2.5 + myRandom() * 3.0;
  firework->hasExploded = GL_FALSE;
}

//Move a firework until it explodes
void timestep(Firework *firework) {
  //If the firework hasn't exploded yet, move it
  for(int i = 0; i < firework->numOfP; i++) {
    if(firework->framesUntilLaunch <= 0) {
      firework->x[i] += firework->speed[i] * firework->xVel[i];
      firework->y[i] += firework->speed[i] * firework->yVel[i];
      firework->z[i] += firework->speed[i] * firework->zVel[i];

      firework->speed[i] += gravity;
    }
  }
  firework->framesUntilLaunch--;

  //Once the firework reaches its peak, explode all its particles
  //in random directions by setting the directions with random speeds
  if(firework->speed[0] <= 0) {
    firework->hasExploded = GL_TRUE;
    for(int i = 0; i < firework->numOfP; i++) {
      //Randomise directions and speeds of each particle
      GLfloat newT = 2 * M_PI * myRandom();
      GLfloat newP = acos(1 - 2 * myRandom());

      firework->xVel[i] = sin(newP) * cos(newT) ;
      firework->yVel[i] = cos(newP);
      firework->zVel[i] = sin(newP) * sin(newT);

      firework->speed[i] = rand() % 50;
    }
  }
  firework->prevY = firework->y[0];
}

//Timestep for after the firework has exploded
void explodeFirework(Firework *firework) {
  for(int i = 0; i < firework->numOfP; i++) {
    //Slow the particle the longer it lives
    firework->speed[i] *= 0.99;

    firework->x[i] += firework->speed[i] * firework->xVel[i];
    firework->y[i] += firework->speed[i] * firework->yVel[i];
    firework->z[i] += firework->speed[i] * firework->zVel[i];
    if(gravityIsOn)
      firework->y[i] += gravity * firework->timeAfterExplosion;
  }

  if(firework->a > 0.0)
    firework->a -= fireworkLife;
  else
    initFirework(firework);
}

void keyboard(unsigned char key, int x, int y) {
  switch(key) {
    case 27: //Esc
      finalTime = glfwGetTime();
      //printf("Average time between frames for %d particle(s) per firework: %fms\n", numOfParticles, 1000.0 * ((finalTime - firstTime) / totalFrames));
      glfwTerminate();
      exit(0);
    case 99: //c - Toggle between random colour particles or a uniform colour
      uniformColour = !uniformColour;
      if(uniformColour)
        printf("Single colour fireworks.\n");
      else
        printf("Random colour per firework particle.\n");
      break;
    case 103: //g - Toggle gravity when the firework explodes
      gravityIsOn = !gravityIsOn;
      if(gravityIsOn)
        printf("Gravity is on.\n");
      else
        printf("Gravity is off.\n");
      break;
    case 116: //t - Toggle rendering with GL_POINTS or GL_TRIANGLES
      useTriangles = !useTriangles;
      if(useTriangles)
        printf("Rendering with triangles.\n");
      else
        printf("Rendering with points.\n");
      break;
    case 43: //+ - Increase the firework particle size
      for(int i = 0; i < firework_num; i++)
        fireworkArr[i].fireworkSize++;
      break;
    case 45: //- Decease the firework particle size
    {
      float newSize = 0.0;
      for(int i = 0; i < firework_num; i++) {
        newSize = fireworkArr[i].fireworkSize--;
        if(newSize <= 1.0) //Min size of 1
          fireworkArr[i].fireworkSize = 1.0;
        else
          fireworkArr[i].fireworkSize--;
      }
      break;
    }
    case 91: // [ - Increase the time for the firework particles to fade out
    {
      float newLife = fireworkLife - 0.001;
      if(newLife <= 0.001)
        fireworkLife = 0.001;
      else
        fireworkLife -= 0.001;
      printf("New alpha change per frame: %.3f\n", fireworkLife);
      break;
    }
    case 93: // ] - Increase the time for the firework particles to fade out
      fireworkLife += 0.001;
      printf("New alpha change per frame: %.3f\n", fireworkLife);
      break;
    case 49: //Viewpoint 1
      eyex = 2000.0;
      eyey = -400.0;
      eyez = 0.0;
      break;
    case 50: //Viewpoint 2
      eyex = 50.0;
      eyey = -990.0;
      eyez = 50.0;
      break;
    case 51: //Viewpoint 3
      eyex = 1.0;
      eyey = 650.0;
      eyez = 1.0;
      break;
    case 52: //Viewpoint 4
      eyex = 1200.0;
      eyey = 50.0;
      eyez = 0.0;
      break;
    case 97: //a - Toggle visibility of the axis
      axisEnabled = !axisEnabled;
      break;
  }
  glutPostRedisplay();
}

void SpecialInput(int key, int x, int y) {
    switch(key)
    {
      case GLUT_KEY_UP: //Increase gravity
        gravity += 0.1;
        printf("Gravity: %f\n", gravity);
        break;
      case GLUT_KEY_DOWN: //Decrease gravity
        gravity -= 0.1;
        printf("Gravity: %f\n", gravity);
        break;
      case GLUT_KEY_LEFT: //Half the number of particles per firework
        if(numOfParticles != 1) {
          numOfParticles /= 2;
          numOfParticlesHasChanged = GL_TRUE;
        }
        printf("Number of particles in a firework: %d.\n", numOfParticles);
        break;
      case GLUT_KEY_RIGHT: //Double the number of particles per firework
        numOfParticles *= 2;
        numOfParticlesHasChanged = GL_TRUE;
        printf("Number of particles in a firework: %d.\n", numOfParticles);
        break;
    }
}

void display() {
  totalFrames++;

  glLoadIdentity();
  gluLookAt(eyex, eyey, eyez,
            0.0, 0.0, 0.0,
            0.0, 1.0, 0.0);
  // Clear the screen
  glClear(GL_COLOR_BUFFER_BIT);
  // If enabled, draw coordinate axis
  if(axisEnabled) glCallList(axisList);

  for(int i = 0; i < firework_num; i++){
    glPointSize(fireworkArr[i].fireworkSize);

    for(int particle = 0; particle < fireworkArr[i].numOfP; particle++) {
      if(!fireworkArr[i].hasExploded) {
        glBegin(GL_POINTS);
        glColor3f(1.0, 1.0, 1.0);
        glVertex3f(fireworkArr[i].x[particle], fireworkArr[i].y[particle], fireworkArr[i].z[particle]);
        glEnd();
      }
      else {
        if(!uniformColour)
          glColor4f(fireworkArr[i].r[particle], fireworkArr[i].g[particle], fireworkArr[i].b[particle], fireworkArr[i].a);
        else
          glColor4f(fireworkArr[i].r[i], fireworkArr[i].g[i], fireworkArr[i].b[i], fireworkArr[i].a);
      }
      if(useTriangles) {
        glBegin(GL_TRIANGLES);
        float sz = fireworkArr[i].fireworkSize;
        glVertex3f(fireworkArr[i].x[particle], fireworkArr[i].y[particle] + sz, fireworkArr[i].z[particle]);
        glVertex3f(fireworkArr[i].x[particle], fireworkArr[i].y[particle] - sz, fireworkArr[i].z[particle] - sz);
        glVertex3f(fireworkArr[i].x[particle], fireworkArr[i].y[particle] - sz, fireworkArr[i].z[particle] + sz);
        glEnd();
      }
      else {
        glBegin(GL_POINTS);
        glVertex3f(fireworkArr[i].x[particle], fireworkArr[i].y[particle], fireworkArr[i].z[particle]);
        glEnd();
      }
    }

    if(!fireworkArr[i].hasExploded) {
      timestep(&fireworkArr[i]);
    }
    else {
      fireworkArr[i].timeAfterExplosion++;
      explodeFirework(&fireworkArr[i]);
    }
  }
  glutPostRedisplay();
  glutSwapBuffers();
}

void reshape(int width, int height) {
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glViewport(0, 0, (GLsizei)width, (GLsizei)height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60, (GLfloat)width / (GLfloat)height, 1.0, 10000.0);
  glMatrixMode(GL_MODELVIEW);
}

// Create a display list for drawing coord axis
void makeAxes() {
  axisList = glGenLists(1);
  glNewList(axisList, GL_COMPILE);
      glLineWidth(2.0);
      glBegin(GL_LINES);
      glColor3f(1.0, 0.0, 0.0);       // X axis - red
      glVertex3f(0.0, 0.0, 0.0);
      glVertex3f(AXIS_SIZE, 0.0, 0.0);
      glColor3f(0.0, 1.0, 0.0);       // Y axis - green
      glVertex3f(0.0, 0.0, 0.0);
      glVertex3f(0.0, AXIS_SIZE, 0.0);
      glColor3f(0.0, 0.0, 1.0);       // Z axis - blue
      glVertex3f(0.0, 0.0, 0.0);
      glVertex3f(0.0, 0.0, AXIS_SIZE);
    glEnd();
  glEndList();
}

void initGraphics(int argc, char *argv[]) {
  glutInit(&argc, argv);
  glutInitWindowSize(1920, 1080);
  glutInitWindowPosition(100, 100);
  glutInitDisplayMode(GLUT_DOUBLE);
  glutCreateWindow("COMP37111 Particles");
  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard);
  glutSpecialFunc(SpecialInput);
  glutReshapeFunc(reshape);
  eyex = 1200.0;
  eyey = 50.0;
  eyez = 0.0;
  if(!glfwInit())
    printf("glfw initialisation failed.");
  makeAxes();
  for(int i = 0; i < firework_num; i++)
    initFirework(&fireworkArr[i]);
  firstTime = glfwGetTime();
}

int main(int argc, char *argv[]) {
  double f;
  srand(time(NULL));
  initGraphics(argc, argv);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_POINT_SMOOTH);
  glutMainLoop();
}
