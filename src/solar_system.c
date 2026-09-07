#include <GL/glut.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <GL/glut.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PI            3.14159265358979f
#define DEG2RAD(d)    ((d) * PI / 180.0f)
#define NUM_STARS     700

static float camTheta = 68.0f;
static float camPhi   =  0.0f;
static float camDist  = 23.0f;

static int lastMX = -1, lastMY = -1, isDragging = 0;

static float aMercury=0,aVenus=0,aEarth=0,aMars=0;
static float aJupiter=0,aSaturn=0,aUranus=0,aNeptune=0;
static float aMoon=0,aPhobos=0,aDeimos=0;

static float rMercury=0,rVenus=0,rEarth=0,rMars=0;
static float rJupiter=0,rSaturn=0,rUranus=0,rNeptune=0;

static float speed  = 1.0f;
static int    paused = 0;

static float stars[NUM_STARS][4]; /* x, y, z, brightness */
static GLuint starList = 0;


static int winW = 900, winH = 900;


static void initStars(void) {
    srand(314159);
    for (int i = 0; i < NUM_STARS; i++) {
        float theta = ((float)rand()/RAND_MAX) * 2.0f * PI;
        float phi   = ((float)rand()/RAND_MAX) * PI;
        float r     = 42.0f + ((float)rand()/RAND_MAX) * 6.0f;
        stars[i][0] = r * sinf(phi) * cosf(theta);
        stars[i][1] = r * sinf(phi) * sinf(theta);
        stars[i][2] = r * cosf(phi);
        stars[i][3] = 0.35f + ((float)rand()/RAND_MAX) * 0.65f; /* brightness */
    }
}

static void buildStarList(void) {
    starList = glGenLists(1);
    glNewList(starList, GL_COMPILE);
        glDisable(GL_LIGHTING);
        glBegin(GL_POINTS);
        for (int i = 0; i < NUM_STARS; i++) {
            float b = stars[i][3];
            /* Tint: bluish for dim, yellowish for bright */
            glColor3f(b * 0.88f + 0.12f, b * 0.88f + 0.12f, b);
            glVertex3f(stars[i][0], stars[i][1], stars[i][2]);
        }
        glEnd();
        glEnable(GL_LIGHTING);
    glEndList();
}

/* ══════════════════════  Drawing helpers  ═══════════════════════ */

static void drawText(const char *text, float x, float y, float z) {
    glDisable(GL_LIGHTING);
    glColor4f(1.0f, 1.0f, 1.0f, 0.85f);
    glRasterPos3f(x, y, z);
    for (int i = 0; text[i]; i++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, text[i]);
    glEnable(GL_LIGHTING);
}

static void setMaterial(float r, float g, float b, float shine) {
    GLfloat amb[]  = { r*0.12f, g*0.12f, b*0.12f, 1.0f };
    GLfloat diff[] = { r,       g,       b,       1.0f };
    GLfloat spec[] = { 0.35f,   0.35f,   0.35f,   1.0f };
    GLfloat zero[] = { 0.0f,    0.0f,    0.0f,    1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT,   amb);
    glMaterialfv(GL_FRONT, GL_DIFFUSE,   diff);
    glMaterialfv(GL_FRONT, GL_SPECULAR,  spec);
    glMaterialf (GL_FRONT, GL_SHININESS, shine);
    glMaterialfv(GL_FRONT, GL_EMISSION,  zero);
}

/* Orbit circle (Darker Version) */
static void drawOrbit(float radius) {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* White, but significantly lower alpha (opacity) to 20% */
    glColor4f(1.0f, 1.0f, 1.0f, 0.20f);

    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 140; i++) {
        float a = 2.0f * PI * i / 140.0f;
        glVertex3f(radius * cosf(a), 0.0f, radius * sinf(a));
    }
    glEnd();
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

/* Ring band  (drawn in planet-local space) */
static void drawRing(float inner, float outer,
                     float r, float g, float b, float a) {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(r, g, b, a);
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= 140; i++) {
        float ang = 2.0f * PI * i / 140.0f;
        float cx = cosf(ang), cz = sinf(ang);
        glVertex3f(inner * cx, 0.0f, inner * cz);
        glVertex3f(outer * cx, 0.0f, outer * cz);
    }
    glEnd();
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

