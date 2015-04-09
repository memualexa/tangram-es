#ifdef PLATFORM_ANDROID

#include "platform.h"

#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <jni.h>
#include <cstdarg>

/* Followed the following document for JavaVM tips when used with native threads
 * http://android.wooyd.org/JNIExample/#NWD1sCYeT-I
 * http://developer.android.com/training/articles/perf-jni.html and
 * http://www.ibm.com/developerworks/library/j-jni/
 * http://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/invocation.html
 */

static AAssetManager* assetManager;

static JNIEnv* jniEnv;
static jobject tangramInstance;

void cacheJniEnv(JNIEnv* _jniEnv) {
    jniEnv = _jniEnv;
}

void cacheTangramInstance(jobject _tangramInstance) {
    tangramInstance = jniEnv->NewGlobalRef(_tangramInstance);
}

void setAssetManager(jobject _assetManager) {

    assetManager = AAssetManager_fromJava(jniEnv, _assetManager);

    if (assetManager == nullptr) {
        logMsg("ERROR: Could not obtain Asset Manager reference\n");
    }

}

void logMsg(const char* fmt, ...) {

    va_list args;
    va_start(args, fmt);
    __android_log_vprint(ANDROID_LOG_DEBUG, "Tangram", fmt, args);
    va_end(args);

}

std::string stringFromResource(const char* _path) {

    std::string out;
    
    // Open asset
    AAsset* asset = AAssetManager_open(assetManager, _path, AASSET_MODE_STREAMING);
    
    if (asset == nullptr) {
        logMsg("Failed to open asset at path: %s\n", _path);
        return out;
    }

    // Allocate string
    int length = AAsset_getLength(asset);
    out.resize(length);
    
    // Read data
    int read = AAsset_read(asset, &out.front(), length);

    // Clean up
    AAsset_close(asset);

    if (read <= 0) {
        logMsg("Failed to read asset at path: %s\n", _path);
    }

    return out;

}

unsigned char* bytesFromResource(const char* _path, unsigned int* _size) {

    unsigned char* data = nullptr;

    AAsset* asset = AAssetManager_open(assetManager, _path, AASSET_MODE_UNKNOWN);

    if (asset == nullptr) {
        logMsg("Failed to open asset at path: %s\n", _path);
        *_size = 0;
        return data;
    }

    *_size = AAsset_getLength(asset);

    data = (unsigned char*) malloc(sizeof(unsigned char) * (*_size));

    int read = AAsset_read(asset, data, *_size);

    if (read <= 0) {
        logMsg("Failed to read asset at path: %s\n", _path);
    }

    AAsset_close(asset);

    return data;
}

bool streamFromHttpASync(const std::string& _url, const TileID& _tileID, const int _dataSourceID) {
    jstring jUrl;
    jboolean methodResult;

    jclass tangramClass = jniEnv->FindClass("com/mapzen/tangram/Tangram");

    jmethodID method = jniEnv->GetMethodID(tangramClass, "networkRequest", "(Ljava/lang/String;IIII)Z");

    jUrl = jniEnv->NewStringUTF(_url.c_str());

    methodResult = jniEnv->CallBooleanMethod(tangramInstance, method, jUrl, (jint)_tileID.x, (jint)_tileID.y, (jint)_tileID.z, (jint)_dataSourceID);

    if(!methodResult) {
        logMsg("\"networkRequest\" returned false");
        return methodResult;
    }

    return methodResult;
}


#endif
