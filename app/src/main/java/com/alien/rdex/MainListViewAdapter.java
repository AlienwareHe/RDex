package com.alien.rdex;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.PixelFormat;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.NinePatchDrawable;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.CheckBox;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.io.File;
import java.util.ArrayList;


/**
 * Created by lyh on 2019/2/14.
 */
public class MainListViewAdapter extends BaseAdapter {

    public static String APP_INFO = "APP_INFO";
    public static String NeedInvoke = "NeedInvoke";


    private ArrayList<AppBean> data;

    private Context mContext;

    private CheckBox cb_invoke;


    public MainListViewAdapter(Context context, ArrayList<AppBean> data, CheckBox cb_invoke) {
        this.mContext = context;
        this.data = data;
        this.cb_invoke = cb_invoke;
    }


    public void setData(ArrayList<AppBean> data) {

        this.data = data;

        notifyDataSetChanged();
    }


    @Override
    public int getCount() {
        return data == null ? 0 : data.size();
    }

    @Override
    public Object getItem(int position) {
        return data.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(final int position, View convertView, ViewGroup parent) {

        if (convertView == null) {
            convertView = View.inflate(mContext, R.layout.activity_list_item, null);
        }
        ViewHolder holder = ViewHolder.getHolder(convertView);
        AppBean appBean = data.get(position);

        holder.iv_appIcon.setImageBitmap(drawable2Bitmap(appBean.appIcon));
        holder.tv_appName.setText(appBean.appName);
        holder.tv_packageName.setText(appBean.packageName);

        holder.All.setOnClickListener(new View.OnClickListener() {
                                          @Override
                                          public void onClick(View v) {
                                              MainListViewAdapter.this.Save(position);
                                          }
                                      }
        );
        return convertView;
    }

    public static Bitmap drawable2Bitmap(Drawable drawable) {
        if (drawable instanceof BitmapDrawable) {
            return ((BitmapDrawable) drawable).getBitmap();
        } else if (drawable instanceof NinePatchDrawable) {
            Bitmap bitmap = Bitmap
                    .createBitmap(
                            drawable.getIntrinsicWidth(),
                            drawable.getIntrinsicHeight(),
                            drawable.getOpacity() != PixelFormat.OPAQUE ? Bitmap.Config.ARGB_8888
                                    : Bitmap.Config.RGB_565);
            Canvas canvas = new Canvas(bitmap);
            drawable.setBounds(0, 0, drawable.getIntrinsicWidth(),
                    drawable.getIntrinsicHeight());
            drawable.draw(canvas);
            return bitmap;
        } else {
            return null;
        }
    }


    private void DeletedOATFile(String getPackageName) {
        //删掉 OAT FILE
        File file = new File("/data/app/" + getPackageName + "/oat");
        if (file.exists()) {
            Log.i("TAG", "发现 OAT 文件尝试删除");
            RootUtils.execShell("rm -r " + file.getPath());
            return;
        }
        File file1 = new File("/data/app/" + getPackageName + "-1/oat");
        if (file1.exists()) {
            RootUtils.execShell("rm -r " + file1.getPath());
            Log.i("TAG", "删除OAT文件成功");
            return;
        }
        File file2 = new File("/data/app/" + getPackageName + "-2/oat");
        if (file2.exists()) {
            RootUtils.execShell("rm -r " + file2.getPath());
            return;
        }
        File file3 = new File("/data/app/" + getPackageName + "-3/oat");
        if (file3.exists()) {
            RootUtils.execShell("rm -r " + file3.getPath());
            return;
        }
    }

    private void Save(int position) {
        SpUtil.putString(mContext, APP_INFO, data.get(position).packageName);
        SpUtil.putBoolean(mContext, NeedInvoke, cb_invoke.isChecked());

        ToastUtils.showToast(mContext, "保存成功 dex路径为    data/data/" + data.get(position).packageName);
        DeletedOATFile(data.get(position).packageName);
    }


    private static class ViewHolder {
        TextView tv_appName, tv_packageName;
        LinearLayout All;
        ImageView iv_appIcon;

        ViewHolder(View convertView) {
            All = convertView.findViewById(R.id.ll_all);
            tv_packageName = convertView.findViewById(R.id.tv_packName);
            tv_appName = convertView.findViewById(R.id.tv_appName);
            iv_appIcon = convertView.findViewById(R.id.iv_appIcon);
        }

        static ViewHolder getHolder(View convertView) {
            ViewHolder holder = (ViewHolder) convertView.getTag();
            if (holder == null) {
                holder = new ViewHolder(convertView);
                convertView.setTag(holder);
            }
            return holder;
        }
    }
}