/* Layered additive glow around the sun - REDDISH GLOW */
static void drawSunGlow(void) {
    /* Kept the exact same alphas and radiuses for the glow you liked */
    static const struct { float r; float a; } layers[] = {
        {1.60f, 0.15f}, {1.85f, 0.10f}, {2.20f, 0.07f},
        {2.70f, 0.04f}, {3.30f, 0.02f}, {4.00f, 0.01f}
    };
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); /* Additive blending */
    glDepthMask(GL_FALSE);

    for (int i = 0; i < 6; i++) {
        float fade = 1.0f - i * 0.15f;

        /* CHANGED: Dropped the green channel significantly to make it a deep, fiery red */
        glColor4f(1.0f, 0.45f * fade, 0.1f * fade, layers[i].a);

        glutSolidSphere(layers[i].r, 40, 40);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

/* On-screen HUD */
static void drawHUD(void) {
    char buf[128];

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, winW, 0, winH);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    /* Speed / pause line */
    snprintf(buf, sizeof(buf), "Speed: %.1fx   %s",
             speed, paused ? "[ PAUSED ]" : "");
    glColor3f(0.7f, 0.92f, 1.0f);
    glRasterPos2i(10, winH - 20);
    for (int i = 0; buf[i]; i++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, buf[i]);

    /* Help lines */
    static const char *help[] = {
        "Mouse Drag: Orbit camera   Scroll / W-S: Zoom",
        "A / D: Pan   +/-: Speed   Space: Pause   ESC: Quit"
    };
    glColor3f(0.45f, 0.65f, 0.88f);
    for (int i = 0; i < 2; i++) {
        glRasterPos2i(10, 36 - i * 16);
        for (int j = 0; help[i][j]; j++)
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, help[i][j]);
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

/* ══════════════════════════  Display  ═══════════════════════════ */

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    /* Spherical camera */
    float cx = camDist * sinf(DEG2RAD(camTheta)) * cosf(DEG2RAD(camPhi));
    float cy = camDist * cosf(DEG2RAD(camTheta));
    float cz = camDist * sinf(DEG2RAD(camTheta)) * sinf(DEG2RAD(camPhi));
    gluLookAt(cx, cy, cz,  0.0, 0.0, 0.0,  0.0, 1.0, 0.0);

    /* Stars */
    glPointSize(1.6f);
    glCallList(starList);

    /* Sun-centered point light */
    GLfloat lpos[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lpos);

    /* Orbit rings */
    float orbitR[] = {3.0f, 4.5f, 6.0f, 8.0f, 10.0f, 12.0f, 14.0f, 16.0f};
    for (int i = 0; i < 8; i++) drawOrbit(orbitR[i]);

    /* ── SUN ─────────────────────────────────────────────────── */
    glPushMatrix();
        glDisable(GL_LIGHTING);

        /* CHANGED: Core is now a deep red instead of yellow-orange */
        glColor3f(1.0f, 0.45f, 0.1f);

        glutSolidSphere(1.5f, 64, 64);
        drawSunGlow();
        drawText("Sun", 0.0f, 2.1f, 0.0f);
        glEnable(GL_LIGHTING);
    glPopMatrix();

    /* ── MERCURY ────────────────────────────────────────────── */
    glPushMatrix();
        glRotatef(aMercury, 0,1,0);
        glTranslatef(3.0f, 0, 0);
        glRotatef(0.03f, 0,0,1);          /* nearly no tilt        */
        glRotatef(rMercury, 0,1,0);
        setMaterial(0.53f, 0.52f, 0.50f, 18);
        glutSolidSphere(0.18f, 32, 32);
        drawText("Mercury", 0, 0.34f, 0);
    glPopMatrix();

    /* ── VENUS ──────────────────────────────────────────────── */
    glPushMatrix();
        glRotatef(aVenus, 0,1,0);
        glTranslatef(4.5f, 0, 0);
        glRotatef(177.4f, 0,0,1);         /* retrograde, near-180° */
        glRotatef(rVenus, 0,1,0);
        setMaterial(0.98f, 0.74f, 0.28f, 28);
        glutSolidSphere(0.28f, 32, 32);
        drawText("Venus", 0, 0.44f, 0);
    glPopMatrix();

    /* ── EARTH (tilt applied to sphere; label/moon in orbit frame) */
    glPushMatrix();
        glRotatef(aEarth, 0,1,0);
        glTranslatef(6.0f, 0, 0);
        /* tilted sphere */
        glPushMatrix();
            glRotatef(23.5f, 0,0,1);
            glRotatef(rEarth, 0,1,0);
            setMaterial(0.20f, 0.44f, 0.98f, 42);
            glutSolidSphere(0.32f, 42, 42);
        glPopMatrix();
        drawText("Earth", 0, 0.54f, 0);
        /* Moon */
        glPushMatrix();
            glRotatef(aMoon, 0,1,0);
            glTranslatef(0.82f, 0.08f, 0);
            setMaterial(0.72f, 0.72f, 0.70f, 14);
            glutSolidSphere(0.09f, 22, 22);
            drawText("Moon", 0, 0.18f, 0);
        glPopMatrix();
    glPopMatrix();

    /* ── MARS + Phobos + Deimos ─────────────────────────────── */
    glPushMatrix();
        glRotatef(aMars, 0,1,0);
        glTranslatef(8.0f, 0, 0);
        glPushMatrix();
            glRotatef(25.2f, 0,0,1);
            glRotatef(rMars, 0,1,0);
            setMaterial(0.82f, 0.30f, 0.16f, 22);
            glutSolidSphere(0.22f, 36, 36);
        glPopMatrix();
        drawText("Mars", 0, 0.40f, 0);
        /* Phobos */
        glPushMatrix();
            glRotatef(aPhobos, 0,1,0);
            glTranslatef(0.42f, 0.0f, 0);
            setMaterial(0.48f, 0.40f, 0.34f, 10);
            glutSolidSphere(0.04f, 14, 14);
        glPopMatrix();
        /* Deimos */
        glPushMatrix();
            glRotatef(aDeimos, 0,1,0);
            glTranslatef(0.60f, 0.04f, 0);
            setMaterial(0.44f, 0.37f, 0.31f, 10);
            glutSolidSphere(0.03f, 12, 12);
        glPopMatrix();
    glPopMatrix();

    /* ── JUPITER ────────────────────────────────────────────── */
    glPushMatrix();
        glRotatef(aJupiter, 0,1,0);
        glTranslatef(10.0f, 0, 0);
        glPushMatrix();
            glRotatef(3.1f, 0,0,1);
            glRotatef(rJupiter, 0,1,0);
            setMaterial(0.88f, 0.70f, 0.48f, 28);
            /* Slightly oblate (scale y down) */
            glScalef(1.0f, 0.94f, 1.0f);
            glutSolidSphere(0.75f, 52, 52);
        glPopMatrix();
        drawText("Jupiter", 0, 0.96f, 0);
    glPopMatrix();

    /* ── SATURN + multi-band rings ──────────────────────────── */
    glPushMatrix();
        glRotatef(aSaturn, 0,1,0);
        glTranslatef(12.0f, 0, 0);
        glRotatef(26.7f, 0,0,1);         /* tilt so rings show nicely */
        glPushMatrix();
            glRotatef(rSaturn, 0,1,0);
            setMaterial(0.93f, 0.86f, 0.54f, 32);
            glScalef(1.0f, 0.90f, 1.0f);
            glutSolidSphere(0.65f, 52, 52);
        glPopMatrix();
        /* Inner bright ring */
        drawRing(0.90f, 1.20f, 0.90f, 0.82f, 0.58f, 0.55f);
        /* Mid ring */
        drawRing(1.20f, 1.50f, 0.82f, 0.72f, 0.46f, 0.38f);
        /* Outer faint ring */
        drawRing(1.50f, 1.85f, 0.70f, 0.62f, 0.38f, 0.18f);
        /* Cassini division gap colour already by alpha change above */
        drawText("Saturn", 0, 1.05f, 0);
    glPopMatrix();

    /* ── URANUS + thin equatorial ring (sideways) ───────────── */
    glPushMatrix();
        glRotatef(aUranus, 0,1,0);
        glTranslatef(14.0f, 0, 0);
        glRotatef(97.8f, 0,0,1);         /* rolls on side */
        glPushMatrix();
            glRotatef(rUranus, 0,1,0);
            setMaterial(0.44f, 0.88f, 0.90f, 52);
            glutSolidSphere(0.48f, 40, 40);
        glPopMatrix();
        drawRing(0.68f, 0.74f, 0.62f, 0.82f, 0.86f, 0.42f);
        drawText("Uranus", 0, 0.68f, 0);
    glPopMatrix();

    /* ── NEPTUNE ────────────────────────────────────────────── */
    glPushMatrix();
        glRotatef(aNeptune, 0,1,0);
        glTranslatef(16.0f, 0, 0);
        glPushMatrix();
            glRotatef(28.3f, 0,0,1);
            glRotatef(rNeptune, 0,1,0);
            setMaterial(0.16f, 0.22f, 0.94f, 58);
            glutSolidSphere(0.46f, 40, 40);
        glPopMatrix();
        drawText("Neptune", 0, 0.65f, 0);
    glPopMatrix();

    /* HUD overlay */
    drawHUD();

    glutSwapBuffers();
}

