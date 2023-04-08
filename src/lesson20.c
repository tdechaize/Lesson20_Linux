/*
=====================================================================
OpenGL Lesson 20 :  Creating Another OpenGL Window with GLX on Linux
=====================================================================

  Authors Name: Jeff Molofee ( NeHe )

  This code was created by Jeff Molofee '99 (modified heavily by Daniel Davis)

  If you've found this code useful, please let me know.

  Visit Jeff at http://nehe.gamedev.net/

  It was modified heavily by Daniel Davis to get rid of Glut (Blah!) and the Tab characters (double Blah!)
  Daniel (planetes@mediaone.net)
  I should note that this was completed on a custom Linux (see www.linuxfromscratch.org)
  using XFree86 4.0.1 with DRI cvs code and a 3dfx Voodoo3 card.

  Disclaimer:

  This program may crash your system or run poorly depending on your
  hardware.  The program and code contained in this archive was scanned
  for virii and has passed all test before it was put online.  If you
  use this code in project of your own, send a shout out to the author!

=====================================================================
                NeHe Productions 1999-2004
=====================================================================
*/

 /*******************************************************************
 *  Project: $(project)
 *  Function : Main program
 ***************************************************************
 *  $Author: Jeff Molofee 2000 ( NeHe )
 *  $Name:  $
 ***************************************************************
 *
 *  Copyright NeHe Production
 *
 *******************************************************************/

/**         Comments manageable by Doxygen
*
*  Modified smoothly by Thierry DECHAIZE
*
*  Paradigm : obtain one source (only one !) compatible for multiple free C Compilers
*    and provide for all users an development environment on Linux (64 bits compatible),
*    the great Code::Blocks manager (version 20.03), and don't use glaux.lib or glaux.dll.
*
*	a) gcc 11.3.0 (32 and 64 bits) version officielle : commande pour l'installation sur Linux Mint
*       -> sudo apt-get install freeglut3 libglew-dev gcc g++ mesa-common-dev build-essential libglew2.2 libglm-dev binutils libc6 libc6-dev gcc-multilib g++-multilib; option -m32 to 32 bits ; no option to 64 bits
*	b) CLANG version 14.0.6 (32 et 64 bits), adossé aux environnements gcc : commande pour l'installation sur Linux Mint
*       -> sudo apt-get install llvm clang ; options pour la compilation et l'édition de liens -> --target=i686-pc-linux-gnu (32 bits) --target=x86_64-pc-linux-gnu (64 bits)
*	c) Mingw 32 ou 64 bits version gcc version 10-win32 20220113 : commande pour l'installation sur Linux Mint (NOT YET ACTIVED - Work in progress : to verify portability of these code)
*        -> sudo apt-get install mingw64    (ATTENTION, il s'agit d'une cross génération : l'exécutable créé ne fonctionne que sur Windows !!!)
*
*  Date : 2023/04/08
*
* \file            lesson20.c
* \author          Jeff Molofee ( NeHe ) originely, podified heavily by Daniel Davis, and after by Thierry Dechaize on Linux Mint
* \version         2.0.1.0
* \date            8 avril 2023
* \brief           Ouvre une simple fenêtre X11 on Linux et affiche sur une trame mouvante dans l'écran avec des chaînes de caractères (NeHe, GameDev) issus de fichier BMP qui se déplacent.
* \details         Ce programme gére plusieurs événements : le clic sur le bouton "Fermé" de la fenêtre, la sortie du programme par la touche ESCAPE ou par les touches "q" ou "Q" [Quit]
* \details                                                  les touches "m" ou "M" qui active ou non le "masking", les touches "f" ou "F" qui active ou non le "Full Screen".
* \details                                                  et enfin la touche "Espace" qui permet de changer de fond pour la scène affichée par OpenGL.
*
*  Point d'attention : il se faut se positionner à l'endroit de l'exécutable pour pouvoir le lancer (car chemins relatifs des différents fichiers recherchés en lecture sous ../../Data/...)
*
*/

#include <X11/Xlib.h>   // Standard X header for X libraries
#include <X11/Xatom.h>  // Header to provide X's Atom functionality
#include <X11/keysym.h> // Header to provide keyboard functionality under X
#include <GL/gl.h>	    // Header File For The OpenGL32 Library
#include <GL/glx.h>     // Header File For The X library for OpenGL
#include <GL/glu.h>	    // Header File For The GLu32 Library

