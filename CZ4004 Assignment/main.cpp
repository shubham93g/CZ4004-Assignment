//
//  main.cpp
//  CZ4004 Assignment
//
//  Created by Shubham Goyal on 9/10/14.
//  Copyright (c) 2014 Shubham Goyal. All rights reserved.
//

#include <iostream>
#include <stdio.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#include <vector>
#include <math.h>

using namespace std;

#define TRANSFORM_NONE      0
#define TRANSFORM_ROTATE    1
#define TRANSFORM_SCALE     2

#define OBJ_WIREFRAME	0
#define OBJ_SOLID		1
#define OBJ_POINT		2

#define VIEW_PERS   0
#define VIEW_ORTH   1

#define SOLID_SMOOTH 0
#define SOLID_FLAT 1

struct Vertex
{
    double x, y, z; // vertex coordinates
    double nx, ny, nz; // normal vector
};

struct Face
{
    int v1, v2, v3; // vertex indices
};

vector<Vertex> vertices; // array of the vertices
vector<Face> faces; // array of the triangles

string const folder = "/Users/shubhamgoyal/Google Drive/Y4S1/CZ4004/Assignment/Models/";
string const fileNames[5] = {"bunny.m","gargoyle.m","knot.m","eight.m","cap.m"};

char const* WINDOW_TITLE = "U1122584C CZ4004 Assignment";
int WINDOW_WIDTH = 640;
int WINDOW_HEIGHT = 640;

//variable to limit the XZ grid plane
float FIELD_LIMIT;

//size of the units on XZ grid plane
float UNIT_SIZE;

//minimum discrete change in camera angle
float camChange;

//factor used to compute camera distance from largest vertex
float camFactor;

//change in camera's angle on XZ plane
float lx=0.0f,lz=0.0f;

// XYZ position change of the camera
float dx,dy,dz;

//XYZ position of camera
float camX, camY, camZ;

//Variables for mouse callbacks
static int press_x, press_y;
static float x_angle;
static float y_angle;
static float scale_size;

//variables for assisting in rendering modes
static int obj_mode = 0;
static int view_mode = 0;
static int solid_mode = 0;
static int xform_mode;

//max and min vertices of loaded mesh
float minVertices[3];
float maxVertices[3];

//perspective projection; far, near
float persZn = 0.0001;
float persZf = 1000.0;

//orthogonal projection; far, near
float orthoZn = 0.0001;
float orthoZf = 1000.0;

//------------Function prototyping------------

//Initialization functions
void InitializeVariables();
void InitializeDisplay();
void InitRendering();
void loadMFile(string,string);

//Callback functions
void processNormalKeys(unsigned char, int, int);
void processSpecialKeys(int,int,int);
void myMouse(int,int,int,int);
void myMotion(int,int);
void reshape(int,int);
void display();

//Drawing functions
void drawText(float,float,float,string);
void drawText(float[3],string);
void drawLightSource();
void drawBoundingBox();
void drawGrid();
void objectAsPoints();
void objectAsTriangles();
void drawObject();

//Support functions
string vectorToStr(float[3]);

//------------main------------

int main(int argc, char **argv) {
    
    //Load File
    loadMFile(folder,fileNames[0]);
    
    // init GLUT and create window
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(0, 0);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    
    //Create Windows
    glutCreateWindow(WINDOW_TITLE);
    
    InitRendering();
    
    // register callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(display);
    glutMouseFunc(myMouse);
    glutMotionFunc(myMotion);
    glutKeyboardFunc(processNormalKeys);
    glutSpecialFunc(processSpecialKeys);
    
    // enter GLUT event processing cycle
    glutMainLoop();
    
    return 0;
}

//------------Initialization functions------------

//Initialize variables to default values
void InitializeVariables(){
    vertices.clear();
    faces.clear();
    maxVertices[0]=0;
    maxVertices[1]=0;
    maxVertices[2]=0;
    
    minVertices[0]=0;
    minVertices[1]=0;
    minVertices[2]=0;
    
    FIELD_LIMIT = 100;
    UNIT_SIZE = 1.0;
    camChange = 0.1;
    
    camFactor=0.5;
    
    // actual vector representing the camera's direction
    lx=0.0f,lz=0.0f;
    
    // XYZ position of the camera
    dx=0.0f,dy=0.0f,dz=0.0f;
    camX = 0.0, camY = 0.0; camZ = 0.0;
    
    x_angle = 0.0;
    y_angle = 0.0;
    scale_size = 1;
    
    obj_mode = obj_mode;
    view_mode = view_mode;
    solid_mode = solid_mode;
    xform_mode = 0;
    
}