/* ══════════════════════  Timer / update  ═════════════════════════ */

void update(int v) {
    if (!paused) {
        /* * TIME SCALE: 1.0 'speed' = 1 Earth Day per frame.
         * Formula: Degrees per frame = (360.0f / Period_in_Days) * speed
         */

        /* ── Orbital Speeds (Years to Days) ─────────────────────── */
        aMercury += (360.0f / 87.97f)    * speed;
        aVenus   += (360.0f / 224.70f)   * speed;
        aEarth   += (360.0f / 365.25f)   * speed;
        aMars    += (360.0f / 686.98f)   * speed;
        aJupiter += (360.0f / 4332.59f)  * speed; // 11.86 years
        aSaturn  += (360.0f / 10759.22f) * speed; // 29.46 years
        aUranus  += (360.0f / 30685.40f) * speed; // 84 years
        aNeptune += (360.0f / 60189.00f) * speed; // 164.8 years

        /* ── Moons Orbital Speeds ───────────────────────────────── */
        aMoon    += (360.0f / 27.32f)    * speed; // orbits Earth ~27 days
        aPhobos  += (360.0f / 0.3189f)   * speed; // orbits Mars 3 times a day
        aDeimos  += (360.0f / 1.263f)    * speed; // orbits Mars ~30 hours

        /* ── Self-Rotation Speeds (Day length in Earth Days) ────── */
        /* Note: Your display function already applies 180° / 90° tilts for
            Venus and Uranus, so we can keep the math positive here. */
        rMercury += (360.0f / 58.65f)    * speed;
        rVenus   += (360.0f / 243.02f)   * speed; // Slowest rotation
        rEarth   += (360.0f / 1.00f)     * speed; // 1 rotation per day
        rMars    += (360.0f / 1.026f)    * speed; // Almost exactly Earth-like
        rJupiter += (360.0f / 0.413f)    * speed; // ~9.9 hours
        rSaturn  += (360.0f / 0.444f)    * speed; // ~10.6 hours
        rUranus  += (360.0f / 0.718f)    * speed; // ~17.2 hours
        rNeptune += (360.0f / 0.671f)    * speed; // ~16.1 hours
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);   /* ~60 fps */
}

