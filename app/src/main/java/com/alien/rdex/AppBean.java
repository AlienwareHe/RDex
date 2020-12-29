package com.alien.rdex;

import android.graphics.drawable.Drawable;

public class AppBean {


    public String appName;

    public String packageName;

    public Drawable appIcon;


    public boolean isSystemApp=true;


    @Override
    public String toString() {
        return "AppBean{" +
                "appName='" + appName + '\'' +
                ", packageName='" + packageName + '\'' +
                ", appIcon=" + appIcon +
                ", isSystemApp=" + isSystemApp +
                '}';
    }
}