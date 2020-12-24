package com.alien.rdex;

import android.os.Build;

import androidx.annotation.NonNull;

public class NativeDump {

    static {
        init(Build.VERSION.SDK_INT);
    }

    public static native void init(int apiLevel);

    public static native void fullDump(@NonNull String dumpPath, @NonNull Object mCookie);
}
