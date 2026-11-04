/**
 * @file
 */

#include "Android.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/sdl/SDLSystem.h"
#include "io/Stream.h"

#include <SDL3/SDL_system.h>
#include <jni.h>

namespace http {
namespace {

struct JniLocal {
	JNIEnv *env;
	jobject obj;

	explicit JniLocal(JNIEnv *e, jobject o = nullptr) : env(e), obj(o) {
	}

	~JniLocal() {
		if (obj != nullptr) {
			env->DeleteLocalRef(obj);
		}
	}

	void reset(jobject o) {
		if (obj != nullptr) {
			env->DeleteLocalRef(obj);
		}
		obj = o;
	}

	JniLocal(const JniLocal &) = delete;
	JniLocal &operator=(const JniLocal &) = delete;
};

bool jniFailed(JNIEnv *env) {
	return env->ExceptionCheck();
}

void logJniException(JNIEnv *env, const char *where) {
	jthrowable ex = env->ExceptionOccurred();
	if (ex == nullptr) {
		return;
	}
	env->ExceptionClear();
	JniLocal cls(env, env->GetObjectClass(ex));
	core::String message(where);
	if (cls.obj != nullptr) {
		jmethodID mid = env->GetMethodID(static_cast<jclass>(cls.obj), "toString", "()Ljava/lang/String;");
		if (mid != nullptr && !env->ExceptionCheck()) {
			JniLocal text(env, env->CallObjectMethod(ex, mid));
			if (env->ExceptionCheck()) {
				env->ExceptionClear();
			} else if (text.obj != nullptr) {
				const char *utf = env->GetStringUTFChars(static_cast<jstring>(text.obj), nullptr);
				if (utf != nullptr) {
					message.append(": ");
					message.append(utf);
					env->ReleaseStringUTFChars(static_cast<jstring>(text.obj), utf);
				}
			}
		} else if (env->ExceptionCheck()) {
			env->ExceptionClear();
		}
	}
	env->DeleteLocalRef(ex);
	Log::error("Http request failed: %s", message.c_str());
}

jstring newString(JNIEnv *env, const char *text) {
	if (text == nullptr) {
		return nullptr;
	}
	return env->NewStringUTF(text);
}

core::String fromString(JNIEnv *env, jstring value) {
	if (value == nullptr) {
		return core::String::Empty;
	}
	const char *utf = env->GetStringUTFChars(value, nullptr);
	if (utf == nullptr) {
		return core::String::Empty;
	}
	core::String out(utf);
	env->ReleaseStringUTFChars(value, utf);
	return out;
}

const char *requestMethod(RequestType type) {
	if (type == RequestType::GET) {
		return "GET";
	}
	if (type == RequestType::PATCH) {
		return "PATCH";
	}
	return "POST";
}

bool readInput(JNIEnv *env, jobject input, io::WriteStream &stream) {
	JniLocal inClass(env, env->GetObjectClass(input));
	if (jniFailed(env) || inClass.obj == nullptr) {
		logJniException(env, "InputStream");
		return false;
	}
	jmethodID readId = env->GetMethodID(static_cast<jclass>(inClass.obj), "read", "([B)I");
	jmethodID closeId = env->GetMethodID(static_cast<jclass>(inClass.obj), "close", "()V");
	if (jniFailed(env) || readId == nullptr) {
		logJniException(env, "InputStream.read");
		return false;
	}
	JniLocal buffer(env, env->NewByteArray(8192));
	if (jniFailed(env) || buffer.obj == nullptr) {
		logJniException(env, "allocate read buffer");
		return false;
	}
	uint8_t tmp[8192];
	for (;;) {
		const jint n = env->CallIntMethod(input, readId, buffer.obj);
		if (jniFailed(env)) {
			logJniException(env, "read response");
			break;
		}
		if (n < 0) {
			if (closeId != nullptr) {
				env->CallVoidMethod(input, closeId);
				if (env->ExceptionCheck()) {
					env->ExceptionClear();
				}
			}
			return true;
		}
		if (n == 0) {
			continue;
		}
		env->GetByteArrayRegion(static_cast<jbyteArray>(buffer.obj), 0, n, reinterpret_cast<jbyte *>(tmp));
		if (stream.write(tmp, (size_t)n) < 0) {
			return false;
		}
	}
	return false;
}

void collectHeaders(JNIEnv *env, jobject conn, jclass connClass, io::WriteStream &stream, Headers *outheaders) {
	jmethodID keyId = env->GetMethodID(connClass, "getHeaderFieldKey", "(I)Ljava/lang/String;");
	jmethodID valueId = env->GetMethodID(connClass, "getHeaderField", "(I)Ljava/lang/String;");
	if (jniFailed(env) || keyId == nullptr || valueId == nullptr) {
		if (env->ExceptionCheck()) {
			env->ExceptionClear();
		}
		return;
	}
	for (jint i = 0; i < 64; ++i) {
		JniLocal key(env, env->CallObjectMethod(conn, keyId, i));
		if (jniFailed(env)) {
			env->ExceptionClear();
			return;
		}
		JniLocal value(env, env->CallObjectMethod(conn, valueId, i));
		if (jniFailed(env)) {
			env->ExceptionClear();
			return;
		}
		if (key.obj == nullptr && value.obj == nullptr) {
			return;
		}
		if (key.obj == nullptr || value.obj == nullptr) {
			continue;
		}
		const core::String keyStr = fromString(env, static_cast<jstring>(key.obj));
		const core::String valueStr = fromString(env, static_cast<jstring>(value.obj));
		if (core::string::iequals(keyStr, "Content-Length")) {
			stream.reserve(valueStr.toInt());
		}
		if (outheaders != nullptr) {
			outheaders->put(keyStr, valueStr);
		}
	}
}

struct HttpConn {
	JNIEnv *env;
	jobject conn;

