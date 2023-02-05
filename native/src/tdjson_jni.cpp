#include "io_github_ablearthy_tdjson_TDJson.h"

#include <td/telegram/td_json_client.h>

#include <cstdint>
#include <string>
#include <memory>

using int32 = std::int32_t;
using uint32 = std::uint32_t;

class JvmThreadDetacher {
 public:
  explicit JvmThreadDetacher(JavaVM *java_vm) : java_vm_(java_vm) {
  }

  JvmThreadDetacher(const JvmThreadDetacher &other) = delete;
  JvmThreadDetacher &operator=(const JvmThreadDetacher &other) = delete;
  JvmThreadDetacher(JvmThreadDetacher &&other) : java_vm_(other.java_vm_) {
    other.java_vm_ = nullptr;
  }
  JvmThreadDetacher &operator=(JvmThreadDetacher &&other) = delete;
  ~JvmThreadDetacher() {
    detach();
  }

  void operator()(JNIEnv *env) {
    detach();
  }
 private:
  JavaVM *java_vm_;

  void detach() {
    if (java_vm_ != nullptr) {
      java_vm_->DetachCurrentThread();
      java_vm_ = nullptr;
    }
  }
};


std::string from_jstring(JNIEnv *env, jstring s);
jstring to_jstring(JNIEnv* env, const std::string& s);

std::unique_ptr<JNIEnv, JvmThreadDetacher> get_jni_env(JavaVM *java_vm, jint jni_version);

/*
 * Class:      io_github_ablearthy_tdjson_TDJson
 * Method:     td_1create_1client_1id
 * Signature:  ()I
 */
JNIEXPORT jint JNICALL Java_io_github_ablearthy_tdjson_TDJson_td_1create_1client_1id
  (JNIEnv *env, jobject clazz) {
    return td_create_client_id();
  }

/*
 * Class:      io_github_ablearthy_tdjson_TDJson
 * Method:     td_1receive
 * Signature:  (D)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_io_github_ablearthy_tdjson_TDJson_td_1receive
  (JNIEnv *env, jobject clazz, jdouble timeout) {
    const char* res = td_receive(timeout);
    if (res) {
      return to_jstring(env, res);
    } else {
      return to_jstring(env, "");
    }
}

/*
 * Class:      io_github_ablearthy_tdjson_TDJson
 * Method:     td_1send
 * Signature:  (ILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_io_github_ablearthy_tdjson_TDJson_td_1send
  (JNIEnv *env, jobject clazz, jint client_id, jstring request) {
    td_send(client_id, from_jstring(env, request).c_str());
}

/*
 * Class:      io_github_ablearthy_tdjson_TDJson
 * Method:     td_1execute
 * Signature:  (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_io_github_ablearthy_tdjson_TDJson_td_1execute
  (JNIEnv *env, jobject clazz, jstring request) {
    const char* result = td_execute(from_jstring(env, request).c_str());
    if (result) {
      return to_jstring(env, result);
    } else {
      return to_jstring(env, "");
    }
}

static constexpr jint JAVA_VERSION = JNI_VERSION_1_6;
static JavaVM *java_vm;
static jobject log_message_handler;

static void on_log_message(int verbosity_level, const char* log_message) {
    auto env = get_jni_env(java_vm, JAVA_VERSION);

    jobject handler = env->NewLocalRef(log_message_handler);
    if (!handler) {
        return;
    }
    jclass handler_class = env->GetObjectClass(handler);
    if (handler_class) {
        jmethodID on_log_message_method = env->GetMethodID(handler_class, "onMessage", "(ILjava/lang/String;)V");
        if (on_log_message_method) {
            jstring log_message_str = to_jstring(env.get(), log_message);
            if (log_message_str) {
                env->CallVoidMethod(handler, on_log_message_method, verbosity_level, log_message_str);
                env->DeleteLocalRef((jobject)log_message_str);
            }
        }
        env->DeleteLocalRef((jobject)handler_class);
    }

    env->DeleteLocalRef(handler);
}

/*
 * Class:      io_github_ablearthy_tdjson_TDJson
 * Method:     td_1set_1log_1message_1callback
 * Signature:  (ILio/github/ablearthy/tdjson/LogMessageHandler;)V
 */
JNIEXPORT void JNICALL Java_io_github_ablearthy_tdjson_TDJson_td_1set_1log_1message_1callback
  (JNIEnv *env, jobject clazz, jint max_verbosity_level, jobject new_handler) {
    if (log_message_handler) {
        td_set_log_message_callback(0, nullptr);
        jobject old = log_message_handler;
        log_message_handler = jobject();
        env->DeleteGlobalRef(old);
    }

    if (new_handler) {
        log_message_handler = env->NewGlobalRef(new_handler);
        if (!log_message_handler) { return; }

        td_set_log_message_callback(max_verbosity_level, on_log_message);
    }
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    java_vm = vm;
    return JAVA_VERSION;
}

// util
// td/tl/tl_jni_object.cpp

