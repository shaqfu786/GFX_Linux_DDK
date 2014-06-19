/*!****************************************************************************
@File           TestShell.java

@Title          Test Shell for Android (Java component)

@Author         Imagination Technologies

@Date           2010/01/22

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

@Description    Test Shell for Android (Java component)

Modifications :-
$Log: TestProgram.java $
Revision 1.4  2010/03/24 16:19:32  ben.jones
Call Activity.finish when eglMain returns. Allows eglinfo to return to the
menu when it finishes
Revision 1.3  2010/02/19 15:14:04  alistair.strachan
Pass resize event down to test wrapper on surfaceChanged().
Revision 1.2  2010/02/18 17:55:58  alistair.strachan
Implement resizeWindow() and requestQuit().
Revision 1.1  2010/01/28 17:58:46  alistair.strachan
Initial revision
Revision 1.1  2010/01/25 17:35:55  alistair.strachan
Initial revision
******************************************************************************/

package com.imgtec.powervr.ddk;

import java.lang.Runnable;
import java.lang.Thread;

import java.util.Random;

import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.Surface;
import android.app.Activity;
import android.os.Bundle;
import android.util.Log;

public abstract class TestProgram extends Activity
								  implements SurfaceHolder.Callback
{
	/** Load wrapped library and find key symbols */
	private static native int init(String wrapLib);

	/** Wrapper for app's EglMain(); unmarshals eglNativeWindow */
	private static native int eglMain(Object surface);

	/** Wrapper for app's ResizeWindow() */
	private static native void resizeWindow(int width, int height);

	/** Wrapper for app's RequestQuit() */
	private static native void requestQuit();

	private SurfaceView mSurfaceView;
	private Thread mAppThread;
	private Surface mSurface;

	private class AppRunnable implements Runnable {
		public void run() {
			eglMain(mSurface);
			finish();
		}
	}

	private void initialize()
	{
		String mTestShellLib = "/data/data/" +
							   getClass().getPackage().getName() +
							   "/lib/libtestwrap.so";
		System.load(mTestShellLib);

		String mWrapLib = "/data/data/" +
						  getClass().getPackage().getName() +
						  "/lib/" + getTestBinary();
		if(init(mWrapLib) < 0)
			throw new RuntimeException("Failed to initialize " + mWrapLib);
	}

	public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
		resizeWindow(w, h);
	}

	public void surfaceCreated(SurfaceHolder holder) {
		mAppThread = new Thread(new AppRunnable());
		mSurface = holder.getSurface();
		mAppThread.start();
	}

	public void surfaceDestroyed(SurfaceHolder holder) {
		requestQuit();
		try {
			mAppThread.join();
		} catch (InterruptedException e) {
			throw new RuntimeException(e);
		}
	}

	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		mSurfaceView = new SurfaceView(getApplication());
		SurfaceHolder holder = mSurfaceView.getHolder();
		holder.addCallback(this);
		setContentView(mSurfaceView);
		initialize();
	}

	protected abstract String getTestBinary();
}
