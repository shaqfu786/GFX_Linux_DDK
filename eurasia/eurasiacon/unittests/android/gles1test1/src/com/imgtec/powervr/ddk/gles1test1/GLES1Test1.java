/*!****************************************************************************
@File           GLES1Test1.java

@Title          Wrapper class for gles1test1

@Author         Imagination Technologies

@Date           2010/01/26

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

@Description    Wrapper class for gles1test1

Modifications :-
$Log: GLES1Test1.java $
Revision 1.1  2010/01/28 17:58:39  alistair.strachan
Initial revision
Revision 1.1  2010/01/25 17:35:55  alistair.strachan
Initial revision
******************************************************************************/

package com.imgtec.powervr.ddk.gles1test1;

import com.imgtec.powervr.ddk.TestProgram;

public class GLES1Test1 extends TestProgram {
	public String getTestBinary() {
		return "libgles1test1.so";
	}
}