#define _XOPEN_SOURCE   600  // Needed because use of function usleep depend of these two define ...!!! However function usleep appear like "... warning: implicit declaration of ..."
#include <unistd.h>     // Header file for sleeping (function usleep)
#include <stdio.h>      // Header file needed by use of printf in this code
#include <string.h>     // Header file needed by use of strcmp in this code
#include <stdlib.h>     // Header file needed by use of malloc/free function in this code

#include "logger.h"     //  added by Thierry DECHAIZE : logger for trace and debug if needed ... only in mode Debug !!!

char *level_debug;    // added by Thierry DECHAIZE, needed in test of logging activity (only with DEBUG mode)

/* Buffer parameters for Double Buffering */
static int dblBuf[] = {GLX_RGBA,
                       GLX_RED_SIZE, 1,
                       GLX_GREEN_SIZE, 1,
                       GLX_BLUE_SIZE, 1,
                       GLX_DEPTH_SIZE, 12,
                       GLX_DOUBLEBUFFER,
                       None};

/* Global variables */
Atom    wmDeleteWindow; // Custom message to Delete Window
Display *dpDisplay;     // Display variable
Window  win;            // Current Window variable
Bool    masking;        // Masking On/Off
Bool    scene;          // Which Scene To Draw

XWindowAttributes window_attr;  // Attributs of X11 window

/* Indicator of Full Screen */
int     nFullScreen=0;

GLuint  iTexture[5];    // Storage For Our Five Textures
GLuint  loop;           // Generic Loop Variable

GLfloat roll;           // Rolling Texture

/* Image type - contains height, width, and data */
struct Texture {
    unsigned long ulDimensionX;
    unsigned long ulDimensionY;
    char *pcData;
};
typedef struct Texture Texture;

void go_fullscreen(Display *dsp, Window win)
{
  XEvent xev;
  Atom wm_state = XInternAtom(dsp, "_NET_WM_STATE", False);
  Atom fullscreen = XInternAtom(dsp, "_NET_WM_STATE_FULLSCREEN", False);
/*
#ifdef DEBUG
     log_print(__FILE__, __LINE__,"In function go_fullscreen -> at the beginning");
#endif // DEBUG
*/
  memset(&xev, 0, sizeof(xev));
  xev.type = ClientMessage;
  xev.xclient.window = win;
  xev.xclient.message_type = wm_state;
  xev.xclient.format = 32;
  xev.xclient.data.l[0] = 1; // _NET_WM_STATE_ADD
  xev.xclient.data.l[1] = fullscreen;
  xev.xclient.data.l[2] = 0;
  XSendEvent(dsp, DefaultRootWindow(dsp), False, SubstructureNotifyMask, &xev);
/*
#ifdef DEBUG
     log_print(__FILE__, __LINE__,"In function go_fullscreen -> at the end");
#endif // DEBUG
*/
}

void return_fullscreen(Display *dsp, Window win)
{
  XEvent xev;
  Atom wm_state = XInternAtom(dsp, "_NET_WM_STATE", False);
  Atom fullscreen = XInternAtom(dsp, "_NET_WM_STATE_FULLSCREEN", False);
  memset(&xev, 0, sizeof(xev));
  xev.type = ClientMessage;
  xev.xclient.window = win;
  xev.xclient.message_type = wm_state;
  xev.xclient.format = 32;
  xev.xclient.data.l[0] = 0; // _NET_WM_STATE_REMOVE
  xev.xclient.data.l[1] = fullscreen;
  xev.xclient.data.l[2] = 0;
  XSendEvent(dsp, DefaultRootWindow(dsp), False, SubstructureNotifyMask | SubstructureRedirectMask, &xev);
}

/*
 * getint and getshort are help functions to load the bitmap byte by byte on
 * SPARC platform.
 * I've got them from xv bitmap load routine because the original bmp loader didn't work
 * I've tried to change as less code as possible.
 */

static unsigned int getint(fp)
     FILE *fp;
{
  int c, c1, c2, c3;

  // get 4 bytes
  c = getc(fp);
  c1 = getc(fp);
  c2 = getc(fp);
  c3 = getc(fp);

  return ((unsigned int) c) +
    (((unsigned int) c1) << 8) +
    (((unsigned int) c2) << 16) +
    (((unsigned int) c3) << 24);
}

