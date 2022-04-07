package com.alien.rdex;

import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Process;
import android.util.Log;

import androidx.annotation.RequiresApi;

import com.android.system.SystemHelpers;
import com.camel.api.CamelToolKit;
import com.camel.api.rposed.RposedHelpers;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.lang.reflect.Array;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;

public class JNILoadHelper {

    // TODO 64位/32位的so如何加载，不然如果目标为32位应用，需要手动编译仅支持32位的so
    @RequiresApi(api = Build.VERSION_CODES.M)
    public static void loadLibraryPro(String libName, ClassLoader classLoader){
        String libPath = "";
        if(Process.is64Bit()){

        }
        if(Build.VERSION.SDK_INT >=28){
            SystemHelpers.callMethod(Runtime.getRuntime(),"nativeLoad",libPath,classLoader);
        }else{
            SystemHelpers.callMethod(Runtime.getRuntime(),"doLoad",libPath,classLoader);
        }
    }

    public static void loadLibrary(String libName, ClassLoader targetClassLoader) {
        try {
            List<ApplicationInfo> list = CamelToolKit.sContext.getPackageManager().getInstalledApplications(PackageManager.GET_META_DATA);
            for (ApplicationInfo info : list) {
                if (info.packageName.equals("com.alien.rdex")) {
                    Log.i(HookEntry.TAG, info.nativeLibraryDir);
                    JNILoadHelper.insertNativeLibraryPathElements(new File(info.nativeLibraryDir), targetClassLoader);
                    RposedHelpers.callMethod(Runtime.getRuntime(), "loadLibrary0", targetClassLoader, libName);
                }
            }
        } catch (Throwable throwable) {
            Log.e(HookEntry.TAG, "JNILoadHelper load library error:" + libName, throwable);
        }
    }

    public static void loadLibrary(String libName) {
        loadLibrary(libName, CamelToolKit.sContext.getClassLoader());
    }

    public static void insertNativeLibraryPathElements(File soDirFile, ClassLoader pathClassLoader) {
        Object pathList = getPathList(pathClassLoader);
        Log.i(HookEntry.TAG, String.format("[insertNativeLibrary] pathList: %s", pathList));
        if (pathList != null) {
            Field nativeLibraryPathElementsField = null;
            try {

                Method makePathElements;
                Object invokeMakePathElements;
                boolean isNewVersion = Build.VERSION.SDK_INT > Build.VERSION_CODES.N_MR1;
                //调用makePathElements
                makePathElements = isNewVersion ? pathList.getClass().getDeclaredMethod("makePathElements", List.class) : pathList.getClass().getDeclaredMethod("makePathElements", List.class, List.class, ClassLoader.class);
                makePathElements.setAccessible(true);
                ArrayList<IOException> suppressedExceptions = new ArrayList<>();
                List<File> nativeLibraryDirectories = new ArrayList<>();
                nativeLibraryDirectories.add(soDirFile);
                List<File> allNativeLibraryDirectories = new ArrayList<>(nativeLibraryDirectories);
                //获取systemNativeLibraryDirectories
                Field systemNativeLibraryDirectoriesField = pathList.getClass().getDeclaredField("systemNativeLibraryDirectories");
                systemNativeLibraryDirectoriesField.setAccessible(true);
                List<File> systemNativeLibraryDirectories = (List<File>) systemNativeLibraryDirectoriesField.get(pathList);
                Log.i(HookEntry.TAG, String.format("[insertNativeLibrary] systemNativeLibraryDirectories: %s", systemNativeLibraryDirectories));
                allNativeLibraryDirectories.addAll(systemNativeLibraryDirectories);
                invokeMakePathElements = isNewVersion ? makePathElements.invoke(pathClassLoader, allNativeLibraryDirectories) : makePathElements.invoke(pathClassLoader, allNativeLibraryDirectories, suppressedExceptions, pathClassLoader);
                Log.i("insertNativeLibrary", "makePathElements " + invokeMakePathElements);

                nativeLibraryPathElementsField = pathList.getClass().getDeclaredField("nativeLibraryPathElements");
                nativeLibraryPathElementsField.setAccessible(true);
                Object list = nativeLibraryPathElementsField.get(pathList);
                Log.i(HookEntry.TAG, String.format("[insertNativeLibrary] nativeLibraryPathElements: %s", list));
                Object dexElementsValue = combineArray(list, invokeMakePathElements);
                //把组合后的nativeLibraryPathElements设置到系统中
                nativeLibraryPathElementsField.set(pathList, dexElementsValue);
            } catch (NoSuchFieldException | IllegalAccessException | NoSuchMethodException | InvocationTargetException e) {
                Log.e(HookEntry.TAG, "insert native library elements error:", e);
            }
        }
    }

    public static Object combineArray(Object hostDexElementValue, Object pluginDexElementValue) {
        //获取原数组类型
        Class<?> localClass = hostDexElementValue.getClass().getComponentType();
        Log.i(HookEntry.TAG, String.format("[insertNativeLibrary] localClass: %s", localClass));
        //获取原数组长度
        int i = Array.getLength(hostDexElementValue);
        //插件数组加上原数组的长度
        int j = i + Array.getLength(pluginDexElementValue);
        //创建一个新的数组用来存储
        Object result = Array.newInstance(localClass, j);
        //一个个的将dex文件设置到新数组中
        for (int k = 0; k < j; ++k) {
            if (k < i) {
                Array.set(result, k, Array.get(hostDexElementValue, k));
            } else {
                Array.set(result, k, Array.get(pluginDexElementValue, k - i));
            }
        }
        return result;
    }

    public static Object getPathList(Object classLoader) {
        Class cls = null;
        String pathListName = "pathList";
        try {
            cls = Class.forName("dalvik.system.BaseDexClassLoader");
            Field declaredField = cls.getDeclaredField(pathListName);
            declaredField.setAccessible(true);
            return declaredField.get(classLoader);
        } catch (ClassNotFoundException | NoSuchFieldException | IllegalAccessException e) {
            Log.e(HookEntry.TAG, "JNILoadHelper get path list error:", e);
        }
        return null;
    }

    public static ArrayList<String> getLoadedSos() {
        ArrayList<String> list = new ArrayList<>();
        int pid = Process.myPid();
        String path = "/proc/" + pid + "/maps";
        File file = new File(path);
        if (file.exists() && file.isFile()) {
            BufferedReader reader = null;
            try {
                reader = new BufferedReader(new FileReader(file));
                String temp = null;
                while ((temp = reader.readLine()) != null) {
                    if (temp.endsWith(".so")) {
                        int index = temp.indexOf("/");
                        if (index != -1) {
                            String str = temp.substring(index);
                            list.add(str);
                        }
                    }
                }
                reader.close();
            } catch (Exception e) {
                e.printStackTrace();
            } finally {
                if (reader != null) {
                    try {
                        reader.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            }
        }
        return list;
    }


}
