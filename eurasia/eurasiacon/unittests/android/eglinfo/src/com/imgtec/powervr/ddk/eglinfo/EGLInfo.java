/*!****************************************************************************
@File           EGLInfo.java

@Title          Wrapper class for eglinfo

@Author         Imagination Technologies

@Date           2010/01/29

@Copyright      Copyright 2010 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either material
                or conceptual may be copied or distributed, transmitted,
                transcribed, stored in a retrieval system or translated into
                any human or computer language in any form by any means,
                electronic, mechanical, manual or otherwise, or disclosed
                to third parties without the express written permission of
                Imagination Technologies Limited, Home Park Estate,
                Kings Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform       android

@Description    Wrapper class for eglinfo

Modifications :-
$Log: EGLInfo.java $
Revision 1.1  2010/02/01 11:07:18  alistair.strachan
Initial revision
******************************************************************************/

package com.imgtec.powervr.ddk.eglinfo;

import com.imgtec.powervr.ddk.TestProgram;

public class EGLInfo extends TestProgram {
	public String getTestBinary() {
		return "libeglinfo.so";
	}
}
