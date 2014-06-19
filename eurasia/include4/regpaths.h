/*****************************************************************************
 Name          	: RegPaths.h

 Title			: PowerVR registry path defininitions

 Author			: Richard Hudson

 Created        : 1/3/00
 
 Copyright      : 2000 by Imagination technologies. All rights reserved.
                  No part of this software, either material or conceptual 
                  may be copied or distributed, transmitted, transcribed,
                  stored in a retrieval system or translated into any 
                  human or computer language in any form by any means,
                  electronic, mechanical, manual or other-wise, or 
                  disclosed to third parties without the express written
                  permission of Imagination Technologies Limited, Unit 8, HomePark
                  Industrial Estate, King's Langley, Hertfordshire,
                  WD4 8LZ, U.K.
		  
 Description    :  Registry Path Definitions
                  
 Version		: $Revision: 1.1 $

 Modifications	: 

 $Log: regpaths.h $

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 
 
  --- Revision Logs Removed ---  
 *****************************************************************************/
#ifndef __REGPATHS_H__
#define __REGPATHS_H__

/************************************************************** Root defines */

#define POWERVR_REG_ROOT 	   			"Drivers\\Display\\PowerVR"
#define POWERVR_CHIP_KEY				"\\SGX1\\"

#define POWERVR_EURASIA_KEY				"PowerVREurasia\\"

#define POWERVR_SERVICES_KEY			"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\PowerVR\\"

#define PVRSRV_REGISTRY_ROOT			POWERVR_EURASIA_KEY "HWSettings\\PVRSRVKM"


#define MAX_REG_STRING_SIZE 128


#endif /* __REGPATHS_H__ */
/*****************************************************************************
 End of file (REGPATHS.H)
*****************************************************************************/

