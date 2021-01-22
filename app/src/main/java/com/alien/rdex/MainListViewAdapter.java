package com.alien.rdex;

import android.content.Context;
import android.content.SharedPreferences;
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

public class MainListViewAdapter extends BaseAdapter {

    private ArrayList<AppBean> data;

    private final Context mContext;

    private final CheckBox cb_invoke;


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

        holder.all.setOnClickListener(new View.OnClickListener() {
                                          @Override
                                          public void onClick(View v) {
                                              MainListViewAdapter.this.save(position);
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

    private void deletedOATFile(String getPackageName) {
        //删掉 OAT FILE
        for (int i = 0; i < 4; i++) {
            File file = new File("/data/app/" + getPackageName + (i == 0 ? "/oat" : "-" + i + "/oat"));
            if (file.exists()) {
                Log.i("TAG", "发现 OAT 文件尝试删除");
                RootUtils.execShell("rm -r " + file.getPath());
                return;
            }
        }
    }

    private void save(int position) {
        SharedPreferences.Editor editor = MultiprocessSharedPreferences.getSharedPreferences(mContext, Constants.SP_NAME, Context.MODE_PRIVATE).edit();
        editor.putString(Constants.APP_INFO,data.get(position).packageName);
        editor.putBoolean(Constants.IS_NEED_INVOKE,cb_invoke.isChecked());
        editor.apply();

        ToastUtils.showToast(mContext, "设置dump目标app:" + data.get(position).packageName);
        deletedOATFile(data.get(position).packageName);
    }


    private static class ViewHolder {
        TextView tv_appName, tv_packageName;
        LinearLayout all;
        ImageView iv_appIcon;

        ViewHolder(View convertView) {
            all = convertView.findViewById(R.id.ll_all);
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
