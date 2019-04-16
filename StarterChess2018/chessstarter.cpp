#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <vector>
using namespace std;
#include "glut.h"
#include "graphics.h"
#include <ctime>


// Global Variables
// Some colors you can use, or make your own and add them
// here and in graphics.h
bool first = false;
enum PIECES { PAWN = 10, ROOK, KNIGHT, BISHOP, QUEEN, KING };
GLdouble redMaterial[] = { 0.7, 0.1, 0.2, 1.0 };
GLdouble greenMaterial[] = { 0.1, 0.7, 0.4, 1.0 };
GLdouble brightGreenMaterial[] = { 0.1, 0.9, 0.1, 1.0 };
GLdouble blueMaterial[] = { 0.1, 0.2, 0.7, 1.0 };
GLdouble whiteMaterial[] = { 1.0, 1.0, 1.0, 1.0 };

double screen_x = 600;
double screen_y = 500;

double GetTime() {
	static clock_t start_time = clock();
	clock_t end_time = clock();
	return (double)(end_time - start_time) / CLOCKS_PER_SEC;
}

//as t changes change v the same ratio
void Interpolate(double t, double t0, double t1,
	double &x, double x0, double x1, double x2,
	double &y, double y0, double y1, double y2) {
	double ratio = (t - t0) / (t1 - t0);
	if (ratio < 0)
		ratio = 0;
	if (ratio > 1)
		ratio = 1;
	x = x0 * (1 - ratio) * (1 - ratio) + x1 * (1 - ratio)*ratio * 2 + x2 * ratio*ratio;
	y = y0 * (1 - ratio) * (1 - ratio) + y1 * (1 - ratio)*ratio * 2 + y2 * ratio*ratio;
}
void Interpolate(double t, double t0, double t1,
	double & v, double v0, double v1)
{
	double ratio = (t - t0) / (t1 - t0);
	if (ratio < 0)
		ratio = 0;
	if (ratio > 1)
		ratio = 1;
	v = v0 + (v1 - v0)*ratio;
}

// Outputs a string of text at the specified location.
void text_output(double x, double y, const char *string)
{
	void *font = GLUT_BITMAP_9_BY_15;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	int len, i;
	glRasterPos2d(x, y);
	len = (int)strlen(string);
	for (i = 0; i < len; i++)
	{
		glutBitmapCharacter(font, string[i]);
	}

	glDisable(GL_BLEND);
}
void DrawSphere(float x, float y, float z, float radius)
{
	glPushMatrix();

	glTranslatef(x, y, z);
	int slices = 40;
	int stacks = 40;
	glutSolidSphere(radius, slices, stacks);
	glPopMatrix();
}
void DrawRectangle(double x1, double y1, double x2, double y2, bool white) {
	glBegin(GL_QUADS);
	if (white) {
		/*(255,248,220)*/
		/*rgb(245,222,179)*/
		GLfloat color[] = { 0.96078431372549019607843137254902f,0.87058823529411764705882352941176f, 0.70196078431372549019607843137255f, 1.0 };
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color);
	}
	else {
		/*rgb(139,69,19)*/
		GLfloat color[] = { 0.54509803921568627450980392156863f, 0.27058823529411764705882352941176f, 0.0f, 1.0 };
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color);
	}
	glVertex3d(x1, 0, y1);
	glVertex3d(x2, 0, y1);
	glVertex3d(x2, 0, y2);
	glVertex3d(x1, 0, y2);
	glEnd();
}
void DrawLine(double x1, double y1, double x2, double y2) {
	GLfloat color[] = { 0.5f,0.0f, 0.0f, 1.0 };
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color);
	glBegin(GL_LINES);
	glVertex3f(x1, y1, 0);
	glVertex3f(x2, y2, 0);
	glEnd();
}

