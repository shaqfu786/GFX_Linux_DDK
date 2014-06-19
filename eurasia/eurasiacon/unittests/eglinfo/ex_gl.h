#ifndef __ex_gl_h_
#define __ex_gl_h_

/************************************************
 * Name:    ex_gl.h
 *
 *************************************************/

#include "ex_global.h"

void PrintOGLAPIInfo(void);
int TryLoadOGLLibrary(void);
void UnloadOGLLibrary(void);
const char *GetOGLVendorString(void);

#endif /* __ex_gl_h_ */
