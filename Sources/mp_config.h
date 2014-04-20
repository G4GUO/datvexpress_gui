//
// This module reads in a configuration item from the config file
//

void config_register_filename( const char *filename );
void config_get_string( const char *item, char *out );
int config_get_value( const char *item );