// Given the three triangle points x[0],y[0],z[0],
//		x[1],y[1],z[1], and x[2],y[2],z[2],
//		Finds the normal vector n[0], n[1], n[2].
void FindTriangleNormal(double x[], double y[], double z[], double n[])
{
	// Convert the 3 input points to 2 vectors, v1 and v2.
	double v1[3], v2[3];
	v1[0] = x[1] - x[0];
	v1[1] = y[1] - y[0];
	v1[2] = z[1] - z[0];
	v2[0] = x[2] - x[0];
	v2[1] = y[2] - y[0];
	v2[2] = z[2] - z[0];

	// Take the cross product of v1 and v2, to find the vector perpendicular to both.
	n[0] = v1[1] * v2[2] - v1[2] * v2[1];
	n[1] = -(v1[0] * v2[2] - v1[2] * v2[0]);
	n[2] = v1[0] * v2[1] - v1[1] * v2[0];

	double size = sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
	n[0] /= -size;
	n[1] /= -size;
	n[2] /= -size;
}
void RatioSet(double currentTime, double time1, double time2, double &value, double value1, double value2)
{
	double ratio = (currentTime - time1) / (time2 - time1);
	if (ratio < 0)
	{
		ratio = 0;
	}
	if (ratio > 1)
	{
		ratio = 1;
	}
	value = value1 + ratio * (value2 - value1);
}
// Loads the given data file and draws it at its default position.
// Call glTranslate before calling this to get it in the right place.
void DrawPiece(const char filename[])
{
	// Try to open the given file.
	char buffer[200];
	ifstream in(filename);
	if (!in)
	{
		cerr << "Error. Could not open " << filename << endl;
		exit(1);
	}

	double x[100], y[100], z[100]; // stores a single polygon up to 100 vertices.
	int done = false;
	int verts = 0; // vertices in the current polygon
	int polygons = 0; // total polygons in this file.
	do
	{
		in.getline(buffer, 200); // get one line (point) from the file.
		int count = sscanf(buffer, "%lf, %lf, %lf", &(x[verts]), &(y[verts]), &(z[verts]));
		done = in.eof();
		if (!done)
		{
			if (count == 3) // if this line had an x,y,z point.
			{
				verts++;
			}
			else // the line was empty. Finish current polygon and start a new one.
			{
				if (verts >= 3)
				{
					glBegin(GL_POLYGON);
					double n[3];
					FindTriangleNormal(x, y, z, n);
					glNormal3dv(n);
					for (int i = 0; i<verts; i++)
					{
						glVertex3d(x[i], y[i], z[i]);
					}
					glEnd(); // end previous polygon
					polygons++;
					verts = 0;
				}
			}
		}
	} while (!done);

	if (verts>0)
	{
		cerr << "Error. Extra vertices in file " << filename << endl;
		exit(1);
	}

}

// NOTE: Y is the UP direction for the chess pieces.
double eyex = 4500;
double eyey = 11000;
double eyez = -6000;
double eyex2 = 4500;
double eyey2 = 0;
double eyez2 = 5000;

double eye[3] = { eyex, eyey, eyez }; // pick a nice vantage point.
double at[3] = { eyex2, eyey2, eyez2 };

/*double eye[3] = { -3500, 9000, -3500 }; // pick a nice vantage point.
double at[3] = { 4500, 0,     5000 };
*/
//

// GLUT callback functions
//

