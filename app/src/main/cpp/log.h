//
// Created by alienhe on 2020/12/23.
//
#ifndef ALOG_H_
#define ALOG_H_

#include <android/log.h>

#define TAG "RDEX"

#define ALOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__))
#define ALOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__))
#define ALOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__))
#define ALOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__))
#define ALOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__))

#endif