static unsigned int getshort(fp)
     FILE *fp;
{
  int c, c1;

  //get 2 bytes
  c = getc(fp);
  c1 = getc(fp);

  return ((unsigned int) c) + (((unsigned int) c1) << 8);
}

// quick and dirty bitmap loader...for 24 bit bitmaps with 1 plane only.
// See http://www.dcs.ed.ac.uk/~mxr/gfx/2d/BMP.txt for more info.

/**	            This function load image stored in file BMP (quick and dirty bitmap loader !! bug on SOLARIS/SPARC, two functions added below correct this code)
*
* \brief      Fonction LoadBMP: fonction chargeant une image stockée dans un fichier BMP
* \details    En entrée, le nom du fichier stockant l'image, en retour l'image chargée en mémoire.
* \param	  *filename			Le nom du fichier stockant l'image					*
* \param	  *image			l'image chargée en mémoire					*
* \return     int               un entier de type booléen (0 / 1).
*
*/

int LoadBMP(char *filename, Texture *texture) {
    FILE *file;
#ifdef __x86_64__
    unsigned int size;                  // size of the image in bytes
#else
    unsigned long size;                 // size of the image in bytes
#endif

#ifdef __x86_64__
    unsigned int i;                     // standard counter (4 bytes stored value)
#else
    unsigned long i;                    // standard counter (4 bytes stored value)
#endif

    unsigned short int planes;          // number of planes in image (must be 1)
    unsigned short int bpp;             // number of bits per pixel (must be 24)
    char temp;                          // used to convert bgr to rgb color.

#ifdef DEBUG
    if (strcmp(level_debug,"BASE") == 0 || strcmp(level_debug,"FULL") == 0)
        log_print(__FILE__, __LINE__,"Begin function ImageLoad.");
#endif // defined DEBUG

    // make sure the file is there.
    if ((file = fopen(filename, "rb"))==NULL) {
      printf("File Not Found : %s\n",filename);
      return 0;
    }

    // seek through the bmp header, up to the width/height:
    fseek(file, 18, SEEK_CUR);

    // No 100% errorchecking anymore!!!

    // read the width
    texture->ulDimensionX =  getint (file);
    printf("Width of %s: %ul\n", filename, texture->ulDimensionX);

    // read the height
    texture->ulDimensionY = getint (file);
    printf("Height of %s: %ul\n", filename, texture->ulDimensionY);

    // calculate the size (assuming 24 bits or 3 bytes per pixel).
    size = texture->ulDimensionX * texture->ulDimensionY * 3;

    // read the planes
    planes = getshort(file);
    if (planes != 1) {
	printf("Planes from %s is not 1: %u\n", filename, planes);
	return 0;
    }

    // read the bpp
    bpp = getshort(file);
    if (bpp != 24) {
      printf("Bpp from %s is not 24: %u\n", filename, bpp);
      return 0;
    }

    // seek past the rest of the bitmap header.
    fseek(file, 24, SEEK_CUR);

    // read the data.
    texture->pcData = (char *) malloc(size);
    if (texture->pcData == NULL) {
	printf("Error allocating memory for color-corrected image data");
	return 0;
    }

    if ((i = fread(texture->pcData, size, 1, file)) != 1) {
	printf("Error reading image data from %s.\n", filename);
	return 0;
    }

    for (i=0;i<size;i+=3) { // reverse all of the colors. (bgr -> rgb)
	temp = texture->pcData[i];
	texture->pcData[i] = texture->pcData[i+2];
	texture->pcData[i+2] = temp;
    }

#ifdef DEBUG
    if (strcmp(level_debug,"BASE") == 0 || strcmp(level_debug,"FULL") == 0)
        log_print(__FILE__, __LINE__,"End function ImageLoad.");
#endif // defined DEBUG

    // we're done.
    return 1;
}

// Load Bitmaps And Convert To Textures

/**	            This function load textures used for object with instructions OpenGL
*
* \brief      Fonction LoadGLTextures : fonction chargeant la texture à appliquer à un objet avec des instructions OpenGL
* \details    Aucun paramètre en entrée, et rien en retour (GLVoid).
* \return     GLvoid              aucun retour.
*
*/

