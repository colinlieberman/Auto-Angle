#include "aa.h"

#include <fstream>

/* lines in the file + 1 */
#define EXPECTED_PREF_LINES 20

#define CONFIG_FILE_PATH "Resources/plugins/AutoAngle/config.txt"
#define ERROR_PREFIX "AutoAngle:"

float config_transition_alt;
float config_offset_angle;
float config_base_angle;

extern char debug_string[255];

void initConfig() {
    string line, lines[ EXPECTED_PREF_LINES + 2 ];
    ifstream pref_file;
    char *end;
    int cur_line = 1;
    float tmp;

    /* set defaults */
    config_transition_alt = 800;
    config_offset_angle   = -5;
    config_base_angle     = 0;

    pref_file.open( CONFIG_FILE_PATH );
    if( ! pref_file ) {
        fprintf( stderr, "%s unable to open file %s; using defaults", ERROR_PREFIX, CONFIG_FILE_PATH );
        return;
    }

    while( !pref_file.eof() )
    {
        if( cur_line > EXPECTED_PREF_LINES ) {
            /* this means we're still reading */
            fprintf( stderr, "%s preferences file %s in unexpeted format, using defaults", ERROR_PREFIX, CONFIG_FILE_PATH );
            return;
        }
        
        getline( pref_file, line );
        lines[ cur_line++ ] = line;
    }
    pref_file.close();

    /* check to make sure we have the expected number of lines -
     * cur line should be one greater than expected length because it
     * incremented at the bottom of the loop */
    if( cur_line != EXPECTED_PREF_LINES + 1 ) {
        fprintf( stderr, "%s preferences file %s in unexpeted format, using defaults", ERROR_PREFIX, CONFIG_FILE_PATH );
        return;
    }
    
    /* line 7: transition alt */
    line = lines[ 7 ];
    tmp  = strtof( line.c_str(), &end );
    if( tmp > 0 && tmp < 100000 ) {
        config_transition_alt = tmp;
    }
    
    /* line 11: offset angle */
    line = lines[ 11 ];
    tmp  = strtof( line.c_str(), &end );
    if( tmp >= -90 && tmp <= 90 ) {
        config_offset_angle = tmp;
    }
    
    /* line 15: base angle */
    line = lines[ 15 ];
    tmp  = strtof( line.c_str(), &end );
    if( tmp >= -90 && tmp <= 90 ) {
        config_base_angle = tmp;
    }

    return;
}
