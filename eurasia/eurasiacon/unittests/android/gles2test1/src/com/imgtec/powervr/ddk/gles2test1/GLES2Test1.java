/*!****************************************************************************
@File           GLES2Test1.java

@Title          Wrapper class for gles2test1

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

@Description    Wrapper class for gles2test1

Modifications :-
$Log: GLES2Test1.java $
Revision 1.1  2010/02/01 11:08:10  alistair.strachan
Initial revision
******************************************************************************/

package com.imgtec.powervr.ddk.gles2test1;

import com.imgtec.powervr.ddk.TestProgram;

public class GLES2Test1 extends TestProgram {
	public String getTestBinary() {
		return "libgles2test1.so";
	}
}