int LoadGLTextures()
{
  // Load Texture
  Texture *TextureImage;

#ifdef DEBUG
    if (strcmp(level_debug,"BASE") == 0 || strcmp(level_debug,"FULL") == 0)
        log_print(__FILE__, __LINE__,"Begin function LoadGLTextures.");
#endif // defined DEBUG

  // allocate space for texture
  TextureImage = (Texture *) malloc(sizeof(Texture));

  glGenTextures(5, &iTexture[0]);                // Create Five Textures

  for(loop=0;loop<5;loop++)
  {
    switch(loop)
    {
      case 0:
        LoadBMP("../../Data/lesson20/logo.bmp", TextureImage);
        break;

      case 1:
        LoadBMP("../../Data/lesson20/image1.bmp", TextureImage);
        break;

      case 2:
        LoadBMP("../../Data/lesson20/mask1.bmp", TextureImage);
        break;

      case 3:
        LoadBMP("../../Data/lesson20/image2.bmp", TextureImage);
        break;

      case 4:
        LoadBMP("../../Data/lesson20/mask2.bmp", TextureImage);
        break;
    }
    glBindTexture(GL_TEXTURE_2D, iTexture[loop]);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, TextureImage->ulDimensionX, TextureImage->ulDimensionY, 0, GL_RGB, GL_UNSIGNED_BYTE, TextureImage->pcData);
  }

  if (TextureImage)                      // If Texture Exists
  {
    if (TextureImage->pcData)            // If Texture Image Exists
    {
      free(TextureImage->pcData);        // Free The Texture Image Memory
    }
    free(TextureImage);                  // Free The Image Structure
  }

#ifdef DEBUG
    if (strcmp(level_debug,"BASE") == 0 || strcmp(level_debug,"FULL") == 0)
        log_print(__FILE__, __LINE__,"End function LoadGLTextures.");
#endif // defined DEBUG

  return True;                                   // Return The Status
}

/* Function to construct and initialize X-windows Window */

/**	            This function initialize the basics characteristics of the screen under X11 (background, depth, type of depth, reset of the projection matrix, ...)
*
* \brief      Fonction xInitWindow : fonction gerant les caractéristiques de base de la scéne lorsque avec des instructions OpenGL (arrière plan, profondeur, type de profondeur, ...)
* \details    Aucun status en retour, deux paramètres en entrée : ceux de la ligne de commande.
* \details    En entrée la taille de la fenêtre X11 (Width, Height) pour le contexte OpenGL, aucun retour.
* \param	  Width 		    la largeur de la fenêtre                *
* \param	  Height			la hauteur de la fenêtre				*
* \return     int               statut du retour.
*
*/

