#ifndef __ex_vg_h_
#define __ex_vg_h_

/************************************************
 * Name:    ex_vg.h
 *
 *************************************************/

#include "ex_global.h"

void PrintOVGAPIInfo(void);
int TryLoadOVGLibrary(void);
void UnloadOVGLibrary(void);
const char *GetOVGVendorString(void);

#endif /* __ex_vg_h_ */
