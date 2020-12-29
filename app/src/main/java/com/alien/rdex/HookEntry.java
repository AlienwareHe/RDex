package com.alien.rdex;

import android.os.Build;
import android.os.Handler;
import android.util.Log;


import java.io.File;
import java.io.IOException;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import camel.external.org.apache.commons.io.FileUtils;
import dalvik.system.DexFile;
import de.robv.android.xposed.IXposedHookLoadPackage;
import de.robv.android.xposed.XC_MethodHook;
import de.robv.android.xposed.XSharedPreferences;
import de.robv.android.xposed.XposedBridge;
import de.robv.android.xposed.XposedHelpers;
import de.robv.android.xposed.callbacks.XC_LoadPackage;

public class HookEntry implements IXposedHookLoadPackage {


    public static final String TAG = "RDEX";

    private static Class DexClass;
    private static Method getBytesMethod;
    private static Method getDexMethod;
    private static boolean isJustInTime = false;
    private static Set<ClassLoader> classLoaders = new HashSet<>();
    private static String dexSaveDir = "/sdcard/";

    /**
     * 进行 注入的 app 名字
     */
    private static volatile String InvokPackage = null;
    private boolean isNeedInvoke = false;
    private XSharedPreferences shared;

    @Override
    public void handleLoadPackage(XC_LoadPackage.LoadPackageParam loadPackageParam) throws Throwable {
        Log.i(TAG, "rdex plugin hooked start");

        if (!loadPackageParam.processName.equals(loadPackageParam.packageName)) {
            Log.i(TAG, "rdex plugin not in main process,ignored:" + loadPackageParam.processName);
            return;
        }

        try {
            shared = new XSharedPreferences(BuildConfig.APPLICATION_ID, "config");
            shared.reload();
            InvokPackage = shared.getString("APP_INFO", "");
            isNeedInvoke = shared.getBoolean("NeedInvoke", false);

            //先重启 选择 好 要进行Hook的 app
            if ("".equals(InvokPackage) || !loadPackageParam.packageName.equals(InvokPackage)) {
                return;
            }
            Log.e("TAG", "发现 被Hook App" + loadPackageParam.packageName);
            Log.e("TAG", "是否需要 反射调用   " + isNeedInvoke);

            JNILoadHelper.loadLibrary("native-lib", this.getClass().getClassLoader());

            internalHook(loadPackageParam);

            if (!isJustInTime) {
                new Handler().postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        for (ClassLoader classLoader : classLoaders) {
                            Object[] nowElements = getClassLoaderElements(classLoader);
                            if (nowElements != null && nowElements.length != 0) {
                                dumpAllClass(nowElements, classLoader, null);
                            }
                        }
                    }
                }, 20 * 1000);
            }

        } catch (Throwable e) {
            Log.e(TAG, "rdex plugin hooked error:", e);
        }

        Log.i(TAG, "rdex plugin hooked finish");
    }

    private void internalHook(XC_LoadPackage.LoadPackageParam loadPackageParam) {

        XposedBridge.hookAllMethods(ClassLoader.class, "loadClass", new XC_MethodHook() {
            @Override
            protected void afterHookedMethod(MethodHookParam param) throws Throwable {
                final Class cls = (Class) param.getResult();
                if (cls == null) {
                    return;
                }
                handleClass(cls);
            }
        });
    }

    /**
     * 获取class对应的classLoader的DexElements数组
     *
     * @param cls 目标class
     */
    private void handleClass(Class cls) {
        ClassLoader classLoader = cls.getClassLoader();
        if (classLoader == null) {
            Log.i(TAG, "classloader is null:" + cls);
            return;
        }

        if (!isJustInTime) {
            classLoaders.add(classLoader);
        } else {
            Object[] nowElements = getClassLoaderElements(classLoader);
            if (nowElements != null && nowElements.length != 0) {
                dumpAllClass(nowElements, classLoader, cls);
            }
        }

    }

    private void dumpAllClass(final Object[] dexElements, final ClassLoader classLoader, final Class cls) {

        ThreadUtils.runOnNonUIThread(new Runnable() {
            @Override
            public void run() {
                try {
                    for (Object dexElement : dexElements) {
                        //每一个 elememt里面都有一个 dex文件
                        Field dexFileField = dexElement.getClass().getDeclaredField("dexFile");
                        dexFileField.setAccessible(true);
                        DexFile dexFile = (DexFile) dexFileField.get(dexElement);
                        if (dexFile == null) {
                            Log.w(TAG, "dexFile is null........");
                            continue;
                        }
                        //初始化每个类，防止填充式类抽取
                        Class firstClass = initAllClass(dexFile, classLoader);
                        if (!isJustInTime && firstClass != null) {
                            saveDexForClass(firstClass);
                        }
                    }
                    if (isJustInTime) {
                        saveDexForClass(cls);
                    }
                } catch (Throwable e) {
                    Log.e(TAG, "dumpAllClass error", e);
                }
            }
        });
    }

    /**
     * 根据 一个 Class保存 dex文件
     */
    private void saveDexForClass(Class mClass) {
        try {
            if (Build.VERSION.SDK_INT <= 25) {
                // 安卓7.1 可直接使用getDex方法
                initDexInvokeMethod();
                Object dex = getDexInClass(mClass);
                if (dex != null) {
                    saveDexToFile((byte[]) getBytesMethod.invoke(dex));
                }
            } else {
                // 否则通过mCookie dumpDex
                dumpDexByCookie(mClass);
            }
        } catch (Throwable e) {
            Log.e(TAG, "saveDexForClass error:", e);
        }
    }

    private HashSet<Object> foundCookies = new HashSet<>();

    /**
     * 通过mCookie dump类所对应的ClassLoader下所有的dex
     * <p>
     * 实际上可能有很多重复行为，因为一个classloader下有很多class，因此加了个mCookie集合去重稍微减少重复
     */
    private void dumpDexByCookie(Class mClass) {
        ClassLoader classLoader = mClass.getClassLoader();
        Object pathList = XposedHelpers.getObjectField(classLoader, "pathList");
        Object[] dexElements = (Object[]) XposedHelpers.getObjectField(pathList, "dexElements");
        for (Object dexElement : dexElements) {
            Object dexFile = XposedHelpers.getObjectField(dexElement, "dexFile");
            if (dexFile == null) {
                Log.w(TAG, "dexFile is null.....");
                continue;
            }
            Object mCookie = XposedHelpers.getObjectField(dexFile, "mCookie");
            if (foundCookies.contains(mCookie)) {
                continue;
            }
            foundCookies.add(mCookie);
            Log.i(TAG, "get mCookie:" + mCookie);
            NativeDump.fullDump(dexSaveDir, mCookie);
        }
    }

    private void saveDexToFile(byte[] bytes) throws IOException {
        if (bytes == null) {
            return;
        }
        //判断 这个 classloader是否 需要保存
        String dumpDexPath = "/sdcard/dump_" + bytes.length + ".dex";
        File file = new File(dumpDexPath);
        if (!file.exists()) {
            //不存在的时候 在保存
            FileUtils.writeByteArrayToFile(new File(dumpDexPath), bytes);
            Log.e(TAG, "save dex file:" + dumpDexPath);
        }
    }

    private Object getDexInClass(Class cls) {
        Object dex = null;
        try {
            if (cls == null) {
                return null;
            }
            dex = getDexMethod.invoke(cls);
        } catch (Throwable e) {
            Log.e(TAG, "getDexInClass  error  ", e);
        }
        return dex;
    }

    /**
     * 初始化反射需要调用的Dex.getBytes和Class.getDex等方法
     */
    private static void initDexInvokeMethod() {
        if (DexClass == null || getBytesMethod == null || getDexMethod == null) {
            try {
                DexClass = Class.forName("com.android.dex.Dex");
                getBytesMethod = DexClass.getDeclaredMethod("getBytes");
                getBytesMethod.setAccessible(true);
                //先拿到 class里面的 getDex方法
                Class classes = Class.forName("java.lang.Class");
                getDexMethod = classes.getDeclaredMethod("getDex");
                getDexMethod.setAccessible(true);
            } catch (Throwable e) {
                Log.e(TAG, "initDexInvokeMethod  error:", e);
            }
        }
    }

    private synchronized Class initAllClass(DexFile dexFile, final ClassLoader loader) {
        int classSize = getDexFileClassSize(dexFile);
        Log.e(TAG, "需要 加载的 getDexFileClassName个数:" + classSize);

        final List<Class> initialClasses = new ArrayList<>();

        try {

            Enumeration<String> enumeration = dexFile.entries();
            while (enumeration.hasMoreElements()) {//遍历
                final String className = enumeration.nextElement();
                try {
                    // 对每个 classloader都进行遍历
                    // 是否初始化需要根据对应加固手段确定，例如爱加密会设置炸弹类
                    initialClasses.add(Class.forName(className, false, loader));
                } catch (Throwable throwable) {

                }
            }
            if (!initialClasses.isEmpty()) {
                return initialClasses.get(0);
            }
            return null;
        } catch (Throwable e) {
            Log.e(TAG, "init all class error:", e);
            return null;
        }
    }

    private int getDexFileClassSize(DexFile dexFile) {
        int size = 0;
        Enumeration<String> entries = dexFile.entries();
        while (entries.hasMoreElements()) {
            //必须调用 nextElement 不然会卡死
            entries.nextElement();
            size++;
        }
        return size;
    }

    /**
     * 获取指定 Classloader Element数组
     */
    private Object[] getClassLoaderElements(ClassLoader loader) {
        try {

            Field pathListField = getPathListField(loader);
            if (pathListField == null) {
                Log.e(TAG, "getClassLoaderElements  获取 pathList == null");
                return null;
            }
            pathListField.setAccessible(true);
            Object dexPathList = pathListField.get(loader);
            Field dexElementsField = dexPathList.getClass().getDeclaredField("dexElements");
            dexElementsField.setAccessible(true);
            Object[] dexElements = (Object[]) dexElementsField.get(dexPathList);
            if (dexElements != null) {
                return dexElements;
            } else {
                Log.e(TAG, "getClassLoaderElements  获取 dexElements == null");
            }

        } catch (NoSuchFieldException e) {
            Log.e(TAG, "getClassLoaderElements  NoSuchFieldException:", e);
        } catch (IllegalAccessException e) {
            Log.e(TAG, "getClassLoaderElements  IllegalAccessException:", e);
        }
        return null;
    }

    /**
     * 根据classload 获取 pathList
     */
    private Field getPathListField(ClassLoader classLoader) {
        Field pathListField = null;
        Class classLoaderClass = classLoader.getClass();
        int maxFindLength = 5;
        while (classLoaderClass != null && pathListField == null && (maxFindLength--) > 0) {
            try {
                pathListField = classLoaderClass.getDeclaredField("pathList");
            } catch (Throwable e) {
                Log.i(TAG, "get path list error,try to find super class:" + classLoaderClass);
                classLoaderClass = classLoaderClass.getSuperclass();
            }
        }
        return pathListField;
    }
}