void xInitWindow(int *argv, char **argc)
{
  XVisualInfo 		*xvVisualInfo;
  Colormap    		cmColorMap;
  XSetWindowAttributes winAttributes;
  GLXContext  		glXContext;

#ifdef DEBUG
    if (strcmp(level_debug,"BASE") == 0 || strcmp(level_debug,"FULL") == 0)
        log_print(__FILE__, __LINE__,"Begin function xInitWindow.");
#endif // defined DEBUG

  /* Open the Display */
  dpDisplay = XOpenDisplay(NULL);
  if(dpDisplay == NULL)
  {
    printf("Could not open display!\n\r");
    exit(0);
  }

  /* Check for GLX extension to X-Windows */
  if(!glXQueryExtension(dpDisplay, NULL, NULL))
  {
    printf("X server has no GLX extension.\n\r");
    exit(0);
  }

  /* Grab a doublebuffering RGBA visual as defined in dblBuf */
  xvVisualInfo = glXChooseVisual(dpDisplay, DefaultScreen(dpDisplay), dblBuf);
  if(xvVisualInfo == NULL)
  {
    printf("No double buffering RGB visual with depth buffer available.\n\r");
    exit(0);
  }

  /* Create a window context */
  glXContext = glXCreateContext(dpDisplay, xvVisualInfo, None, True);
  if(glXContext == NULL)
  {
    printf("Could not create rendering context.\n\r");
    exit(0);
  }

  /* Create the color map for the new window */
  cmColorMap = XCreateColormap(dpDisplay, RootWindow(dpDisplay, xvVisualInfo->screen), xvVisualInfo->visual, AllocNone);
  winAttributes.colormap = cmColorMap;
  winAttributes.border_pixel = 0;
  winAttributes.event_mask = ExposureMask | ButtonPressMask | StructureNotifyMask |
                             KeyPressMask;

  /* Create the actual window object */
  win = XCreateWindow(dpDisplay,
                      RootWindow(dpDisplay, xvVisualInfo->screen),
                      0,
                      0,
                      640,                      // Horizontal Size
                      480,                      // Veritical Size
                      0,
                      xvVisualInfo->depth,
                      InputOutput,
                      xvVisualInfo->visual,
                      CWBorderPixel | CWColormap | CWEventMask,
                      &winAttributes);

  /* Set the standard properties for the window. */
  XSetStandardProperties(dpDisplay,
                         win,
                         "Daniel Davis's Fog Tutorial - Lesson 20 ... NeHe '99",
                         "lesson20",
                         None,
                         (char **) argv,
                         (int) argc,
                         NULL);

  /* Establish new event */
  wmDeleteWindow = XInternAtom(dpDisplay, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(dpDisplay, win, &wmDeleteWindow, 1);

  /* Bind the OpenGL context to the newly created window. */
  glXMakeCurrent(dpDisplay, win, glXContext);

  /* Make the new window the active window. */
  XMapWindow(dpDisplay, win);

#ifdef DEBUG
    if (strcmp(level_debug,"BASE") == 0 || strcmp(level_debug,"FULL") == 0)
        log_print(__FILE__, __LINE__,"End function xInitWindow.");
#endif // defined DEBUG

}

/* A general OpenGL initialization function.  Sets all of the initial parameters. */

/**	            This function initialize the basics characteristics of the scene with instructions OpenGL (background, depth, type of depth, reset of the projection matrix, ...)
*
* \brief      Fonction InitGL : fonction gerant les caractéristiques de base de la scéne lorsque avec des instructions OpenGL (arrière plan, profondeur, type de profondeur, ...)
* \details    Juste un statut en retour, aucun paramètre en entrée (GLVoid).
* \details    En entrée la taille de la fenêtre X11 (Width, Height) pour le contexte OpenGL, en retour un entier représentant le statut OK/NOK de l'appel de cette fonction.
* \param	  Width 		    la largeur de la fenêtre                *
* \param	  Height			la hauteur de la fenêtre				*
* \return     int               statut du retour.
*
*/

int InitGL(int Width, int Height)                // We call this right after our OpenGL window is created.
{
#ifdef DEBUG
    if (strcmp(level_debug,"BASE") == 0 || strcmp(level_debug,"FULL") == 0)
        log_print(__FILE__, __LINE__,"Begin function InitGL.");
#endif // defined DEBUG

  if (!LoadGLTextures())                         // Jump To Texture Loading Routine
  {
    return False;                                // If Texture Didn't Load Return FALSE
  }

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);          // Clear The Background Color To Black
  glClearDepth(1.0);                             // Enables Clearing Of The Depth Buffer
  glEnable(GL_DEPTH_TEST);                       // Enable Depth Testing
  glShadeModel(GL_SMOOTH);                       // Enables Smooth Color Shading
  glEnable(GL_TEXTURE_2D);                       // Enable 2D Texture Mapping

#ifdef DEBUG
    if (strcmp(level_debug,"BASE") == 0 || strcmp(level_debug,"FULL") == 0)
        log_print(__FILE__, __LINE__,"End function InitGL.");
#endif // defined DEBUG

  return True;
}


/* The function called when our window is resized  */

/**	            This function manage the new dimension of the scene when resize of windows with instructions OpenGL
*
* \brief      Fonction ReSizeGLScene : fonction gerant les nouvelles dimensions de la scéne lorsque l'utilisateur agit sur un redimensionnement de la fenêtre
* \details    En retour les deux paramètres des nouvelles valeurs de largeur et de hauteur de la fenêtre redimensionnée.
* \param	  Width			    la  nouvelle largeur de la fenêtre redimensionnée					*
* \param	  Height			la  nouvelle hauteur de la fenêtre redimensionnée					*
* \return     void              aucun retour.
*
*/

