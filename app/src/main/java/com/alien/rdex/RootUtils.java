package com.alien.rdex;

import android.util.Log;

import java.io.DataOutputStream;
import java.io.IOException;
import java.io.OutputStream;

/**
 * Created by Lyh on
 * 2019/10/28
 */
public class RootUtils {
    /**
     * 应用程序运行命令获取 Root权限，设备必须已破解(获得ROOT权限)
     *
     * @return 应用程序是/否获取Root权限
     */
    public static boolean upgradeRootPermission(String pkgCodePath) {
        Process process = null;
        DataOutputStream os = null;
        try {
            String cmd = "chmod 777 " + pkgCodePath;
            process = Runtime.getRuntime().exec("su"); //切换到root帐号
            os = new DataOutputStream(process.getOutputStream());
            os.writeBytes(cmd + "\n");
            os.writeBytes("exit\n");
            os.flush();
            process.waitFor();
        } catch (Exception e) {
            return false;
        } finally {
            try {
                if (os != null) {
                    os.close();
                }
                if (process != null) {
                    process.destroy();
                }
            } catch (Exception e) {
            }
        }
        return true;
    }

    public static void execShell(String cmd) {
        OutputStream outputStream = null;
        DataOutputStream dataOutputStream = null;
        try {

            Runtime runtime = Runtime.getRuntime();
            runtime.exec(cmd + "\n");
            //权限设置
            Process p = Runtime.getRuntime().exec("su");
            //获取输出流
            outputStream = p.getOutputStream();
            dataOutputStream = new DataOutputStream(outputStream);
            //将命令写入
            dataOutputStream.writeBytes(cmd + "\n");
            //提交命令
            dataOutputStream.flush();

        } catch (Throwable t) {
            Log.e("TAG", "错误信息 " + t.getMessage());
            t.printStackTrace();
        } finally {
            if (outputStream != null) {
                try {
                    outputStream.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
            if (dataOutputStream != null) {
                try {
                    dataOutputStream.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }

    }
}
