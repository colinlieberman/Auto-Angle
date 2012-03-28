#include "aa.h"

#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMUtilities.h"
#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"
#include "XPLMPlugin.h"
#include "XPLMMenus.h"
#include "XPWidgets.h"
#include "XPStandardWidgets.h"

#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <string.h>
#include <math.h>

XPLMDataRef ref_alt_agl         = NULL;
XPLMDataRef ref_alt_msl         = NULL;
XPLMDataRef ref_deg_vert        = NULL;

XPLMWindowID	debug_window = NULL;
int				clicked = 0;

char debug_string[255];

/* window callbacks for debugging */
void MyDrawWindowCallback(
                                   XPLMWindowID         inWindowID,
                                   void *               inRefcon);

void MyHandleKeyCallback(
                                   XPLMWindowID         inWindowID,
                                   char                 inKey,
                                   XPLMKeyFlags         inFlags,
                                   char                 inVirtualKey,
                                   void *               inRefcon,
                                   int                  losingFocus);

int MyHandleMouseClickCallback(
                                   XPLMWindowID         inWindowID,
                                   int                  x,
                                   int                  y,
                                   XPLMMouseStatus      inMouse,
                                   void *               inRefcon);


float AutoAngleCallback(
        float   inElapsedSinceLastCall,
        float   inElapsedTimeSinceLastFlightLoop,
        int     inCounter,
        void    *inRefcon) {

    /* get altitude - it's used in a number of places */
    float alt_agl = XPLMGetDataf( ref_alt_agl );
    float alt_msl = XPLMGetDataf( ref_alt_msl );
    float deg_vert;

    /* TODO: config */
    if( alt_agl > 800 ) {
        deg_vert = -5;
    }
    else {
        deg_vert = 0;
    }

    XPLMSetDataf( ref_deg_vert, deg_vert );

    return CALLBACK_INTERVAL;
}

PLUGIN_API int XPluginStart(char *outName, char *outSig, char *outDesc) {

    /* First set up our plugin info. */
    strcpy(outName, "AutoAngle 0.1");
    strcpy(outSig, "AutoAngle by Colin Lieberman");
    strcpy(outDesc, "adjust view down when crusing, and flat when landing");

    ref_alt_msl         = XPLMFindDataRef("sim/flightmodel/position/elevation");
    ref_alt_agl         = XPLMFindDataRef("sim/flightmodel/position/y_agl");
    ref_deg_vert        = XPLMFindDataRef("sim/graphics/view/field_of_view_vertical_deg");

	if( DEBUG ) {
        debug_window = XPLMCreateWindow(
                        50, 600, 400, 200,			/* Area of the window. */
                        1,							/* Start visible. */
                        MyDrawWindowCallback,		/* Callbacks */
                        MyHandleKeyCallback,
                        MyHandleMouseClickCallback,
                        NULL);						/* Refcon - not used. */
    }
        
    // * Register our callback for every loop. Positive intervals
    // * are in seconds, negative are the negative of sim frames.  Zero
    // * registers but does not schedule a callback for time.
    XPLMRegisterFlightLoopCallback(
            AutoAngleCallback,	// * Callback *
            CALLBACK_INTERVAL,          // * Interval -1 every loop*
            NULL);				        // * refcon not used. *

    return 1;
}

PLUGIN_API void	XPluginStop(void) {
    XPLMUnregisterFlightLoopCallback(AutoAngleCallback, NULL);

    if( DEBUG ) {
        XPLMDestroyWindow(debug_window);
    }
}


PLUGIN_API void XPluginDisable(void) {
}

PLUGIN_API int XPluginEnable(void) {
    return 1;
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, long inMessage, void *inParam){
}

/*
 * MyDrawingWindowCallback
 *
 * This callback does the work of drawing our window once per sim cycle each time
 * it is needed.  It dynamically changes the text depending on the saved mouse
 * status.  Note that we don't have to tell X-Plane to redraw us when our text
 * changes; we are redrawn by the sim continuously.
 *
 */
void MyDrawWindowCallback(
                                   XPLMWindowID         inWindowID,
                                   void *               inRefcon)
{
	int		left, top, right, bottom;
	float	color[] = { 1.0, 1.0, 1.0 }; 	/* RGB White */
    float alt_agl, deg_vert;
    char  buffer[50];

    alt_agl     = XPLMGetDataf( ref_alt_agl );
    deg_vert    = XPLMGetDataf( ref_deg_vert );

	/* First we get the location of the window passed in to us. */
	XPLMGetWindowGeometry(inWindowID, &left, &top, &right, &bottom);

	/* We now use an XPLMGraphics routine to draw a translucent dark
	 * rectangle that is our window's shape. */
	XPLMDrawTranslucentDarkBox(left, top, right, bottom);

    /* Finally we draw the text into the window, also using XPLMGraphics
	 * routines.  The NULL indicates no word wrapping. */
	sprintf(buffer, "deg_vert: %f", deg_vert );
    XPLMDrawString(color, left + 5, top - 20, buffer, NULL, xplmFont_Basic);
	
    sprintf(buffer, "alt_agl: %d", (int)floor(alt_agl));
    XPLMDrawString(color, left + 5, top - 40, buffer, NULL, xplmFont_Basic);

    XPLMDrawString(color, left + 5, top - 80, debug_string, NULL, xplmFont_Basic);

}

/*
 * MyHandleKeyCallback
 *
 * Our key handling callback does nothing in this plugin.  This is ok;
 * we simply don't use keyboard input.
 *
 */
void MyHandleKeyCallback(
                                   XPLMWindowID         inWindowID,
                                   char                 inKey,
                                   XPLMKeyFlags         inFlags,
                                   char                 inVirtualKey,
                                   void *               inRefcon,
                                   int                  losingFocus)
{
}

/*
 * MyHandleMouseClickCallback
 *
 * Our mouse click callback toggles the status of our mouse variable
 * as the mouse is clicked.  We then update our text on the next sim
 * cycle.
 *
 */
int MyHandleMouseClickCallback(
                                   XPLMWindowID         inWindowID,
                                   int                  x,
                                   int                  y,
                                   XPLMMouseStatus      inMouse,
                                   void *               inRefcon)
{
	/* If we get a down or up, toggle our status click.  We will
	 * never get a down without an up if we accept the down. */
	if ((inMouse == xplm_MouseDown) || (inMouse == xplm_MouseUp))
		clicked = 1 - clicked;

	/* Returning 1 tells X-Plane that we 'accepted' the click; otherwise
	 * it would be passed to the next window behind us.  If we accept
	 * the click we get mouse moved and mouse up callbacks, if we don't
	 * we do not get any more callbacks.  It is worth noting that we
	 * will receive mouse moved and mouse up even if the mouse is dragged
	 * out of our window's box as long as the click started in our window's
	 * box. */
	return 1;
}