void ReSizeGLScene(int Width, int Height)
{
#ifdef DEBUG
    if (strcmp(level_debug,"BASE") == 0 || strcmp(level_debug,"FULL") == 0)
        log_print(__FILE__, __LINE__,"Begin function ResizeGLScene.");
#endif // defined DEBUG

  if (Height==0)                                // Prevent A Divide By Zero If The Window Is Too Small
    Height=1;

  glViewport(0, 0, Width, Height);              // Reset The Current Viewport And Perspective Transformation

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  gluPerspective(45.0f,(GLfloat)Width/(GLfloat)Height,0.1f,100.0f);
  glMatrixMode(GL_MODELVIEW);

#ifdef DEBUG
    if (strcmp(level_debug,"BASE") == 0 || strcmp(level_debug,"FULL") == 0)
        log_print(__FILE__, __LINE__,"End function ResizeGLScene.");
#endif // defined DEBUG
}

/* The function to draw the screendrawing function. */

/**	            This function generate the scene with instructions OpenGL
*
* \brief      Fonction DrawGLScene : fonction generant l'affichage attendu avec des instructions OpenGL
* \details    Aucun paramètre dans ce cas de figure car tout est géré directement à l'intérieur de cette fonction d'affichage.
* \return     void             aucun retour.
*
*/

void DrawGLScene()
{
#ifdef DEBUG
    if (strcmp(level_debug,"BASE") == 0 || strcmp(level_debug,"FULL") == 0)
        log_print(__FILE__, __LINE__,"Begin function DrawGLScene.");
#endif // defined DEBUG

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear The Screen And The Depth Buffer
  glLoadIdentity();                              // Reset The Modelview Matrix
  glTranslatef(0.0f,0.0f,-2.0f);                 // Move Into The Screen 5 Units

  glBindTexture(GL_TEXTURE_2D, iTexture[0]);     // Select Our Logo Texture
  glBegin(GL_QUADS);                             // Start Drawing A Textured Quad
    glTexCoord2f(0.0f, -roll+0.0f); glVertex3f(-1.1f, -1.1f,  0.0f); // Bottom Left
    glTexCoord2f(3.0f, -roll+0.0f); glVertex3f( 1.1f, -1.1f,  0.0f); // Bottom Right
    glTexCoord2f(3.0f, -roll+3.0f); glVertex3f( 1.1f,  1.1f,  0.0f); // Top Right
    glTexCoord2f(0.0f, -roll+3.0f); glVertex3f(-1.1f,  1.1f,  0.0f); // Top Left
  glEnd();                                       // Done Drawing The Quad

  glEnable(GL_BLEND);                            // Enable Blending
  glDisable(GL_DEPTH_TEST);                      // Disable Depth Testing

  if (masking)                                   // Is Masking Enabled?
  {
    glBlendFunc(GL_DST_COLOR,GL_ZERO);           // Blend Screen Color With Zero (Black)
  }

  if (scene)                                     // Are We Drawing The Second Scene?
  {
    glTranslatef(0.0f,0.0f,-1.0f);               // Translate Into The Screen One Unit
    glRotatef(roll*360,0.0f,0.0f,1.0f);          // Rotate On The Z Axis 360 Degrees.
    if (masking)                                 // Is Masking On?
    {
      glBindTexture(GL_TEXTURE_2D, iTexture[3]); // Select The Second Mask Texture
      glBegin(GL_QUADS);                         // Start Drawing A Textured Quad
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.1f, -1.1f,  0.0f); // Bottom Left
        glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.1f, -1.1f,  0.0f); // Bottom Right
        glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.1f,  1.1f,  0.0f); // Top Right
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.1f,  1.1f,  0.0f); // Top Left
      glEnd();                                   // Done Drawing The Quad
    }

    glBlendFunc(GL_ONE, GL_ONE);                 // Copy Image 2 Color To The Screen
    glBindTexture(GL_TEXTURE_2D, iTexture[4]);   // Select The Second Image Texture
    glBegin(GL_QUADS);                           // Start Drawing A Textured Quad
      glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.1f, -1.1f,  0.0f); // Bottom Left
      glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.1f, -1.1f,  0.0f); // Bottom Right
      glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.1f,  1.1f,  0.0f); // Top Right
      glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.1f,  1.1f,  0.0f); // Top Left
      glEnd();                                   // Done Drawing The Quad
  }
  else                                           // Otherwise
  {
    if (masking)                                 // Is Masking On?
    {
      glBindTexture(GL_TEXTURE_2D, iTexture[1]); // Select The First Mask Texture
      glBegin(GL_QUADS);                         // Start Drawing A Textured Quad
        glTexCoord2f(roll+0.0f, 0.0f); glVertex3f(-1.1f, -1.1f,  0.0f); // Bottom Left
        glTexCoord2f(roll+4.0f, 0.0f); glVertex3f( 1.1f, -1.1f,  0.0f); // Bottom Right
        glTexCoord2f(roll+4.0f, 4.0f); glVertex3f( 1.1f,  1.1f,  0.0f); // Top Right
        glTexCoord2f(roll+0.0f, 4.0f); glVertex3f(-1.1f,  1.1f,  0.0f); // Top Left
      glEnd();                                   // Done Drawing The Quad
    }

    glBlendFunc(GL_ONE, GL_ONE);                 // Copy Image 1 Color To The Screen
    glBindTexture(GL_TEXTURE_2D, iTexture[2]);   // Select The First Image Texture
    glBegin(GL_QUADS);                           // Start Drawing A Textured Quad
      glTexCoord2f(roll+0.0f, 0.0f); glVertex3f(-1.1f, -1.1f,  0.0f); // Bottom Left
      glTexCoord2f(roll+4.0f, 0.0f); glVertex3f( 1.1f, -1.1f,  0.0f); // Bottom Right
      glTexCoord2f(roll+4.0f, 4.0f); glVertex3f( 1.1f,  1.1f,  0.0f); // Top Right
      glTexCoord2f(roll+0.0f, 4.0f); glVertex3f(-1.1f,  1.1f,  0.0f); // Top Left
    glEnd();                                     // Done Drawing The Quad
  }

  glEnable(GL_DEPTH_TEST);                       // Enable Depth Testing
  glDisable(GL_BLEND);                           // Disable Blending

  roll+=0.002f;                                  // Increase Our Texture Roll Variable
  if (roll>1.0f)                                 // Is Roll Greater Than One
  {
    roll-=1.0f;                                  // Subtract 1 From Roll
  }

   // since this is double buffered, swap the buffers to display what just got drawn.
  glXSwapBuffers(dpDisplay, win);

