#include "aa.h"
#include "XPPConfig.h"

#define TRANSITION_AMT 0.05

XPLMDataRef ref_alt_agl         = NULL;
XPLMDataRef ref_deg_vert        = NULL;

XPLMWindowID	debug_window = NULL;
int				clicked = 0;

char debug_string[255];

float config_transition_alt = 1000;

float view_angle_above;
float view_angle_below;

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

    float alt_agl   = XPLMGetDataf( ref_alt_agl );
    float cur_angle = XPLMGetDataf( ref_deg_vert );
    float target_angle;
    float delta;

    /* check to see how close we are, and adjust accordingly */
    if( alt_agl > config_transition_alt ) {
        target_angle = view_angle_above; 
    }
    else {
        target_angle = view_angle_below;
    }

    /* now decide what direction to move the view */
    delta = target_angle - cur_angle;

    if( delta > 0 ) {
   
        /* > 0 means target angle is larger, need to adjust up */
        target_angle = cur_angle + ( delta > TRANSITION_AMT 
                                 ? TRANSITION_AMT
                                 : delta );

    }
    else if( delta < 0 ) {
        /* < 0 means cur_angle is larger, need to adjust down */
        delta *= -1;
        target_angle = cur_angle - ( delta > TRANSITION_AMT
                                 ? TRANSITION_AMT
                                 : delta );
    }

    XPLMSetDataf( ref_deg_vert, target_angle );

    return CALLBACK_INTERVAL;
}

PLUGIN_API int XPluginStart(char *outName, char *outSig, char *outDesc) {

    float config_offset_angle = -7;
    float config_base_angle = -.1;

    int num_configs = 3;

    XPPCItem *configs = XPPCInit( num_configs );

    /* First set up our plugin info. */
    strcpy(outName, "AutoAngle 0.1");
    strcpy(outSig, "AutoAngle by Colin Lieberman");
    strcpy(outDesc, "adjust view down when crusing, and flat when landing");

    configs[0].key = "# TRANSITION ALTITUDE";
    configs[0].ref = &config_transition_alt;

    configs[1].key = "# OFFSET";
    configs[1].ref = &config_offset_angle;
    
    configs[2].key = "# BASE";
    configs[2].ref = &config_base_angle;

    if( !XPPCParseConfigFile( "Resources/plugins/AutoAngle/config.txt", configs, num_configs ) ) {
        XPLMDebugString( XPPCLastError() );
    }

    view_angle_above = config_base_angle + config_offset_angle;
    view_angle_below = config_base_angle;

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