//Initialize display, including projection settings
void InitializeDisplay(){
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    
    switch (view_mode) {
        case VIEW_PERS:
            gluPerspective(45.0, WINDOW_WIDTH/WINDOW_HEIGHT, persZn, persZf);
            break;
            
        case VIEW_ORTH:
            glOrtho(-maxVertices[0]-dx, maxVertices[0]+dx , -maxVertices[0]-dy, maxVertices[0]+dy, orthoZn, orthoZf);
            break;
    }
    
    glMatrixMode(GL_MODELVIEW);
}

//Initialize rendering settings
void InitRendering(){
    // OpenGL init
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    
    switch(solid_mode){
        case SOLID_FLAT:
            glShadeModel(GL_FLAT);
            break;
        case SOLID_SMOOTH:
            glShadeModel(GL_SMOOTH);
            break;
    }
}

//Load M file and complete initialization of all vertex related data
void loadMFile(string folder, string fileName)
{
    InitializeVariables();
    const char* filePath = (folder + fileName).c_str();
    cout<<"Loading file "<<filePath<<endl;
    
    FILE *f;
    f = std::fopen(filePath,"r");
    
    char type;
    float x, y, z, nx, ny, nz;
    int vid, fid, v1, v2, v3;
    Vertex *vertex;
    Face *face;
    
    while ((type=fgetc(f)) != EOF)
    {
        switch (type)
        {
            case 'V': // it’s a vertex line
                if (fscanf(f,"ertex %d %f %f %f {normal=(%f %f %f)}\n", &vid, &x, &y, &z, &nx, &ny, &nz) == 7)
                {
                    vertex = new Vertex();
                    vertex->x = x;
                    vertex->y = y;
                    vertex->z = z;
                    vertex->nx = nx;
                    vertex->ny = ny;
                    vertex->nz = nz;
                    
                    if(vertices.size()==0){
                        minVertices[0] = x;
                        maxVertices[0] = x;
                        minVertices[1] = y;
                        maxVertices[1] = y;
                        minVertices[2] = z;
                        maxVertices[2] = z;
                    }
                    
                    //min vertices
                    if(x<minVertices[0])
                        minVertices[0] = x;
                    if(y<minVertices[1])
                        minVertices[1] = y;
                    if(z<minVertices[2])
                        minVertices[2] = z;
                    
                    //max vertices
                    if(x>maxVertices[0])
                        maxVertices[0] = x;
                    if(y>maxVertices[1])
                        maxVertices[1] = y;
                    if(z>maxVertices[2])
                        maxVertices[2] = z;
                    
                    // save the vertex into the vertex array
                    vertices.push_back(*vertex);
                }
                break;
                
            case 'F': // it’s a face line
                if (fscanf(f, "ace %d %d %d %d\n", &fid, &v1, &v2, &v3) == 4)
                {
                    face = new Face();
                    
                    //Note: Indexes have been changed to -1 because array starts from 0 while index in file starts from 1
                    face->v1 = v1-1;
                    face->v2 = v2-1;
                    face->v3 = v3-1;
                    
                    // save the triangle into the face array
                    faces.push_back(*face);
                }
                break;
            default: // ignore the comments
                do
                {
                    type=fgetc(f);
                }while (type != EOF && type != '\n');
                break;
        }
    }
    
    float max;
    if (maxVertices[0]>maxVertices[2])
        max = maxVertices[0];
    else
        max = maxVertices[2];
    
    FIELD_LIMIT = max+1;
    camChange = max/100;
    
    cout<<"Load complete"<<endl;
    cout<<"Total Vertices "<<vertices.size()<<endl;
    cout<<"Total Faces "<<faces.size()<<endl;
    
    // force the redraw function
    glutPostRedisplay();
}