#ifdef DEBUG
    if (strcmp(level_debug,"BASE") == 0 || strcmp(level_debug,"FULL") == 0)
        log_print(__FILE__, __LINE__,"End function DrawGLScene.");
#endif // defined DEBUG
}


/* The function called whenever a key is pressed. */

/**	            This function inform caller with key pressed.
*
* \brief      Fonction keyPressed : fonction permettant de connaître quelle touche a été tapée au clavier
* \details    Le premier paarmètre en retour de cette fonction correspond à la touche précédemment appuyée.
* \param	  keysym			    la touche appuyée 					*
* \return     void              aucun retour.
*
*/

void keyPressed(KeySym key)
{
  /* avoid thrashing this procedure */
//  usleep(100);

  /* If escape is pressed, kill everything. */
  switch(key)
  {
 //   Exit program
    case XK_Q:
    case XK_q:
    case XK_Escape:
      XCloseDisplay(dpDisplay);

      /* exit the program...normal termination. */
      exit(0);
      break;

 //   Fullscreen
    case XK_F:
    case XK_f:
        if (nFullScreen) {
            return_fullscreen(dpDisplay, win);
            ReSizeGLScene(640, 480);
            XFlush(dpDisplay);
            nFullScreen = 0;
        } else {
            go_fullscreen(dpDisplay, win);
/*
#ifdef DEBUG
     log_print(__FILE__, __LINE__,"In function keypressed-> before call XGetWindowAttributes.");
#endif // DEBUG
*/
            XGetWindowAttributes(dpDisplay, win, &window_attr);
/*
#ifdef DEBUG
     log_print(__FILE__, __LINE__,"In function keypressed-> after call XGetWindowAttributes.");
#endif // DEBUG
*/
            printf("Width of X screen : %i\n",window_attr.width);
            printf("Heigth of X screen : %i\n",window_attr.height);
            ReSizeGLScene(window_attr.width, window_attr.height);
            XFlush(dpDisplay);
            nFullScreen = 1;
        }
      break;
    case XK_space:
      scene=!scene;
      break;

    case XK_M:
    case XK_m:
      masking=!masking;
      break;
  }
}

