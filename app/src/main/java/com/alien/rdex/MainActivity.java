package com.alien.rdex;

import android.Manifest;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.util.Log;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.ListView;
import android.widget.Toolbar;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import java.util.ArrayList;
import java.util.List;

public class MainActivity extends AppCompatActivity {

    public static final String TAG = "dump";

    private ListView mLv_list;
    private ArrayList<AppBean> mAllPackageList = new ArrayList<>();
    private ArrayList<AppBean> CommonPackageList = new ArrayList<>();
    private Toolbar mToolbar;

    String[] permissionList = {
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.INSTALL_PACKAGES,
            Manifest.permission.GET_PACKAGE_SIZE
    };

    private CheckBox mCb_checkbox;
    private CheckBox mCb_invoke;

    private MainListViewAdapter mMainListViewAdapter;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        initData();
        initView();
        PermissionUtils.initPermission(this, permissionList);

    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == 1 && grantResults.length > 0) {//将这里的requestCode改成你的任意数字，上边的一致就行
            boolean granted = grantResults[0] == PackageManager.PERMISSION_GRANTED;//是否授权，可以根据permission作为标记
            if (granted) {//true为授予了

            } else {////没有授予，比如可以给用户弹窗告诉用户，你拒绝了权限，所以不能实现某个功能，如果想实现你可以跳转到设置，如果不想实现那么直接把这个弹窗取消

            }
        }
    }


    private void initData() {
        mAllPackageList = getPackageList();
        Log.e("TAG", "发现 " + mAllPackageList.size() + " App");
    }

    private void initView() {
        mLv_list = (ListView) findViewById(R.id.lv_list);

        mToolbar = (Toolbar) findViewById(R.id.tb_toolbar);

        mCb_checkbox = (CheckBox) findViewById(R.id.cb_checkbox);
        mCb_invoke = (CheckBox) findViewById(R.id.cb_invoke);

        mMainListViewAdapter = new MainListViewAdapter(this, CommonPackageList, mCb_invoke);
        mCb_checkbox.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                    //需要显示 系统 app
                    mMainListViewAdapter.setData(mAllPackageList);
                } else {
                    mMainListViewAdapter.setData(CommonPackageList);
                }
            }
        });
        mToolbar.setTitle("");

        mLv_list.setAdapter(mMainListViewAdapter);
        mMainListViewAdapter.notifyDataSetChanged();
    }

    public ArrayList<AppBean> getPackageList() {
        PackageManager packageManager = getPackageManager();
        List<PackageInfo> packages = packageManager.getInstalledPackages(0);
        ArrayList<AppBean> appBeans = new ArrayList<>();

        for (PackageInfo packageInfo : packages) {
            AppBean appBean = new AppBean();
            // 判断系统/非系统应用
            // 非系统应用
            // 系统应用
            appBean.isSystemApp = (packageInfo.applicationInfo.flags & ApplicationInfo.FLAG_SYSTEM) != 0;
            appBean.appName = packageInfo.applicationInfo.loadLabel(getPackageManager()).toString();
            appBean.packageName = packageInfo.packageName;
            appBean.appIcon = packageInfo.applicationInfo.loadIcon(getPackageManager());

            appBeans.add(appBean);

            if (!appBean.isSystemApp) {
                CommonPackageList.add(appBean);
            }

        }
        return appBeans;
    }
}
