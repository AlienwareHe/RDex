package com.alien.rdex;

import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Environment;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by lyh on 2018/9/18.
 */

public class PermissionUtils {


    //判断哪些权限 未授权
    private static List<String> mPermissionList = new ArrayList<>();

    /**
     * 初始化权限的方法
     */
    public static void initPermission(Activity activity, String[] PERMISSIONS_STORAGE) {
        //先检查权限 如果是 6.0一下直接 过
        if (isGetPermission(activity, PERMISSIONS_STORAGE)) {
            return;
        } else {
            ActivityCompat.requestPermissions(activity, mPermissionList.
                    toArray(new String[mPermissionList.size()]), 1);
        }

    }


    /**
     * 检测全选代码
     *
     * @param Permission 检测权限的 字符串数组
     * @return 是否都获取到
     */
    private static boolean isGetPermission(Context context, String[] Permission) {
        //大于6.0
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            //检测权限的 集合
            mPermissionList.clear();
            for (String s : Permission) {
                if (ContextCompat.checkSelfPermission(context, s) != PackageManager.PERMISSION_GRANTED) {
                    //将没有获取到的权限 加到集合里
                    mPermissionList.add(s);
                }

            }
            if (mPermissionList.size() == 0) {
                return true;
            } else {
                return false;
            }
        }
        //小于6.0直接true
        return true;
    }
}