/**	            This function is the X11 waiting loop of events
*
* \brief      Fonction xMainLoop : orrespondant à la boucle d'attente d'événements de X11
* \details    Aucun paramètre en entrée et aucun retour.
* \return     void             aucun retour.
*
*/

void xMainLoop()
{
  XEvent event;
  KeySym ks;
  Bool needRedraw = False;

  while(1)
  {
    if(XPending(dpDisplay))  // While more events are pending, continue processing.
    {
      /* Get the current event from the system queue. */
      XNextEvent(dpDisplay, &event);

      /* Process the incoming event. */
      switch(event.type)
      {
        case Expose:
          needRedraw = True;
          break;

        /* If window moves, redraw it. */
        case MotionNotify:
          needRedraw = True;
          break;

        /* If a key was pressed, get keystroke and called the processing function. */
        case KeyPress:
          ks = XLookupKeysym((XKeyEvent *) &event, 0);
          keyPressed(ks);
          break;

        /* If the screen was resized, call the appropriate function. */
        case ConfigureNotify:
          ReSizeGLScene(event.xconfigure.width, event.xconfigure.height);
          break;

        case ButtonPress:
          break;

        /* Process any custom messages. */
        case ClientMessage:
          if(event.xclient.data.l[0] == wmDeleteWindow)
          {
 XCloseDisplay(dpDisplay);
            exit(0);
          }
          break;
      }
    }

    /* If redraw flag is set, redraw the window. */
//    if(needRedraw)
    {
      DrawGLScene();
    }
  }
}


int main(int argc, char **argv)
{

   if (getenv("LEVEL")) {                 // LEVEL is set
       level_debug=getenv("LEVEL");           // Added by Thierry DECHAIZE : récupérer la valeur de la variable d'environnement LEVEL si elle existe
       }
    else {                                // if LEVEL not set, same with Debug mode, nothing log is generated (file log.txt in current directory)
       level_debug = '\0';
    }

#ifdef DEBUG
    printf("Niveau de trace : %s.\n",level_debug);

    if (strcmp(level_debug,"BASE") == 0 || strcmp(level_debug,"FULL") == 0)
        log_print(__FILE__, __LINE__,"Enter within main, before call of xInitWindow.");
#endif // defined DEBUG

  /* Initialize our window. */

/**	            This Code initialize the context of windows on X11.
*
* \brief      Appel de la fonction xInitWindow: fonction X11 d'initialisation
* \details    En entrée de cette fonction, les paramètres de la ligne de commande.
* \param	  argc			    le nombre de paramètres mis à disposition sur la ligne de commande
* \param	  argv              le tableau des différents paramètres mis à disposition sur la ligne de commande
*
*/

  xInitWindow(&argc, argv);

#ifdef DEBUG
    printf("Niveau de trace : %s.\n",level_debug);

    if (strcmp(level_debug,"BASE") == 0 || strcmp(level_debug,"FULL") == 0)
        log_print(__FILE__, __LINE__,"Next step in main, before call of InitGL.");
#endif // defined DEBUG

  /* Initialize OpenGL routines */

/**	            This Code initialize the OpenGL context of windows on X11.
*
* \brief      Appel de la fonction InitGL : fonction X11 d'initialisation
* \details    En entrée de cette fonction, la largeur et la hauteur de la fenêtre
* \param	  Width 		    la largeur de la fenêtre
* \param	  Height            la hauteur de la fenêtre
*
*/


  if(!InitGL(640, 480))
  {
    printf("Error initializing OpenGL. \n\r");
    exit(0);
  }

#ifdef DEBUG
    if (strcmp(level_debug,"BASE") == 0 || strcmp(level_debug,"FULL") == 0)
        log_print(__FILE__, __LINE__,"Next step within main, before call of xMainLoop.");
#endif // defined DEBUG

  /* Start Event Processing Engine */

/**	            This Code run on main X11 loop waiting events.
*
* \brief      Appel de la fonction xMainLoop : boucle principale d'attente des événements sous X11
* \details    Aucun paramètre.
*
*/

  xMainLoop();

  return(1);
}

