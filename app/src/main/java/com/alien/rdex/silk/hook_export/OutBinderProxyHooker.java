package com.alien.rdex.silk.hook_export;

import android.app.PendingIntent;

public class OutBinderProxyHooker {
    public static java.lang.String developerPayload;
    public static java.lang.String sku;

    public void hookConsumeAsync(int i, de.robv.android.xposed.XC_MethodHook.MethodHookParam methodHookParam) {
        android.os.Parcel parcel = (android.os.Parcel) methodHookParam.args[2];
        android.util.Log.e("\u6307\u9488\uff1a", parcel.dataPosition() + "");
        parcel.readException();
        parcel.readInt();
        android.os.Bundle.CREATOR.createFromParcel(parcel);
        android.util.Log.e("\u6307\u9488\uff1a", parcel.dataPosition() + "");
        int dataPosition = parcel.dataPosition();
        parcel.writeNoException();
        if (i == 5) {
            parcel.writeInt(0);
            android.util.Log.e("\u4fee\u6b63\u6d88\u8017\u7ed3\u679c", "0");
        }
        if (i == 12) {
            parcel.writeInt(1);
            android.os.Bundle bundle = new android.os.Bundle();
            bundle.putInt("RESPONSE_CODE", 0);
            bundle.putString("DEBUG_MESSAGE", "success");
            bundle.writeToParcel(parcel, 0);
            android.util.Log.e("\u4fee\u6b63\u6d88\u8017\u7ed3\u679c", "12");
        }
        parcel.setDataPosition(dataPosition);
    }

    public void hookBuyIntent(android.os.Parcel parcel) {
        parcel.readInt();
        parcel.readString();
        sku = parcel.readString();
        parcel.readString();
        developerPayload = parcel.readString();
        android.util.Log.e("\u5165\u5e93", "\u6863\u4f4did : " + sku);
        android.util.Log.e("\u5165\u5e93", "developerPayload : " + developerPayload);
    }

    public void hookBinderProxy(de.robv.android.xposed.callbacks.XC_LoadPackage.LoadPackageParam loadPackageParam) {
        de.robv.android.xposed.XposedHelpers.findAndHookMethod("android.os.BinderProxy", loadPackageParam.classLoader, "transact", new java.lang.Object[]{java.lang.Integer.TYPE, android.os.Parcel.class, android.os.Parcel.class, java.lang.Integer.TYPE, new de.robv.android.xposed.XC_MethodHook() {
            /* class OutBinderProxyHooker.AnonymousClass1 */

            /* access modifiers changed from: protected */
            public void afterHookedMethod(de.robv.android.xposed.XC_MethodHook.MethodHookParam methodHookParam) throws java.lang.Throwable {
                super.afterHookedMethod(methodHookParam);
                int intValue = ((java.lang.Integer) methodHookParam.args[0]).intValue();
                if (intValue == 8 || intValue == 3 || intValue == 5 || intValue == 12) {
                    android.os.Parcel parcel = (android.os.Parcel) methodHookParam.args[1];
                    int dataPosition = parcel.dataPosition();
                    parcel.setDataPosition(0);
                    parcel.readInt();
                    if (android.text.TextUtils.equals(parcel.readString(), "com.android.vending.billing.IInAppBillingService")) {
                        if (intValue == 8 || intValue == 3) {
                            OutBinderProxyHooker.this.hookBuyIntent(parcel);
                        }
                        if (intValue == 5 || intValue == 12) {
                            android.util.Log.e("\u6d88\u8017code:", intValue + "");
                            OutBinderProxyHooker.this.hookConsumeAsync(intValue, methodHookParam);
                        }
                    }
                    parcel.setDataPosition(dataPosition);
                }
            }
        }});
    }

    public void hookBuyPendingIntent() {
        de.robv.android.xposed.XposedHelpers.findAndHookMethod(android.os.Bundle.class, "getParcelable", new java.lang.Object[]{java.lang.String.class, new de.robv.android.xposed.XC_MethodHook() {
            public void beforeHookedMethod(de.robv.android.xposed.XC_MethodHook.MethodHookParam methodHookParam) throws java.lang.Throwable {
                super.beforeHookedMethod(methodHookParam);
                if (android.text.TextUtils.equals((java.lang.String) methodHookParam.args[0], "BUY_INTENT")) {
                    android.app.Application currentApplication = android.app.AndroidAppHelper.currentApplication();
                    android.content.Intent intent = new android.content.Intent("PayDialogActivity");
                    intent.putExtra("sku", sku);
                    intent.putExtra("packageName", currentApplication.getPackageName());
                    methodHookParam.setResult(android.app.PendingIntent.getActivity(currentApplication, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT));
                }
            }
        }});
    }

    public void enter(de.robv.android.xposed.callbacks.XC_LoadPackage.LoadPackageParam loadPackageParam) {
        hookBuyPendingIntent();
        hookBinderProxy(loadPackageParam);
    }
}
