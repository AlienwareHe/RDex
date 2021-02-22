package com.alien.rdex.silk.hook_export;

public class ExportHooker {
    public void hook(de.robv.android.xposed.callbacks.XC_LoadPackage.LoadPackageParam loadPackageParam) {
        new OutBinderProxyHooker().enter(loadPackageParam);
        new JsonHooker().hook(loadPackageParam);
    }
}