//------------Callback functions------------

//Keyboard input
void processNormalKeys(unsigned char key, int x, int y) {
    
    switch(key)
    {
        //escape -> exit
        case 27:
            cout<< "Pressed Escape. Exiting program\n";
            exit(0);
            
        //w or W -> render wireframe
        case 87:
        case 119:
            cout << "key 'w' is pressed! draw the object in wireframe" << endl;
            obj_mode = OBJ_WIREFRAME;
            break;
            
        //s or S -> render solid object. Toggle smooth/flat
        case 83:
        case 115:
            cout << "key 's' is pressed! draw the object in solid" << endl;
            obj_mode = OBJ_SOLID;
            if(solid_mode==SOLID_FLAT){
                solid_mode = SOLID_SMOOTH;
                cout << "Mode: Smooth" << endl;
            }
            else{
                solid_mode = SOLID_FLAT;
                cout << "Mode: Flat" << endl;
            }
            InitRendering();
            break;
            
        //e or E -> render point cloud
        case 69:
        case 101:
            cout << "key 'e' is pressed! draw the object in point mode" << endl;
            obj_mode = OBJ_POINT;
            break;
        
        //v or V -> Toggle projection/view mode
        case 86:
        case 118:
            cout << "key 'v' is pressed! Toggling view mode" << endl;
            if(view_mode==VIEW_ORTH){
                view_mode = VIEW_PERS;
                cout << "Mode: Perspective" << endl;
            }
            else{
                view_mode = VIEW_ORTH;
                cout << "Mode: Orthogonal" << endl;
            }
            InitializeDisplay();
            break;
            
        //1-5 -> load one of the 5 files
        case 49:
            persZn = 0.0001;
            persZf = 1000.0;
            orthoZn = 0.000000001;
            orthoZf = 1000.0;
            loadMFile(folder,fileNames[key-49]);
            InitializeDisplay();
            break;
        case 50:
            persZn = 1.0;
            persZf = 40000.0;
            orthoZn = 0.1;
            orthoZf = 40000.0;
            loadMFile(folder,fileNames[key-49]);
            InitializeDisplay();
            break;
        case 51:
        case 52:
        case 53:
            persZn = 0.001;
            persZf = 10000.0;
            orthoZn = 0.001;
            orthoZf = 10000.0;
            loadMFile(folder,fileNames[key-49]);
            InitializeDisplay();
            break;
    }
    
    // force the redraw function
    glutPostRedisplay();
}

void processSpecialKeys(int key, int xx, int yy) {
    
    switch (key) {
        case GLUT_KEY_LEFT : //pan left
            lx -= camChange;
            lz -= -camChange;
            break;
        case GLUT_KEY_RIGHT : //pan right
            lx += camChange;
            lz += -camChange;
            break;
        case GLUT_KEY_UP : //zoom in
            dx -= camX/100;
            dy -= camY/100;
            dz -= camZ/100;
            InitializeDisplay();
            break;
        case GLUT_KEY_DOWN : //zoom out
            dx += camX/100;
            dy += camY/100;
            dz += camZ/100;
            InitializeDisplay();
            break;
    }
}

//Mouse input
void myMouse(int button, int state, int x, int y)
{
    if (state == GLUT_DOWN)
    {
        press_x = x; press_y = y;
        if (button == GLUT_LEFT_BUTTON)
            xform_mode = TRANSFORM_ROTATE;
        else if (button == GLUT_RIGHT_BUTTON)
            xform_mode = TRANSFORM_SCALE;
    }
    else if (state == GLUT_UP)
    {
        xform_mode = TRANSFORM_NONE;
    }
}

//mouse motion callback
void myMotion(int x, int y)
{
    if (xform_mode == TRANSFORM_ROTATE)
    {
        x_angle += (x - press_x)/5.0;
        
        if (x_angle > 180)
            x_angle -= 360;
        else if (x_angle <-180)
            x_angle += 360;
        
        press_x = x;
        
        y_angle += (y - press_y)/5.0;
        
        if (y_angle > 180)
            y_angle -= 360;
        else if (y_angle <-180)
            y_angle += 360;
        
        press_y = y;
    }
    else if (xform_mode == TRANSFORM_SCALE)
    {
        float old_size = scale_size;
        
        scale_size *= (1 + (y - press_y)/60.0);
        
        if (scale_size <0)
            scale_size = old_size;
        press_y = y;
    }
    
    // force the redraw function
    glutPostRedisplay();
}

