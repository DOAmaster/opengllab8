//modified by: Derrick Alden
//fonts commented out until VM compile issue is solved on my home machine
//
//3480 Computer Graphics
//lab8.cpp
//Author: Gordon Griesel
//Date:   2017
//
//a simple implementation of OpenGL
//circle drawing
//rotation matrix
//
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include "fonts.h"
#include <math.h>

#define rnd() (float)rand() / (float)RAND_MAX
const int MAX_POINTS = 10000;
const float PI = 3.14159265358979;

void initXWindows(void);
void init_opengl(void);
void cleanupXWindows(void);
void check_resize(XEvent *e);
//void check_mouse(XEvent *e);
int check_keys(XEvent *e);
void physics(void);
void render(void);

struct Point {
	float x, y;
};

class Global {
public:
	int xres, yres;
	int mode;
	Point point[MAX_POINTS];
	int npoints;
	Point center;
	float radius;
	float matrix[2][2];
	Global() {
		srand((unsigned)time(NULL));
		xres = 800;
		yres = 600;
		mode = 0;
		npoints = 5;
		center.x = xres/2;
		center.y = yres/2;
		radius = 100.0;
	}
} g;

//creates windows, destory window, and other openGL init
class X11_wrapper {
private:
	Display *dpy;
	Window win;
	GLXContext glc;
public:
	X11_wrapper() {
		GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
		//GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, None };
		XSetWindowAttributes swa;
		//setup_screen_res(g.xres, g.yres);
		dpy = XOpenDisplay(NULL);
		if(dpy == NULL) {
			printf("\n\tcannot connect to X server\n\n");
			exit(EXIT_FAILURE);
		}
		Window root = DefaultRootWindow(dpy);
		XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
		if(vi == NULL) {
			printf("\n\tno appropriate visual found\n\n");
			exit(EXIT_FAILURE);
		} 
		Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
		swa.colormap = cmap;
		swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
							StructureNotifyMask | SubstructureNotifyMask;
		win = XCreateWindow(dpy, root, 0, 0, g.xres, g.yres, 0,
								vi->depth, InputOutput, vi->visual,
								CWColormap | CWEventMask, &swa);
		set_title();
		glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
		glXMakeCurrent(dpy, win, glc);
	}
	~X11_wrapper() {
		XDestroyWindow(dpy, win);
		XCloseDisplay(dpy);
	}
	void set_title() {
		//Set the window title bar.
		XMapWindow(dpy, win);
		XStoreName(dpy, win, "Lab-8 Circles");
	}
	void check_resize(XEvent *e) {
		//The ConfigureNotify is sent by the
		//server if the window is resized.
		if (e->type != ConfigureNotify)
			return;
		XConfigureEvent xce = e->xconfigure;
		if (xce.width != g.xres || xce.height != g.yres) {
			//Window size did change.
			void reshape_window(int width, int height);
			reshape_window(xce.width, xce.height);
		}
	}
	bool getPending() { return XPending(dpy); }
	void getNextEvent(XEvent *e) { XNextEvent(dpy, e); }
	void swapBuffers() { glXSwapBuffers(dpy, win); }
} x11;

int main()
{
	init_opengl();
	int done = 0;
	while (!done) {
		while (x11.getPending()) {
			XEvent e;
			x11.getNextEvent(&e);
			x11.check_resize(&e);
			//check_mouse(&e);
			done = check_keys(&e);
		}
		physics();
		render();
		x11.swapBuffers();
	}
	return 0;
}

void setup_screen_res(const int w, const int h) {
	g.xres = w;
	g.yres = h;
}

void reshape_window(int width, int height) {
	//window has been resized.
	setup_screen_res(width, height);
	glViewport(0, 0, (GLint)width, (GLint)height);
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	glOrtho(0, g.xres, 0, g.yres, -1, 1);
	x11.set_title();
}

void init_opengl()
{
	//OpenGL initialization
	glViewport(0, 0, g.xres, g.yres);
	//Initialize matrices
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	//This sets 2D mode (no perspective)
	glOrtho(0, g.xres, 0, g.yres, -1, 1);
	//Clear the screen
	glClearColor(0.0, 0.0, 0.0, 1.0);
	//glClear(GL_COLOR_BUFFER_BIT);
	//Do this to allow fonts
	glEnable(GL_TEXTURE_2D);
//	initialize_fonts();
	glClear(GL_COLOR_BUFFER_BIT);
}

