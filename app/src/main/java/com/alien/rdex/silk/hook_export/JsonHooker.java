package com.alien.rdex.silk.hook_export;

public class JsonHooker {
    private void hookJson(de.robv.android.xposed.callbacks.XC_LoadPackage.LoadPackageParam loadPackageParam) {
        try {
            de.robv.android.xposed.XposedHelpers.findAndHookMethod("org.json.JSONObject", loadPackageParam.classLoader, "getString", new java.lang.Object[]{java.lang.String.class, new de.robv.android.xposed.XC_MethodHook() {
                /* class com.game.hook_export.JsonHooker.AnonymousClass1 */

                public void beforeHookedMethod(de.robv.android.xposed.XC_MethodHook.MethodHookParam methodHookParam) throws java.lang.Throwable {
                    java.lang.String str;
                    beforeHookedMethod(methodHookParam);
                    if (android.text.TextUtils.equals((java.lang.String) methodHookParam.args[0], "developerPayload") && (str = OutBinderProxyHooker.developerPayload) != null) {
                        methodHookParam.setResult(str);
                        android.util.Log.e("\u5165\u5e93", "\u585e\u5165developerPayload =" + str);
                    }
                }
            }});
            de.robv.android.xposed.XposedHelpers.findAndHookMethod("org.json.JSONObject", loadPackageParam.classLoader, "optString", new java.lang.Object[]{java.lang.String.class, new de.robv.android.xposed.XC_MethodHook() {
                /* class com.game.hook_export.JsonHooker.AnonymousClass2 */

                public void beforeHookedMethod(de.robv.android.xposed.XC_MethodHook.MethodHookParam methodHookParam) throws java.lang.Throwable {
                    java.lang.String str;
                    super.beforeHookedMethod(methodHookParam);
                    if (android.text.TextUtils.equals((java.lang.String) methodHookParam.args[0], "developerPayload") && (str = OutBinderProxyHooker.developerPayload) != null) {
                        methodHookParam.setResult(str);
                        android.util.Log.e("\u5165\u5e93", "\u585e\u5165developerPayload =" + str);
                    }
                }
            }});
        } catch (java.lang.Throwable unused) {
        }
    }

    public void hook(de.robv.android.xposed.callbacks.XC_LoadPackage.LoadPackageParam loadPackageParam) {
        hookJson(loadPackageParam);
    }
}
