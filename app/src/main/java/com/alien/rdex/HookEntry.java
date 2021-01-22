package com.alien.rdex;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.Handler;
import android.text.TextUtils;
import android.util.Log;
import android.widget.Toast;

import com.android.system.ISystemLoadPackage;
import com.android.system.MethodHook;
import com.android.system.SystemBridge;
import com.android.system.SystemHelpers;
import com.android.system.callbacks.LoadPackage;
import com.camel.api.CamelToolKit;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.atomic.AtomicBoolean;

import camel.external.org.apache.commons.io.FileUtils;
import dalvik.system.DexFile;

public class HookEntry implements ISystemLoadPackage {


    public static final String TAG = "RDEX";

    private static Class<?> dexClass;
    private static Method getBytesMethod;
    private static Method getDexMethod;
    private static final boolean isJustInTime = false;
    private static final Set<ClassLoader> classLoaders = new HashSet<>();
    private static String dexSaveDir = "/sdcard/";

    /**
     * 进行 注入的 app 名字
     */
    private static volatile String targetDumpPakcage = null;
    private boolean isNeedInvoke = false;
    private static final AtomicBoolean soInited = new AtomicBoolean(false);

    @Override
    public void handleLoadPackage(final LoadPackage.LoadPackageParam loadPackageParam) throws Throwable {
        Log.i(TAG, "rdex plugin hooked start");

        if (!loadPackageParam.processName.equals(loadPackageParam.packageName)) {
            Log.i(TAG, "rdex plugin not in main process,ignored:" + loadPackageParam.processName);
            return;
        }

        internalHook(loadPackageParam);

        SystemHelpers.findAndHookMethod(Activity.class, "onResume", new MethodHook() {
            @Override
            protected void afterHookedMethod(MethodHookParam param) {
                CamelToolKit.sContext = (Context) param.thisObject;
                if (!soInited.compareAndSet(false, true)) {
                    return;
                }

                try {
                    JNILoadHelper.loadLibrary("native-lib", this.getClass().getClassLoader());

                    dexSaveDir = CamelToolKit.sContext.getFilesDir() + "/";
                    // 先在插件中选择好要进行Hook的app
                    MultiprocessSharedPreferences.setAuthority("com.alien.rdex.MultiprocessSharedPreferences");
                    SharedPreferences sharedPreferences = MultiprocessSharedPreferences.getSharedPreferences(CamelToolKit.sContext, Constants.SP_NAME, Context.MODE_PRIVATE);
                    targetDumpPakcage = sharedPreferences.getString(Constants.APP_INFO, "");
                    isNeedInvoke = sharedPreferences.getBoolean(Constants.IS_NEED_INVOKE, false);

                    if (TextUtils.isEmpty(targetDumpPakcage) || !loadPackageParam.packageName.equals(targetDumpPakcage)) {
                        Log.i(TAG, "Not the target dump app:" + targetDumpPakcage + ",current package name:" + loadPackageParam.packageName);
                        return;
                    }

                    Log.e(TAG, "发现 被Hook App" + loadPackageParam.packageName);
                    Log.e(TAG, "是否需要 反射调用:" + isNeedInvoke);

                    if (!isJustInTime) {
                        new Handler().postDelayed(new Runnable() {
                            @Override
                            public void run() {
                                ToastUtils.showToast(CamelToolKit.sContext, "找到Classloader数量:" + classLoaders.size());
                                for (ClassLoader classLoader : classLoaders) {
                                    Object[] nowElements = getClassLoaderElements(classLoader);
                                    if (nowElements != null && nowElements.length != 0) {
                                        dumpAllClass(nowElements, classLoader, null);
                                    }
                                }
                            }
                        }, 20 * 1000);
                        ToastUtils.showToast(CamelToolKit.sContext, "20秒后开始DUMP对应DEX文件");
                    }
                } catch (Throwable e) {
                    Log.e(TAG, "dump app failed!", e);
                }
            }
        });

        Log.i(TAG, "rdex plugin hooked finish");
    }

    private void internalHook(LoadPackage.LoadPackageParam loadPackageParam) {

        SystemBridge.hookAllMethods(ClassLoader.class, "loadClass", new MethodHook() {
            @Override
            protected void afterHookedMethod(MethodHookParam param) throws Throwable {
                final Class<?> cls = (Class<?>) param.getResult();
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
    private void handleClass(Class<?> cls) {
        ClassLoader classLoader = cls.getClassLoader();
        if (classLoader == null) {
            Log.i(TAG, "classloader is null:" + cls);
            return;
        }

        if (!isJustInTime) {
            Log.i(TAG,"监听到新的classLoader:" + classLoader);
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
                    ToastUtils.showToast(CamelToolKit.sContext, "找到dexElements数量:" + dexElements.length);
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
                        Class<?> firstClass = initAllClass(dexFile, classLoader);
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
    private void saveDexForClass(Class<?> mClass) {
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

    private final HashSet<Object> foundCookies = new HashSet<>();

    /**
     * 通过mCookie dump类所对应的ClassLoader下所有的dex
     * <p>
     * 实际上可能有很多重复行为，因为一个classloader下有很多class，因此加了个mCookie集合去重稍微减少重复
     */
    private void dumpDexByCookie(Class<?> mClass) {
        ClassLoader classLoader = mClass.getClassLoader();
        Object pathList = SystemHelpers.getObjectField(classLoader, "pathList");
        Object[] dexElements = (Object[]) SystemHelpers.getObjectField(pathList, "dexElements");
        for (Object dexElement : dexElements) {
            Object dexFile = SystemHelpers.getObjectField(dexElement, "dexFile");
            if (dexFile == null) {
                Log.w(TAG, "dexFile is null.....");
                continue;
            }
            Object mCookie = SystemHelpers.getObjectField(dexFile, "mCookie");
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
        String dumpDexPath = dexSaveDir + bytes.length + ".dex";
        File file = new File(dumpDexPath);
        if (!file.exists()) {
            //不存在的时候 在保存
            FileUtils.writeByteArrayToFile(new File(dumpDexPath), bytes);
            Log.e(TAG, "save dex file:" + dumpDexPath);
        }
    }

    private Object getDexInClass(Class<?> cls) {
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
    @SuppressLint("PrivateApi")
    private static void initDexInvokeMethod() {
        if (dexClass == null || getBytesMethod == null || getDexMethod == null) {
            try {
                dexClass = Class.forName("com.android.dex.Dex");
                getBytesMethod = dexClass.getDeclaredMethod("getBytes");
                getBytesMethod.setAccessible(true);
                //先拿到 class里面的 getDex方法
                Class<?> classes = Class.forName("java.lang.Class");
                getDexMethod = classes.getDeclaredMethod("getDex");
                getDexMethod.setAccessible(true);
            } catch (Throwable e) {
                Log.e(TAG, "initDexInvokeMethod  error:", e);
            }
        }
    }

    private synchronized Class<?> initAllClass(DexFile dexFile, final ClassLoader loader) {
        int classSize = getDexFileClassSize(dexFile);
        Log.e(TAG, "需要 加载的 getDexFileClassName个数:" + classSize);

        final List<Class<?>> initialClasses = new ArrayList<>();

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
            assert dexPathList != null;
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
        Class<?> classLoaderClass = classLoader.getClass();
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