int check_keys(XEvent *e)
{
	//Was there input from the keyboard?
	if (e->type == KeyPress) {
		int key = XLookupKeysym(&e->xkey, 0);
		switch (key) {
			case XK_1:
				//lab-8 circle
				g.mode = 1;
				break;
			case XK_2:
				//rotation matrix
				g.mode = 2;
				break;
			case XK_3:
				//lab-8 circle 
				g.mode = 3;
				break;
			case XK_4:
				//Bresenham's circle algorithm
				g.mode = 4;
				break;
			case XK_5:
				//E
				//triangle strip
				g.mode = 5;
				break;
			case XK_6:
				//triangle fan
				g.mode = 6;
				break;
			case XK_7:
				g.mode = 7;
				break;
			case XK_minus:
				if (--g.npoints < 5)
					g.npoints = 5;
				break;
			case XK_equal:
				if (++g.npoints >= MAX_POINTS)
					g.npoints = MAX_POINTS;
				break;
			case XK_comma:
				g.radius -= 2.0;
				if (g.radius < 2.0)
					g.radius = 2.0;
				break;
			case XK_period:
				g.radius += 2.0;
				if (g.radius > 1000.0)
					g.radius = 1000.0;
				break;
			case XK_Escape:
				return 1;
				break;
		}
	}
	return 0;
}

void physics()
{

}

/*
void showMenu()
{
	Rect r;
	r.bot = g.yres - 20;
	r.left = 15;
	r.center = 0;
	unsigned int color = 0x00ffff00;
	ggprint8b(&r, 20, color, "3480 Lab-8");
	color = 0x0000ff00;
	ggprint8b(&r, 16, color, "+ - number of points: %i", g.npoints);
	ggprint8b(&r, 20, color, "< > radius: %4.0f", g.radius);
	ggprint8b(&r, 16, color, "1 - points on a circle, (cos sin)");
	ggprint8b(&r, 16, color, "2 - points on a circle, (rotation matrix)");
	ggprint8b(&r, 16, color, "3 - Lab-8 circle");
	ggprint8b(&r, 16, color, "4 - Bresenham's circle");
	ggprint8b(&r, 16, color, "5 - Triangle-strip circle");
	ggprint8b(&r, 16, color, "6 - Triangle-fan circle");
}
*/

//mode 1
void showPointsOnACircle()
{

	float angle = 0.0;
	float inc = PI * 2.0 / (float)g.npoints;

	//define x and y using cos and sin
	for (int i=0; i<g.npoints; i++) {
		g.point[i].x = cos(angle) * g.radius;
		g.point[i].y = sin(angle) * g.radius;
		angle += inc;
	}

	//draw lines that connect points
	//the first and last points are connected in GL_LINE_LOOP
	
	glPushMatrix();
	glTranslatef(g.center.x, g.center.y, 0.0);
	glBegin(GL_LINE_LOOP);
	glColor3ub(255,255,90);

	for (int i=0; i<g.npoints; i ++) {
	    //translation
	//	glVertex2f(g.center.x+g.point[i].x, g.center.y+g.point[i].y);
		glVertex2f(g.point[i].x, g.point[i].y);
	}
	glEnd();
	glPopMatrix();


}

//own bresenham's mode 3
void lab8Circle()
{

	float x = g.radius;
	float y = 0.0;
	glColor3ub(255,255,90);
	glBegin(GL_POINTS);
	//draw point and translate it to the center
	//check if x is outside of radius, if so move it back in
	while(x >= y) {
	    	//ajusting for the center
		//mirror a few more times to complete the circle
		glVertex2f(g.center.x + x,g.center.y+y);
		glVertex2f(g.center.x + y,g.center.y+x);

		y += 1.0;

		float d0 = x;
		float d1 = y;
		float len = sqrt(d0 * d0 + d1*d1);
		if (len > g.radius)
		    x -= 1.0;
	}
	
	glEnd();
	glPopMatrix();


}