/* ══════════════════════  Input handlers  ═════════════════════════ */

void mouseButton(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        isDragging = (state == GLUT_DOWN);
        lastMX = x;  lastMY = y;
    }
    /* Scroll-wheel zoom */
    if (button == 3) camDist = camDist >  5.0f ? camDist - 0.8f :  5.0f;
    if (button == 4) camDist = camDist < 42.0f ? camDist + 0.8f : 42.0f;
    glutPostRedisplay();
}

void mouseMotion(int x, int y) {
    if (isDragging && lastMX >= 0) {
        camPhi   += (x - lastMX) * 0.40f;
        camTheta += (y - lastMY) * 0.40f;
        if (camTheta <   3.0f) camTheta =   3.0f;
        if (camTheta > 177.0f) camTheta = 177.0f;
        lastMX = x;  lastMY = y;
        glutPostRedisplay();
    }
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 'w': case 'W': camDist -= 0.9f; if (camDist <  5.0f) camDist =  5.0f; break;
        case 's': case 'S': camDist += 0.9f; if (camDist > 42.0f) camDist = 42.0f; break;
        case 'a': case 'A': camPhi  -= 3.0f; break;
        case 'd': case 'D': camPhi  += 3.0f; break;
        case '+': case '=': speed += 0.2f; if (speed > 12.0f) speed = 12.0f; break;
        case '-': case '_': speed -= 0.2f; if (speed <  0.1f) speed =  0.1f; break;
        case ' ':           paused = !paused; break;
        case 27:            exit(0);
    }
    glutPostRedisplay();
}

void reshape(int w, int h) {
    winW = w;  winH = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(55.0, (double)w / (double)(h ? h : 1), 0.4, 130.0);
    glMatrixMode(GL_MODELVIEW);
}

/* ══════════════════════════  Init (Darker Scene)  ══════════════════════════════ */
void init(void) {
    /* Set background to pure deep space black */
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glShadeModel(GL_SMOOTH);

    /* --- Key Darkness Update --- */
    /* Extremely low scene ambient so dark-side of planets are near pure black */
    GLfloat sceneAmb[] = {0.005f, 0.005f, 0.005f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, sceneAmb);

    /* --- Key Darkness Update --- */
    /* Dimmer, warmer light source hitting the planets (dim amber) */
    GLfloat lDiff[] = {0.80f, 0.70f, 0.50f, 1.0f};
    GLfloat lSpec[] = {0.70f, 0.60f, 0.40f, 1.0f};
    GLfloat lAmb[]  = {0.00f, 0.00f, 0.00f, 1.0f};
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  lDiff);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lSpec);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  lAmb);

    /* Linear point-size hint for stars */
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_POINT_SMOOTH);

    initStars();
    buildStarList();
}

/* ════════════════════════  Entry point  ═════════════════════════ */

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(900, 900);
    glutCreateWindow("Solar System");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);
    glutTimerFunc(16, update, 0);

    glutMainLoop();
    return 0;
}
