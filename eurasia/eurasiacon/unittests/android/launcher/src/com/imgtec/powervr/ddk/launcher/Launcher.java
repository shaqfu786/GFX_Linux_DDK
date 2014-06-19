/*!****************************************************************************
@File           Launcher.java

@Title          Test launcher for Android (Java component)

@Author         Imagination Technologies

@Date           2010/02/22

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

@Description    Test launcher for Android (Java component)

Modifications :-
$Log: Launcher.java $
Revision 1.1  2010/02/22 12:21:51  alistair.strachan
Initial revision
******************************************************************************/

package com.imgtec.powervr.ddk.launcher;

import java.util.Collections;
import java.util.Comparator;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import java.text.Collator;

import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageInfo;

import android.widget.SimpleAdapter;
import android.widget.ListView;

import android.app.ListActivity;
import android.content.Intent;
import android.view.View;
import android.os.Bundle;
import android.util.Log;

public class Launcher extends ListActivity
{
	private List<Map<String,Object>> getDDKActivityList() {
		List<Map<String,Object>> list = new ArrayList<Map<String,Object>>();
		PackageManager pm = getPackageManager();

		for(PackageInfo p : pm.getInstalledPackages(PackageManager.GET_ACTIVITIES))
		{
			if(!p.packageName.startsWith("com.imgtec.powervr.ddk"))
				continue;

			if(p.packageName.equals("com.imgtec.powervr.ddk.launcher"))
				continue;

			try {
				ApplicationInfo ai = pm.getApplicationInfo(p.packageName, 0);
				Map<String,Object> m = new HashMap<String,Object>();
				Intent i = new Intent();
				i.setClassName(ai.packageName, p.activities[0].name);
				m.put("title", pm.getApplicationLabel(ai).toString());
				m.put("intent", i);
				list.add(m);
			} catch (NameNotFoundException e) {
				e.printStackTrace();
			}
		}

		Collections.sort(list, new Comparator<Map>(){
			private final Collator collator = Collator.getInstance();
			public int compare(Map map1, Map map2) {
				return collator.compare(map1.get("title"), map2.get("title"));
			}
		});

		return list;
	}

	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setListAdapter(
			new SimpleAdapter(
				this,
				getDDKActivityList(),
				android.R.layout.simple_list_item_1,
				new String[] { "title" },
				new int[] { android.R.id.text1 }
			)
		);
	}

    @Override
    protected void onListItemClick(ListView l, View v, int position, long id) {
        Map map = (Map)l.getItemAtPosition(position);
        Intent intent = (Intent)map.get("intent");
        startActivity(intent);
    }
}
