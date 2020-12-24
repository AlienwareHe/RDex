package com.alien.rdex;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.util.Log;

import com.camel.api.rposed.RposedHelpers;

public class MainActivity extends AppCompatActivity {

    public static final String TAG = "dump";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        final String saveDir = this.getApplicationContext().getFilesDir() + "/dump";

        new Thread(new Runnable() {
            @Override
            public void run() {
                System.loadLibrary("native-lib");

                try {
                    Thread.sleep(10000);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }

                ClassLoader classLoader = MainActivity.class.getClassLoader();
                Object pathList = RposedHelpers.getObjectField(classLoader, "pathList");
                Object[] dexElements = (Object[]) RposedHelpers.getObjectField(pathList, "dexElements");
                for (Object dexElement : dexElements) {
                    Object dexFile = RposedHelpers.getObjectField(dexElement, "dexFile");
                    Object mCookie = RposedHelpers.getObjectField(dexFile, "mCookie");
                    Log.i(TAG, "get mCookie:" + mCookie);
                    NativeDump.fullDump(saveDir, mCookie);
                }

            }
        }).start();
    }
}
