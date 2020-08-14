package com.alien.rdex;

import android.util.Log;

import com.camel.api.CamelToolKit;
import com.camel.api.rposed.IRposedHookLoadPackage;
import com.camel.api.rposed.RC_MethodHook;
import com.camel.api.rposed.RposedBridge;
import com.camel.api.rposed.callbacks.RC_LoadPackage;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.Enumeration;

import camel.external.org.apache.commons.io.FileUtils;
import dalvik.system.DexFile;

public class HookEntry implements IRposedHookLoadPackage {


    public static final String TAG = "RDEX";

    private static Class DexClass;
    private static Method getBytesMethod;
    private static Method getDexMethod;

    @Override
    public void handleLoadPackage(RC_LoadPackage.LoadPackageParam loadPackageParam) throws Throwable {
        Log.i(TAG, "rdex plugin hooked start");

        try {
            internalHook(loadPackageParam);
        } catch (Throwable e) {
            Log.e(TAG, "rdex plugin hooked error:", e);
        }

        Log.i(TAG, "rdex plugin hooked finish");
    }

    private void internalHook(RC_LoadPackage.LoadPackageParam loadPackageParam) {

        RposedBridge.hookAllMethods(ClassLoader.class, "loadClass", new RC_MethodHook() {
            @Override
            protected void afterHookedMethod(MethodHookParam param) throws Throwable {
                Class cls = (Class) param.getResult();
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

        Object[] nowElements = getClassLoaderElements(classLoader);
        if (nowElements != null && nowElements.length != 0) {
            dumpAllClass(nowElements, classLoader, cls);
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
                        //初始化每个类，防止填充式类抽取
                        initAllClass(dexFile, classLoader);
                    }
                    saveDexForClass(cls);
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
            initDexInvokeMethod();
            Object dex = getDexInClass(mClass);
            if (dex != null) {
                saveDexToFile((byte[]) getBytesMethod.invoke(dex));
            }
        } catch (Throwable e) {
            Log.e(TAG, "saveDexForClass error:", e);
        }
    }

    private void saveDexToFile(byte[] bytes) throws IOException {
        if (bytes == null) {
            return;
        }
        //判断 这个 classloader是否 需要保存
        String dumpDexPath = CamelToolKit.whiteSdcardDirPath + "dump_" + bytes.length + ".dex";
        File file = new File(dumpDexPath);
        if (!file.exists()) {
            //不存在的时候 在保存
            FileUtils.writeByteArrayToFile(new File(dumpDexPath), bytes);
            Log.i(TAG, "save dex file:" + dumpDexPath);
        }
    }

    private Object getDexInClass(Class cls) {
        Object dex = null;
        try {
            if (cls != null) {
                dex = getDexMethod.invoke(cls);
            }
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

    private synchronized void initAllClass(DexFile dexFile, final ClassLoader loader) {
        int classSize = getDexFileClassSize(dexFile);
        Log.e(TAG, "需要 加载的 getDexFileClassName个数:" + classSize);

        try {

            Enumeration<String> enumeration = dexFile.entries();
            while (enumeration.hasMoreElements()) {//遍历
                final String className = enumeration.nextElement();
                try {
                    //对每个 classloader都进行遍历
                    ThreadUtils.runOnMainThread(new Runnable() {
                        @Override
                        public void run() {
                            try {
                                Class.forName(className, true, loader);
                            } catch (Throwable ignored) {

                            }
                        }
                    });
                } catch (Throwable throwable) {

                }
            }

        } catch (Throwable e) {
            Log.e(TAG, "init all class error:", e);
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
        while (pathListField == null && (maxFindLength--) > 0) {
            try {
                pathListField = classLoaderClass.getDeclaredField("pathList");
            } catch (Throwable e) {
                classLoaderClass = classLoaderClass.getSuperclass();
            }
        }
        return pathListField;
    }
}