	~HttpConn() {
		if (conn == nullptr) {
			return;
		}
		jclass cls = env->GetObjectClass(conn);
		if (!env->ExceptionCheck() && cls != nullptr) {
			jmethodID mid = env->GetMethodID(cls, "disconnect", "()V");
			if (!env->ExceptionCheck() && mid != nullptr) {
				env->CallVoidMethod(conn, mid);
			}
		}
		if (env->ExceptionCheck()) {
			env->ExceptionClear();
		}
		if (cls != nullptr) {
			env->DeleteLocalRef(cls);
		}
		env->DeleteLocalRef(conn);
	}
};

} // namespace

static bool httpRequestJni(io::WriteStream &stream, int *statusCode, Headers *outheaders, RequestContext &ctx) {
	JNIEnv *env = static_cast<JNIEnv *>(SDL_GetAndroidJNIEnv());
	if (env == nullptr) {
		Log::error("Http request for '%s' failed: no JNI environment", ctx._url.c_str());
		return false;
	}

	JniLocal urlClass(env, env->FindClass("java/net/URL"));
	if (jniFailed(env) || urlClass.obj == nullptr) {
		logJniException(env, "java.net.URL");
		return false;
	}
	jmethodID urlCtor = env->GetMethodID(static_cast<jclass>(urlClass.obj), "<init>", "(Ljava/lang/String;)V");
	if (jniFailed(env) || urlCtor == nullptr) {
		logJniException(env, "URL constructor");
		return false;
	}
	JniLocal urlText(env, newString(env, ctx._url.c_str()));
	if (jniFailed(env) || urlText.obj == nullptr) {
		logJniException(env, "URL string");
		return false;
	}
	JniLocal url(env, env->NewObject(static_cast<jclass>(urlClass.obj), urlCtor, urlText.obj));
	if (jniFailed(env) || url.obj == nullptr) {
		logJniException(env, "open URL");
		return false;
	}
	jmethodID openConn = env->GetMethodID(static_cast<jclass>(urlClass.obj), "openConnection", "()Ljava/net/URLConnection;");
	if (jniFailed(env) || openConn == nullptr) {
		logJniException(env, "openConnection");
		return false;
	}
	HttpConn http{env, env->CallObjectMethod(url.obj, openConn)};
	if (jniFailed(env) || http.conn == nullptr) {
		logJniException(env, ctx._url.c_str());
		return false;
	}
	JniLocal connClass(env, env->GetObjectClass(http.conn));
	if (jniFailed(env) || connClass.obj == nullptr) {
		logJniException(env, "connection class");
		return false;
	}
	jclass connCls = static_cast<jclass>(connClass.obj);

	jmethodID setConnectTimeout = env->GetMethodID(connCls, "setConnectTimeout", "(I)V");
	jmethodID setReadTimeout = env->GetMethodID(connCls, "setReadTimeout", "(I)V");
	jmethodID setFollow = env->GetMethodID(connCls, "setInstanceFollowRedirects", "(Z)V");
	jmethodID setMethod = env->GetMethodID(connCls, "setRequestMethod", "(Ljava/lang/String;)V");
	jmethodID setProperty = env->GetMethodID(connCls, "setRequestProperty", "(Ljava/lang/String;Ljava/lang/String;)V");
	jmethodID setDoOutput = env->GetMethodID(connCls, "setDoOutput", "(Z)V");
	jmethodID getOutput = env->GetMethodID(connCls, "getOutputStream", "()Ljava/io/OutputStream;");
	jmethodID getCode = env->GetMethodID(connCls, "getResponseCode", "()I");
	jmethodID getInput = env->GetMethodID(connCls, "getInputStream", "()Ljava/io/InputStream;");
	jmethodID getError = env->GetMethodID(connCls, "getErrorStream", "()Ljava/io/InputStream;");
	if (jniFailed(env) || setConnectTimeout == nullptr || setReadTimeout == nullptr || setMethod == nullptr ||
		setProperty == nullptr || getCode == nullptr) {
		logJniException(env, "HttpURLConnection methods");
		return false;
	}

	env->CallVoidMethod(http.conn, setConnectTimeout, (jint)(ctx._connectTimeoutSecond * 1000));
	env->CallVoidMethod(http.conn, setReadTimeout, (jint)(ctx._timeoutSecond * 1000));
	if (setFollow != nullptr) {
		env->CallVoidMethod(http.conn, setFollow, ctx._followRedirects ? JNI_TRUE : JNI_FALSE);
	}
	if (jniFailed(env)) {
		logJniException(env, "configure connection");
		return false;
	}

	if (!ctx._userAgent.empty()) {
		JniLocal key(env, newString(env, "User-Agent"));
		JniLocal value(env, newString(env, ctx._userAgent.c_str()));
		if (key.obj != nullptr && value.obj != nullptr) {
			env->CallVoidMethod(http.conn, setProperty, key.obj, value.obj);
		}
	}
	for (const auto &entry : ctx._headers) {
		JniLocal key(env, newString(env, entry->first.c_str()));
		JniLocal value(env, newString(env, entry->second.c_str()));
		if (jniFailed(env) || key.obj == nullptr || value.obj == nullptr) {
			logJniException(env, "request header");
			return false;
		}
		env->CallVoidMethod(http.conn, setProperty, key.obj, value.obj);
		if (jniFailed(env)) {
			logJniException(env, "request header");
			return false;
		}
	}

	JniLocal methodText(env, newString(env, requestMethod(ctx._type)));
	env->CallVoidMethod(http.conn, setMethod, methodText.obj);
	if (jniFailed(env)) {
		logJniException(env, "request method");
		return false;
	}

	if (!ctx._body.empty()) {
		if (setDoOutput == nullptr || getOutput == nullptr) {
			Log::error("Http request for '%s' failed: no request body support", ctx._url.c_str());
			return false;
		}
		env->CallVoidMethod(http.conn, setDoOutput, JNI_TRUE);
		JniLocal output(env, env->CallObjectMethod(http.conn, getOutput));
		if (jniFailed(env) || output.obj == nullptr) {
			logJniException(env, "request body");
			return false;
		}
		JniLocal outClass(env, env->GetObjectClass(output.obj));
		jmethodID writeId = env->GetMethodID(static_cast<jclass>(outClass.obj), "write", "([B)V");
		jmethodID flushId = env->GetMethodID(static_cast<jclass>(outClass.obj), "flush", "()V");
		jmethodID closeId = env->GetMethodID(static_cast<jclass>(outClass.obj), "close", "()V");
		JniLocal bytes(env, env->NewByteArray((jsize)ctx._body.size()));
		if (jniFailed(env) || writeId == nullptr || bytes.obj == nullptr) {
			logJniException(env, "request body");
			return false;
		}
		env->SetByteArrayRegion(static_cast<jbyteArray>(bytes.obj), 0, (jsize)ctx._body.size(),
								reinterpret_cast<const jbyte *>(ctx._body.data()));
		env->CallVoidMethod(output.obj, writeId, bytes.obj);
		if (jniFailed(env)) {
			logJniException(env, "write request body");
			return false;
		}
		if (flushId != nullptr) {
			env->CallVoidMethod(output.obj, flushId);
		}
		if (closeId != nullptr) {
			env->CallVoidMethod(output.obj, closeId);
		}
		if (jniFailed(env)) {
			logJniException(env, "write request body");
			return false;
		}
	}

	const jint code = env->CallIntMethod(http.conn, getCode);
	if (jniFailed(env)) {
		logJniException(env, ctx._url.c_str());
		return false;
	}
	if (statusCode != nullptr) {
		*statusCode = (int)code;
	}
	Log::debug("Got status code %i for %s", (int)code, ctx._url.c_str());
	collectHeaders(env, http.conn, connCls, stream, outheaders);

	JniLocal input(env, nullptr);
	if (code >= 400 && getError != nullptr) {
		input.reset(env->CallObjectMethod(http.conn, getError));
	} else if (getInput != nullptr) {
		input.reset(env->CallObjectMethod(http.conn, getInput));
	}
	if (jniFailed(env)) {
		env->ExceptionClear();
		if (getError != nullptr) {
			input.reset(env->CallObjectMethod(http.conn, getError));
		}
		if (env->ExceptionCheck()) {
			logJniException(env, "response body");
			return false;
		}
	}
	if (input.obj == nullptr) {
		return code >= 0;
	}
	return readInput(env, input.obj, stream);
}

struct HttpJob {
	io::WriteStream *stream;
	int *statusCode;
	Headers *outheaders;
	RequestContext *ctx;
	bool result;
};

static int httpThread(void *userdata) {
	HttpJob *job = static_cast<HttpJob *>(userdata);
	job->result = httpRequestJni(*job->stream, job->statusCode, job->outheaders, *job->ctx);
	return 0;
}

bool http_request(io::WriteStream &stream, int *statusCode, Headers *outheaders, RequestContext &ctx) {
	// HttpURLConnection's TLS handshake is interpreted on the calling thread.
	// The SDL main thread stack is too small for that, so run it on an 8 MiB stack.
	HttpJob job{&stream, statusCode, outheaders, &ctx, false};
	SDL_PropertiesID props = SDL_CreateProperties();
	if (props == 0) {
		Log::error("Http request for '%s' failed: %s", ctx._url.c_str(), SDL_GetError());
		return false;
	}
	SDL_SetPointerProperty(props, SDL_PROP_THREAD_CREATE_ENTRY_FUNCTION_POINTER, reinterpret_cast<void *>(httpThread));
	SDL_SetPointerProperty(props, SDL_PROP_THREAD_CREATE_USERDATA_POINTER, &job);
	SDL_SetStringProperty(props, SDL_PROP_THREAD_CREATE_NAME_STRING, "vengi-http");
	SDL_SetNumberProperty(props, SDL_PROP_THREAD_CREATE_STACKSIZE_NUMBER, (Sint64)(8 * 1024 * 1024));
	SDL_Thread *thread = SDL_CreateThreadWithProperties(props);
	SDL_DestroyProperties(props);
	if (thread == nullptr) {
		Log::error("Http request for '%s' failed: %s", ctx._url.c_str(), SDL_GetError());
		return false;
	}
	SDL_WaitThread(thread, nullptr);
	return job.result;
}

} // namespace http