//Window reshaping callback
void reshape(int width, int height)
{
    if (height == 0)
        height = 1;
    
    WINDOW_HEIGHT = height;
    WINDOW_WIDTH = width;
    
    InitializeDisplay();
}

//Display callback
void display()
{
    // Clear Color and Depth Buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Reset transformations
    glLoadIdentity();
    
    // Set the camera
    camX = maxVertices[0]/camFactor;
    camY = maxVertices[1]/camFactor;
    camZ = maxVertices[2]/camFactor;
    gluLookAt(dx + camX, dy+ camY, dz+ camZ, //place camera to cover entire object; dx dy dz for camera movements
              lx,0.0f,lz, //lx,lz for camera movements
              0.0f, 1.0f, 0.0f);
    
    //rotate object
    glTranslatef((minVertices[0]+maxVertices[0])/2, (minVertices[1]+maxVertices[1])/2, (minVertices[2]+maxVertices[2])/2);
    glRotatef(x_angle, 0, 1,0);
    glRotatef(y_angle, 1,0,0);
    glTranslatef(-(minVertices[0]+maxVertices[0])/2, -(minVertices[1]+maxVertices[1])/2, -(minVertices[2]+maxVertices[2])/2);
    
    drawLightSource();
    drawBoundingBox();
    drawObject();
    drawGrid();
    
    glutSwapBuffers();
    
}

//------------Drawing functions------------

//Draw user-facing text at XYZ position
void drawText(float posX, float posY, float posZ, string text){
    glColor3f(1.0, 1.0, 1.0);
    glRasterPos3f(posX, posY, posZ);
    for(int i=0;i<text.length();i++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, text[i]);
}

//draw text - XYZ position supplied as a vector
void drawText(float vec[3], string text){
    drawText(vec[0], vec[1], vec[2], text);
}

//Configure lightsource
void drawLightSource(){
    
    GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 }; // light position
    
    GLfloat white_light[] = { 1.0, 1.0, 1.0, 1.0 }; // light color
    GLfloat lmodel_ambient[] = { 1, 0.1, 0.1, 1.0 };
    
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light);
    glLightfv(GL_LIGHT0, GL_SPECULAR, white_light);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);}

//draw bounding box around object
void drawBoundingBox(){
    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    glBegin(GL_QUADS);
    glColor3f(0.8f, 0.8f, 0.8f);
    glVertex3f(minVertices[0],minVertices[1],minVertices[2]);
    glVertex3f(maxVertices[0],minVertices[1],minVertices[2]);
    glVertex3f(maxVertices[0],maxVertices[1],minVertices[2]);
    glVertex3f(minVertices[0],maxVertices[1],minVertices[2]);
    glVertex3f(minVertices[0],minVertices[1],maxVertices[2]);
    glVertex3f(maxVertices[0],minVertices[1],maxVertices[2]);
    glVertex3f(maxVertices[0],maxVertices[1],maxVertices[2]);
    glVertex3f(minVertices[0],maxVertices[1],maxVertices[2]);
    glEnd();
    
    glBegin(GL_LINES);
    glVertex3f(minVertices[0],minVertices[1],minVertices[2]);
    glVertex3f(minVertices[0],minVertices[1],maxVertices[2]);
    
    glVertex3f(maxVertices[0],minVertices[1],minVertices[2]);
    glVertex3f(maxVertices[0],minVertices[1],maxVertices[2]);
    
    glVertex3f(maxVertices[0],maxVertices[1],minVertices[2]);
    glVertex3f(maxVertices[0],maxVertices[1],maxVertices[2]);
    
    glVertex3f(minVertices[0],maxVertices[1],maxVertices[2]);
    glVertex3f(minVertices[0],maxVertices[1],minVertices[2]);
    glEnd();
    
    drawText(maxVertices, vectorToStr(maxVertices));
}

