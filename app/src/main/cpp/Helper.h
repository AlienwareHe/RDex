//
// VirtualApp Native Project
//

#ifndef NDK_HELPER
#define NDK_HELPER

// A smart pointer that provides read-only access to a Java string's UTF chars.
// Unlike GetStringUTFChars, we throw NullPointerException rather than abort if
// passed a null jstring, and c_str will return NULL.
// This makes the correct idiom very simple:
//
//   ScopedUtfChars name(env, java_name);
//   if (name.c_str() == NULL) {
//     return NULL;
//   }
class ScopedUtfChars {
public:
    ScopedUtfChars(JNIEnv *env, jstring s) : env_(env), string_(s) {
        if (s == NULL) {
            utf_chars_ = NULL;
            // jniThrowNullPointerException(env, NULL);
        } else {
            utf_chars_ = env->GetStringUTFChars(s, NULL);
        }
    }

    ~ScopedUtfChars() {
        if (utf_chars_) {
            env_->ReleaseStringUTFChars(string_, utf_chars_);
        }
    }

    const char *c_str() const {
        return utf_chars_;
    }

    size_t size() const {
        return strlen(utf_chars_);
    }

    const char &operator[](size_t n) const {
        return utf_chars_[n];
    }

private:
    JNIEnv *env_;
    jstring string_;
    const char *utf_chars_;

    // Disallow copy and assignment.
    ScopedUtfChars(const ScopedUtfChars &);

    void operator=(const ScopedUtfChars &);
};

#endif //NDK_HELPER