//triangle fan GL_TRIANGLE_FAN Mode 6
void triangleFan()
{

	//float x = g.radius;
	//float y = 0.0;
	glColor3ub(255,255,90);
	glBegin(GL_TRIANGLE_FAN);
	//draw point and translate it to the center
	//check if x is outside of radius, if so move it back in
	glTranslatef(g.center.x, g.center.y, 0.0);
	for (int i =0; i < g.npoints; i++) {    	
		glVertex3f(g.center.x +g.point[i].x, g.center.y+g.point[i].y, 0.0);
	}
	
	glEnd();
	glPopMatrix();


}
//triangle strips GL_TRIANGLE_FAN Mode 5 incomplete
void triangleStrip()
{

	float angle = 0.0;
	float inc = PI * 2.0 / (float)g.npoints;

	glColor3ub(255,255,90);
	glBegin(GL_TRIANGLE_STRIP);
	glTranslatef(g.center.x, g.center.y, 0.0);

	glFrontFace(GL_CW);

	for (int i =0; i < g.npoints; i++) {    	
		glVertex3f(g.center.x +g.point[i].x, g.center.y+g.point[i].y, 0.0);
	}
	
	glEnd();
	glPopMatrix();


}


void triangleStripRing()
{


	float angle = 0.0;
	float inc = PI * 2.0 / (float)g.npoints;

	//define x and y using cos and sin
	for (int i=0; i<g.npoints; i++) {
		g.point[i].x = cos(angle) * g.radius;
		g.point[i].y = sin(angle) * g.radius;
		angle += inc;
	}


	//define x and y using cos and sin smalle radius
	for (int i=0; i<g.npoints; i++) {
		g.point[i+ g.npoints].x = cos(angle) * g.radius - 100;
		g.point[i+ g.npoints].y = sin(angle) * g.radius - 100;
		angle += inc;
	}

	glColor3ub(255,255,90);
	glBegin(GL_TRIANGLE_STRIP);
	glTranslatef(g.center.x, g.center.y, 0.0);

	glFrontFace(GL_CW);

	int i = 0;
	for (i = 0; i < g.npoints; i++) {    	
		glVertex3f(g.center.x +g.point[i].x, g.center.y+g.point[i].y, 0.0);
		glVertex3f(g.center.x +g.point[i+g.npoints].x, g.center.y+g.point[i+g.npoints].y, 0.0);
	}
	
	glEnd();
	glPopMatrix();


}

//float is faster than double in openGL
void setupMatrix(float m[2][2], float angle) 
{
	m[0][0] = cos(angle);
	m[0][1] = -sin(angle);
	m[1][0] = sin(angle);
	m[1][1] = cos(angle);
}

void transformPoint(float m[2][2], Point *p) 
{
    	//make temp var when changing px
	Point tmp;
	tmp.x = p->x * m[0][0] + p->y * m[0][1];
	tmp.y = p->x * m[1][0] + p->y * m[1][1];

	p->x = tmp.x;
	p->y = tmp.y;
}

//rotation matrix Mode 2
void rotationMatrix()
{

    	float angle = 0.0;
	float inc = PI * 2.0 / (float)g.npoints;

	for ( int i =0 ; i < g.npoints; i++) {    	
		setupMatrix(g.matrix, angle);
		g.point[i].x = g.radius;
		g.point[i].y = 0.0;
		transformPoint(g.matrix, &g.point[i]);
		angle += inc;
	}
	glPushMatrix();
	glTranslatef(g.center.x, g.center.y, 0.0);
	glBegin(GL_LINE_LOOP);
	glColor3ub(255,255,90);
	for (int i=0; i<g.npoints; i++) {
		glVertex2f(g.point[i].x, g.point[i].y);
	}
	glEnd();
	glPopMatrix();


}

void render()
{
	glClear(GL_COLOR_BUFFER_BIT);
//	showMenu();
	switch (g.mode) {
		case 1:
			glColor3ub(30,60,90);
			showPointsOnACircle();

			break;
		case 2:
			rotationMatrix();
			break;
		case 3: 
			lab8Circle();

			break;
		case 5:
			triangleStrip();
			break;
		case 6:
			triangleFan();
			break;
		case 7:
			triangleStripRing();
			break;
		default:
			break;
	}
}























