#ifndef __ex_gles_h_
#define __ex_gles_h_

/************************************************
 * Name:    ex_gles.h
 *
 *************************************************/

#include "ex_global.h"

void PrintOGLES1APIInfo(void);
int TryLoadOGLES1Library(void);
void UnloadOGLES1Library(void);
const char *GetOGLES1VendorString(void);

#endif /* __ex_gles_h_ */