// This callback function gets called by the Glut
// system whenever it decides things need to be redrawn.
void DrawCircle(double x1, double y1, double radius)
{
	glBegin(GL_LINE_LOOP);
	for (int i = 0; i<32; i++)
	{
		double theta = (double)i / 32.0 * 2.0 * 3.1415926;
		double x = x1 + radius * cos(theta);
		double y = y1 + radius * sin(theta);
		glVertex2d(x, y);
	}
	glEnd();
}
void DrawBoard()
{
	//    glBegin(GL_QUADS);
	//    glVertex3d(0, -1000, 0);
	//    glVertex3d(0, -1000, 9000);
	//    glVertex3d(9000, -1000, 9000);
	//    glVertex3d(9000, -1000, 0);
	//    glEnd();
	/*	rgb(210,105,30)*/
	GLfloat black[] = { 0.82352941176470588235294117647059, 0.41176470588235294117647058823529, 0.11764705882352941176470588235294, 1.0 };
	/*	rgb(245,222,179)*/
	GLfloat white[] = { 0.96078431372549019607843137254902, 0.87058823529411764705882352941176, 0.70196078431372549019607843137255, 1.0 };
	/*rgb(128,0,0)
	145,41,3 */
	GLfloat brown[] = { 0.56862745098039215686274509803922, 0.16078431372549019607843137254902, 0.01176470588235294117647058823529, 1.0 };
	bool oddCol = false;
	bool oddRow = false;
	for (int row = 500; row<10000; row += 1000)
	{
		for (int col = 500; col<10000; col += 1000)
		{
			if (col == 500 || row == 500 || col == 9500 || row == 9500) {
				glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, brown);
			}
			else  if (!oddRow)
			{
				// even row
				if (!oddCol)
				{
					// even col
					glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, white);

				}
				else
				{
					// odd col
					glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, black);
				}
			}
			else
			{
				// odd row
				if (!oddCol)
				{
					// even col
					glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, black);

				}
				else
				{
					// odd col
					glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, white);
				}
			}
			glBegin(GL_POLYGON);
			glNormal3f(0, 1, 0);
			glVertex3d(col, 0, row);
			glVertex3d(col, 0, row + 1000);
			glVertex3d(col + 1000, 0, row + 1000);
			glVertex3d(col + 1000, 0, row);
			glEnd();
			glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, brown);
			glBegin(GL_POLYGON);
			glNormal3f(0, 0, -1);
			glVertex3d(col, 0, row);
			glVertex3d(col, -1000, row);
			glVertex3d(col + 1000, -1000, row);
			glVertex3d(col + 1000, 0, row);
			glEnd();

			oddCol = !oddCol;
		}
		oddRow = !oddRow;
	}
}
void move() {
	/**/
	double z;
	double x;
	double t = GetTime();

	Interpolate(t, 1.0, 3.0, z, 2000, 5000);
	

	


}
void display(void)
{
	double t = GetTime();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glLoadIdentity();
	gluLookAt(eye[0], eye[1], eye[2], at[0], at[1], at[2], 0, 1, 0); // Y is up!

																	 //draw the board
	const int spaceSize = 1000;
	DrawBoard();

	// Set the color for one side (white), and draw its 16 pieces.

	// Set the color for one side (white), and draw its 16 pieces.
	/*White chess*/
	GLfloat mat_amb_diff1[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_amb_diff1);

	double rook_x = 2000;
	double rook_x2 = 2400;
	double rook_y = 0;
	double rook_z = 2000;
	double rook_z2 = 2600;
	double rook_z3 = 3600;
	double bomb_z = 4800;
	double bomb_y = 420;
	double rook_x3 = 5400;
	double rook_y2 = 400;



	glPushMatrix();
	glTranslatef(9000, 0, 2000);
	//DrawPiece("ROOK.POL");
	glCallList(ROOK);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(8000, 0, 2000);
	//DrawPiece("KNIGHT.POL");
	glCallList(KNIGHT);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(7000, 0, 2000);
	//DrawPiece("BISHOP.POL");
	glCallList(BISHOP);
	glPopMatrix();


	double z;
	double x;

	glPushMatrix();
	Interpolate(t, 11.5, 12.0, x, 5000, 5000, 5000, z, 2000, 1500, 1000);
	glTranslatef(x, 0, z);
	DrawPiece("KING.POL");
	glCallList(KING);
	glPopMatrix();


	/*(time,diffence how fast, empty?, startlocation x, new location forward )*/
	//Interpolate(t, 1.0, 2.5, z, 2000, 4000);
	//Interpolate(t, 1.0, 2.5, x,5000, 5000, 6000, z, 2000, 2000, 5000);

	glPushMatrix();
	glTranslatef(6000, 0, 2000);
	//DrawPiece("QUEEN.POL");
	glCallList(QUEEN);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(4000, 0, 2000);
	glCallList(BISHOP);
	DrawPiece("BISHOP.POL");
	glPopMatrix();

	glPushMatrix();
	glTranslatef(3000, 0, 2000);
	DrawPiece("KNIGHT.POL");
	glCallList(KNIGHT);
	glPopMatrix();

	/*
	Interpolate(t, 9.0, 10.0, x, 1000, 4500, 8000, z, 1000, 4500, 8000);
	glPushMatrix();
	glTranslatef(x, 1000, z);
	glCallList(rook);
	glPopMatrix();
	*/
	/*Time, x start location, change, y start, change*/
	//Interpolate(t, 0.0, 2.0, x, 2000, 4000, 5000, z, 2000, 4000, 5000);
	glPushMatrix();
	glTranslatef(2000, 0, 2000);
	//DrawPiece("ROOK.POL");
	glCallList(ROOK);
	glPopMatrix();


	glPushMatrix();
	glTranslatef(2000, 0, 3000);


	glCallList(PAWN);
	//DrawPiece("PAWN.POL");
	glPopMatrix();

	/*Time, x start location, change, y start, change*/
	Interpolate(t, 6.0, 8.0, x, 3000, 3000, 3000, z, 3000, 3500, 5000);
	glPushMatrix();
	glTranslatef(x, 0, z);
	//DrawPiece("PAWN.POL");
	glCallList(PAWN);
	glPopMatrix();

	Interpolate(t, 0.0, 2.0, x, 4000, 4000, 4000, z, 3000, 3500, 5000);
	glPushMatrix();
	glTranslatef(x, 0, z);
	//DrawPiece("PAWN.POL");
	glCallList(PAWN);
	glPopMatrix();
	for (int x = 5000; x <= 9000; x += 1000)
	{
	
		cout << x << endl;
		glPushMatrix();
		glTranslatef(x, 0, 3000);
		glCallList(PAWN);
		DrawPiece("PAWN.POL");
		glPopMatrix();
	}

	

	

	// Set the color for one side (black), and draw its 16 pieces.
	GLfloat mat_amb_diff2[] = { 0.3f, 0.3f, 0.3f, 1.0 };
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_amb_diff2);

	/*double x;
	Interpolate(t, 4.0, 6.0, x, 4000, 2000);
	glPushMatrix();
	glTranslatef(x, 0, 8000);
	DrawPiece("KING.POL");
	glPopMatrix();
	*/
	for (int x = 2000; x <= 9000; x += 1000)
	{
		if (x != 5000) {
			glPushMatrix();
			glTranslatef(x, 0, 8000);
			glCallList(PAWN);
			// DrawPiece("PAWN.POL");
			glPopMatrix();
		}

	}
	Interpolate(t, 2.5, 4.0, x, 5000, 5000, 5000, z, 8000, 7000, 6000);
	glPushMatrix();
	glTranslatef(x, 0, z);
	glCallList(PAWN);
	//DrawPiece("PAWN.POL");
	glPopMatrix();
	glPushMatrix();
	glTranslatef(9000, 0, 9000);
	glCallList(ROOK);
	//DrawPiece("ROOK.POL");
	glPopMatrix();

	glPushMatrix();
	glTranslatef(8000, 0, 9000);
	glCallList(KNIGHT);
	//DrawPiece("KNIGHT.POL");
	glPopMatrix();

	glPushMatrix();
	glTranslatef(7000, 0, 9000);
	glCallList(BISHOP);
	//DrawPiece("BISHOP.POL");
	glPopMatrix();

	glPushMatrix();
	glTranslatef(5000, 0, 9000);
	glCallList(KING);
	//DrawPiece("KING.POL");
	glPopMatrix();

	
	
	//glTranslatef(5000, 0, 2000);
	
	glPushMatrix();
	Interpolate(t, 8.0, 10.0, x, 4000, 1000, 0, z, 4000, 2000, 0);
	glTranslatef(x, 0, z);

	//DrawPiece("QUEEN.POL");
	
	Interpolate(t, 11.0, 12.0, x, 2000, 3000, 5000, z, 5000, 3000, 2000);
	
	glTranslatef(x, 0, z);
	glCallList(QUEEN);
	//DrawPiece("QUEEN.POL");
	glPopMatrix();



	


	glPushMatrix();
	glTranslatef(4000, 0, 9000);
	glCallList(BISHOP);
	//DrawPiece("BISHOP.POL");
	glPopMatrix();

	glPushMatrix();
	glTranslatef(3000, 0, 9000);
	glCallList(KNIGHT);
	//DrawPiece("KNIGHT.POL");
	glPopMatrix();

	glPushMatrix();
	glTranslatef(2000, 0, 9000);
	//DrawPiece("ROOK.POL");
	glCallList(ROOK);
	glPopMatrix();

	GLfloat light_position[] = { 1,2,-.1f, 0 }; // light comes FROM this vector direction.
	glLightfv(GL_LIGHT0, GL_POSITION, light_position); // position first light
	move();
	glutSwapBuffers();
	glutPostRedisplay();
}

