package com.alien.rdex;

import android.content.Context;
import android.util.Log;
import android.widget.Toast;

public class ToastUtils {

    private static Toast toast;

    public static void showToast(final Context context, final String msg) {
        Log.i(HookEntry.TAG,msg);
        ThreadUtils.runOnMainThread(new Runnable() {
            @Override
            public void run() {
                if (toast == null) {
                    //如果toast第一次创建 makeText创建toast对象
                    toast = Toast.makeText(context.getApplicationContext(), msg, Toast.LENGTH_SHORT);

                } else {
                    //如果toast存在 只需要修改文字
                    toast.setText(msg);
                }
                toast.show();
            }
        });
    }
}