#ifndef __ex_gles2_h_
#define __ex_gles2_h_


/************************************************
 * Name:    ex_gles2.h
 *
 *************************************************/

#include "ex_global.h"

void PrintOGLES2APIInfo(void);
int TryLoadOGLES2Library(void);
void UnloadOGLES2Library(void);
const char *GetOGLES2VendorString(void);

#endif /* __ex_gles2_h_ */
