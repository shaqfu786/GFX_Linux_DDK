/*!****************************************************************************
@File           testwrap.h

@Title          Test wrapper for Android (JNI component)

@Author         Imagination Technologies

@Date           2010/01/25

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

@Description    Test wrapper for Android (JNI component)

Modifications :-
$Log: testwrap.h $
******************************************************************************/

#ifndef __TESTWRAP_H__
#define __TESTWRAP_H__

#include <nativehelper/jni.h>

JNIEXPORT jint JNICALL Java_com_imgtec_powervr_ddk_TestProgram_init(
	JNIEnv *env, jobject obj, jstring wrapLib);

JNIEXPORT jint JNICALL Java_com_imgtec_powervr_ddk_TestProgram_eglMain(
	JNIEnv *env, jobject obj, jobject eglNativeWindow);

JNIEXPORT void JNICALL Java_com_imgtec_powervr_ddk_TestProgram_resizeWindow(
	JNIEnv *env, jobject obj, jint width, jint height);

JNIEXPORT void JNICALL Java_com_imgtec_powervr_ddk_TestProgram_requestQuit(
	JNIEnv *env, jobject obj);

#endif /* __TESTWRAP_H__ */
