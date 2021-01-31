#include <fcntl.h>
#include <syscall.h>
#include <cstdio>
#include <cstdlib>
#include <jni.h>
#include <string>
#include <dlfcn.h>
#include "DexUtil.h"
#include <vector>
#include <unistd.h>
#include <fstream>
#include "Helper.h"
#include "dexfile_art.h"

extern int SDK_INT = 0;

static int dex_index = 0;

static void convert_java_array_to_dexfiles(JNIEnv *env, jobject mCookie,
                                           std::vector<const art::DexFile *> &dex_files);

static bool toDexFiles_8_0(JNIEnv *env, jobject mCookie, std::vector<const art::DexFile *> &dex_files);


int dump_complete_dex(art::DexFile *dexFile, char save_path[256]);

extern "C"
JNIEXPORT void JNICALL
Java_com_alien_rdex_NativeDump_init(JNIEnv *env, jclass clazz, jint api_level) {
    SDK_INT = api_level;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_alien_rdex_NativeDump_fullDump(JNIEnv *env, jclass clazz, jstring dump_path,
                                        jobject m_cookie) {

    // convert m_cookie to dex_file
    std::vector<const art::DexFile *> dex_files;
    convert_java_array_to_dexfiles(env, m_cookie, dex_files);
    if (dex_files.empty()) {
        ALOGD("convert mcookie to dex files error");
        return;
    }

    const char *save_dir = "/sdcard";
    if (dump_path != nullptr) {
        ScopedUtfChars path_str(env, dump_path);
        save_dir = path_str.c_str();
    }

    ALOGI("save dir of dex: %s", save_dir);

    static char save_path[256];
    for (int j = 0; j < dex_files.size(); j++) {
        memset(save_path, 0, 256);
        snprintf(save_path, 255, "%s%d%s", save_dir, dex_index++, ".dex");
        ALOGI("save path of dex: %s", save_path);
        art::DexFile *dexFile = const_cast<art::DexFile *> (dex_files.at(j));
        if (dexFile == nullptr) {
            ALOGI("[-] Dump dex of path: %s failed,because dexFile is null", save_path);
            continue;
        }
        if (dump_complete_dex(dexFile, save_path) == JNI_ERR) {
            ALOGI("[-] Dump dex of path: %s failed!", save_path);
        }
    }
}

int dump_complete_dex(art::DexFile *dexFile, char *save_path) {
    int fd = open(save_path, O_CREAT | O_EXCL | O_WRONLY, 0777);
    if (fd < 0) {
        ALOGE("[-] create file %s failed, %s", save_path, strerror(errno));
        return JNI_ERR;
    }

    write(fd, dexFile->begin_, dexFile->size_);
    close(fd);
    return JNI_OK;
}


void convert_java_array_to_dexfiles(JNIEnv *env, jobject mCookie,
                                    std::vector<const art::DexFile *> &dex_files) {
    if (SDK_INT >= 26) {
        // android_8.0
        toDexFiles_8_0(env, mCookie, dex_files);
    }else{
        ALOGI("安卓8以下直接使用Dex#getBytes方法即可");
    }
}

/**
 * http://androidxref.com/8.0.0_r4/xref/art/runtime/native/dalvik_system_DexFile.cc#ConvertDexFilesToJavaArray
 *
 * @param env JNIEnv
 * @param mCookie DexFile#mCookie
 * @return
 */
bool toDexFiles_8_0(JNIEnv *env, jobject mCookie, std::vector<const art::DexFile *> &dex_files) {
    jarray array = reinterpret_cast<jarray>(mCookie);
    jsize array_size = env->GetArrayLength(array);
    if (env->ExceptionCheck() == JNI_TRUE) {
        return false;
    }

    jboolean is_long_data_copied;
    jlong *long_data = env->GetLongArrayElements(reinterpret_cast<jlongArray>(array),
                                                 &is_long_data_copied);
    if (env->ExceptionCheck() == JNI_TRUE) {
        return false;
    }

    ALOGI("get mCookie size:%d", array_size);
    dex_files.reserve(array_size - 1);
    // kDexFileIndexStart = 1
    for (jsize i = 1; i < array_size; ++i) {
        dex_files.push_back(
                reinterpret_cast<const art::DexFile *>(static_cast<uintptr_t>(long_data[i])));
    }

    return true;
}