//draw coordinate axes and supporting XZ plane grid
void drawGrid(){
    
    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    
    //Coordinate axes
    float
    ORG[3] = {0,0,0},
    XP[3] = {FIELD_LIMIT,0,0},
    XN[3] = {-FIELD_LIMIT,0,0},
    YP[3] = {0,FIELD_LIMIT,0},
    YN[3] = {0,-FIELD_LIMIT,0},
    ZP[3] = {0,0,FIELD_LIMIT},
    ZN[3] = {0,0,-FIELD_LIMIT};
    

    glLineWidth(3.0);
    glBegin(GL_LINES);
    
    //pos X; red
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3fv(ORG);
    glVertex3fv(XP);
    
    //pos Y; green
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3fv(ORG);
    glVertex3fv(YP);
    
    //pos Z; blue
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3fv(ORG);
    glVertex3fv(ZP);
    
    glColor3f(0.3f, 0.3f, 0.3f);
    
    //neg X
    glVertex3fv(XN);
    glVertex3fv(ORG);
    
    //neg Y
    glVertex3fv(YN);
    glVertex3fv(ORG);
    
    //neg Z
    glVertex3fv(ZN);
    glVertex3fv(ORG);
    glEnd();
    
    glLineWidth(1.0);
    glBegin(GL_LINES);
    glColor3f(0.4f, 0.4f, 0.4f);
    
    //floor on XZ plane
    for (float i=-int(FIELD_LIMIT ); i<=int(FIELD_LIMIT); i+=UNIT_SIZE) {
        if(i==0)
            continue;
        XN[2] = i;
        XP[2] = i;
        ZN[0] = i;
        ZP[0] = i;
        
        glVertex3fv(XN);
        glVertex3fv(XP);
        
        glVertex3fv(ZN);
        glVertex3fv(ZP);
    }
    
    glEnd( );
    
    drawText(maxVertices[0]*0.75, 0,0, "X");
    drawText(0,maxVertices[1]*0.75,0, "Y");
    drawText(0,0,maxVertices[2]*0.75, "Z");
    
    
}

//render object as points
void objectAsPoints()
{
    glPointSize(2.0);
    glBegin(GL_POINTS);
    glColor3f(1.0f, 0.85f, 0.35f);
    
    for (int i =0; i<vertices.size(); i++){
        glVertex3f((float)vertices[i].x, (float)vertices[i].y, (float)vertices[i].z);
        glNormal3f((float)vertices[i].nx, (float)vertices[i].ny, (float)vertices[i].nz);
    }
    glEnd();
}

//render object as triangles
void objectAsTriangles()
{
    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 0.85f, 0.35f);
    
    for (int i=0; i<faces.size(); i++){
        glNormal3f(vertices[faces[i].v1].nx, vertices[faces[i].v1].ny, vertices[faces[i].v1].nz);
        glVertex3f(vertices[faces[i].v1].x, vertices[faces[i].v1].y, vertices[faces[i].v1].z);
        
        glNormal3f(vertices[faces[i].v2].nx, vertices[faces[i].v2].ny, vertices[faces[i].v2].nz);
        glVertex3f(vertices[faces[i].v2].x, vertices[faces[i].v2].y, vertices[faces[i].v2].z);
        
        glNormal3f(vertices[faces[i].v3].nx, vertices[faces[i].v3].ny, vertices[faces[i].v3].nz);
        glVertex3f(vertices[faces[i].v3].x, vertices[faces[i].v3].y, vertices[faces[i].v3].z);
    }
    
    glEnd();
}

//draw the object
void drawObject()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    switch (obj_mode) {
        case OBJ_POINT:
            objectAsPoints();
            glDisable(GL_LIGHTING);
            glDisable(GL_LIGHT0);
            return;
            
        case OBJ_WIREFRAME:
            glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
            objectAsTriangles();
            break;
            
        case OBJ_SOLID:
            glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
            objectAsTriangles();
            break;
    }
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    
}

//------------Support functions------------

//convert vector to string in format "float,float,float"
string vectorToStr(float vec[3]){
    return string(to_string(vec[0]) + "\n" + to_string(vec[1]) + "\n" + to_string(vec[2]) );
}