void SetPerspectiveView(int w, int h)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	double aspectRatio = (GLdouble)w / (GLdouble)h;
	gluPerspective(
		/* field of view in degree */ 45.0,
		/* aspect ratio */ aspectRatio,
		/* Z near */ 100, /* Z far */ 30000.0);
	glMatrixMode(GL_MODELVIEW);
}

// This callback function gets called by the Glut
// system whenever the window is resized by the user.

void PieceList(PIECES e, char filename[])
{
	glNewList(e, GL_COMPILE);
	DrawPiece(filename);
	glEndList();
}

void reshape(int w, int h)
{
	screen_x = w;
	screen_y = h;

	// Set the pixel resolution of the final picture (Screen coordinates).
	glViewport(0, 0, w, h);

	SetPerspectiveView(w, h);

}

// This callback function gets called by the Glut
// system whenever any mouse button goes up or down.
void mouse(int mouse_button, int state, int x, int y)
{
	if (mouse_button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
	}
	if (mouse_button == GLUT_LEFT_BUTTON && state == GLUT_UP)
	{
	}
	if (mouse_button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN)
	{
	}
	if (mouse_button == GLUT_MIDDLE_BUTTON && state == GLUT_UP)
	{
	}
	glutPostRedisplay();
}
void keyboard(unsigned char c, int x, int y)
{
	switch (c)
	{
	case 27: // escape character means to quit the program
		exit(0);
		break;
	default:
		return; // if we don't care, return without glutPostRedisplay()
	}

	glutPostRedisplay();
}
// Your initialization code goes here.
void InitializeMyStuff()
{
	// set material's specular properties
	GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat mat_shininess[] = { 50.0 };
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

	// set light properties
	GLfloat light_position[] = { (float)eye[0], (float)eye[1], (float)eye[2],1 };
	GLfloat white_light[] = { 1,1,1,1 };
	GLfloat low_light[] = { .3f,.3f,.3f,1 };
	glLightfv(GL_LIGHT0, GL_POSITION, light_position); // position first light
	glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light); // specify first light's color
	glLightfv(GL_LIGHT0, GL_SPECULAR, low_light);

	glEnable(GL_DEPTH_TEST); // turn on depth buffering
	glEnable(GL_LIGHTING);	// enable general lighting
	glEnable(GL_LIGHT0);	// enable the first light.

	PieceList(PAWN, "PAWN.POL");
	PieceList(BISHOP, "BISHOP.POL");
	PieceList(ROOK, "ROOK.POL");
	PieceList(KING, "KING.POL");
	PieceList(KNIGHT, "KNIGHT.POL");
	PieceList(QUEEN, "QUEEN.POL");
}


int main(int argc, char **argv)
{
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(screen_x, screen_y);
	glutInitWindowPosition(10, 10);

	int fullscreen = 0;
	if (fullscreen)
	{
		glutGameModeString("800x600:32");
		glutEnterGameMode();
	}
	else
	{
		glutCreateWindow("Shapes");
	}

	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutReshapeFunc(reshape);
	glutMouseFunc(mouse);

	glClearColor(1, 1, 1, 1);
	InitializeMyStuff();

	glutMainLoop();

	return 0;
}