static void utf8_to_utf16(const char *p, size_t len, jchar *res) {
  // UTF-8 correctness is supposed
  for (size_t i = 0; i < len;) {
    uint32 a = static_cast<unsigned char>(p[i++]);
    if (a >= 0x80) {
      uint32 b = static_cast<unsigned char>(p[i++]);
      if (a >= 0xe0) {
        uint32 c = static_cast<unsigned char>(p[i++]);
        if (a >= 0xf0) {
          uint32 d = static_cast<unsigned char>(p[i++]);
          uint32 val = ((a & 0x07) << 18) + ((b & 0x3f) << 12) + ((c & 0x3f) << 6) + (d & 0x3f) - 0x10000;
          *res++ = static_cast<jchar>(0xD800 + (val >> 10));
          *res++ = static_cast<jchar>(0xDC00 + (val & 0x3ff));
        } else {
          *res++ = static_cast<jchar>(((a & 0x0f) << 12) + ((b & 0x3f) << 6) + (c & 0x3f));
        }
      } else {
        *res++ = static_cast<jchar>(((a & 0x1f) << 6) + (b & 0x3f));
      }
    } else {
      *res++ = static_cast<jchar>(a);
    }
  }
}

static void utf16_to_utf8(const jchar *p, jsize len, char *res) {
  for (jsize i = 0; i < len; i++) {
    uint32 cur = p[i];
    // TODO conversion uint32 -> signed char is implementation defined
    if (cur <= 0x7f) {
      *res++ = static_cast<char>(cur);
    } else if (cur <= 0x7ff) {
      *res++ = static_cast<char>(0xc0 | (cur >> 6));
      *res++ = static_cast<char>(0x80 | (cur & 0x3f));
    } else if ((cur & 0xF800) != 0xD800) {
      *res++ = static_cast<char>(0xe0 | (cur >> 12));
      *res++ = static_cast<char>(0x80 | ((cur >> 6) & 0x3f));
      *res++ = static_cast<char>(0x80 | (cur & 0x3f));
    } else {
      // correctness is already checked
      uint32 next = p[++i];
      uint32 val = ((cur - 0xD800) << 10) + next - 0xDC00 + 0x10000;

      *res++ = static_cast<char>(0xf0 | (val >> 18));
      *res++ = static_cast<char>(0x80 | ((val >> 12) & 0x3f));
      *res++ = static_cast<char>(0x80 | ((val >> 6) & 0x3f));
      *res++ = static_cast<char>(0x80 | (val & 0x3f));
    }
  }
}

static size_t get_utf8_from_utf16_length(const jchar *p, jsize len) {
  size_t result = 0;
  for (jsize i = 0; i < len; i++) {
    uint32 cur = p[i];
    if ((cur & 0xF800) == 0xD800) {
      if (i < len) {
        uint32 next = p[++i];
        if ((next & 0xFC00) == 0xDC00 && (cur & 0x400) == 0) {
          result += 4;
          continue;
        }
      }

      // TODO wrong UTF-16, it is possible
      return 0;
    }
    result += 1 + (cur >= 0x80) + (cur >= 0x800);
  }
  return result;
}

static jsize get_utf16_from_utf8_length(const char *p, size_t len, jsize *surrogates) {
  // UTF-8 correctness is supposed
  jsize result = 0;
  for (size_t i = 0; i < len; i++) {
    result += ((p[i] & 0xc0) != 0x80);
    *surrogates += ((p[i] & 0xf8) == 0xf0);
  }
  return result;
}

std::string from_jstring(JNIEnv *env, jstring s) {
  if (!s) {
    return std::string();
  }
  jsize s_len = env->GetStringLength(s);
  const jchar *p = env->GetStringChars(s, nullptr);
  if (p == nullptr) {
    return std::string();
  }
  size_t len = get_utf8_from_utf16_length(p, s_len);
  std::string res(len, '\0');
  if (len) {
    utf16_to_utf8(p, s_len, &res[0]);
  }
  env->ReleaseStringChars(s, p);
  return res;
}

jstring to_jstring(JNIEnv* env, const std::string& s) {
  jsize surrogates = 0;
  jsize unicode_len = get_utf16_from_utf8_length(s.c_str(), s.size(), &surrogates);
  if (surrogates == 0) {
    // TODO '\0'
    return env->NewStringUTF(s.c_str());
  }
  jsize result_len = surrogates + unicode_len;
  if (result_len <= 256) {
    jchar result[256];
    utf8_to_utf16(s.c_str(), s.size(), result);
    return env->NewString(result, result_len);
  }

  auto result = std::make_unique<jchar[]>(result_len);
  utf8_to_utf16(s.c_str(), s.size(), result.get());
  return env->NewString(result.get(), result_len);
}

std::unique_ptr<JNIEnv, JvmThreadDetacher> get_jni_env(JavaVM *vm, jint jni_version) {
  JNIEnv *env = nullptr;
  if (vm->GetEnv(reinterpret_cast<void **>(&env), jni_version) == JNI_EDETACHED) {
#ifdef JDK1_2  // if not Android JNI
    auto p_env = reinterpret_cast<void **>(&env);
#else
    auto p_env = &env;
#endif
    if (vm->AttachCurrentThread(p_env, nullptr) != JNI_OK) {
      vm = nullptr;
      env = nullptr;
    }
  } else {
    vm = nullptr;
  }

  return std::unique_ptr<JNIEnv, JvmThreadDetacher>(env, JvmThreadDetacher(vm));
}