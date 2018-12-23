#include "androidPlatform.h"
#include "data/clientGeoJsonSource.h"
#include "map.h"

#include <cassert>
#include <android/bitmap.h>

using namespace Tangram;

#define FUNC(CLASS, NAME) JNICALL Java_com_mapzen_tangram_ ## CLASS ## _ ## NAME

#define auto_map(ptr) assert(ptr); auto map = reinterpret_cast<Tangram::AndroidMap*>(mapPtr)

std::vector<Tangram::SceneUpdate> unpackSceneUpdates(JNIEnv* jniEnv, jobjectArray updateStrings) {
    size_t nUpdateStrings = (updateStrings == NULL)? 0 : jniEnv->GetArrayLength(updateStrings);

    std::vector<Tangram::SceneUpdate> sceneUpdates;
    for (size_t i = 0; i < nUpdateStrings;) {
        jstring path = (jstring) (jniEnv->GetObjectArrayElement(updateStrings, i++));
        jstring value = (jstring) (jniEnv->GetObjectArrayElement(updateStrings, i++));
        sceneUpdates.emplace_back(stringFromJString(jniEnv, path), stringFromJString(jniEnv, value));
        jniEnv->DeleteLocalRef(path);
        jniEnv->DeleteLocalRef(value);
    }
    return sceneUpdates;
}

extern "C" {

    JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
        return AndroidPlatform::jniOnLoad(vm);
    }

    JNIEXPORT void JNI_OnUnload(JavaVM* vm, void* reserved) {
        AndroidPlatform::jniOnUnload(vm);
    }


#define MapRenderer(NAME) FUNC(MapRenderer, NAME)

    JNIEXPORT jboolean MapRenderer(nativeUpdate)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat dt) {
        auto_map(mapPtr);
        auto result = map->update(dt);
        return static_cast<jboolean>(result);
    }

    JNIEXPORT jboolean MapRenderer(nativeRender)(JNIEnv* jniEnv, jobject obj, jlong mapPtr) {
        auto_map(mapPtr);
        return map->render();
    }

    JNIEXPORT void MapRenderer(nativeSetupGL)(JNIEnv* jniEnv, jobject obj, jlong mapPtr) {
        AndroidPlatform::bindJniEnvToThread(jniEnv);
        auto_map(mapPtr);
        map->setupGL();
    }

    JNIEXPORT void MapRenderer(nativeResize)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jint width, jint height) {
        auto_map(mapPtr);
        map->resize(width, height);
    }

    JNIEXPORT void MapRenderer(nativeCaptureSnapshot)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jintArray buffer) {
        auto_map(mapPtr);
        jint* ptr = jniEnv->GetIntArrayElements(buffer, NULL);
        unsigned int* data = reinterpret_cast<unsigned int*>(ptr);
        map->captureSnapshot(data);
        jniEnv->ReleaseIntArrayElements(buffer, ptr, JNI_ABORT);
    }

#undef MapRenderer


#define MapController(NAME) FUNC(MapController, NAME)

    JNIEXPORT void MapController(nativeGetCameraPosition)(JNIEnv* jniEnv, jobject obj, jlong mapPtr,
                                                          jdoubleArray lonLat, jfloatArray zoomRotationTilt) {
        auto_map(mapPtr);
        jdouble* pos = jniEnv->GetDoubleArrayElements(lonLat, NULL);
        jfloat* zrt = jniEnv->GetFloatArrayElements(zoomRotationTilt, NULL);

        auto camera = map->getCameraPosition();
        pos[0] = camera.longitude;
        pos[1] = camera.latitude;
        zrt[0] = camera.zoom;
        zrt[1] = camera.rotation;
        zrt[2] = camera.tilt;

        jniEnv->ReleaseDoubleArrayElements(lonLat, pos, 0);
        jniEnv->ReleaseFloatArrayElements(zoomRotationTilt, zrt, 0);
    }

    JNIEXPORT void MapController(nativeUpdateCameraPosition)(JNIEnv* jniEnv, jobject obj, jlong mapPtr,
                                                             jint set, jdouble lon, jdouble lat,
                                                             jfloat zoom, jfloat zoomBy,
                                                             jfloat rotation, jfloat rotateBy,
                                                             jfloat tilt, jfloat tiltBy,
                                                             jdouble b1lon, jdouble b1lat,
                                                             jdouble b2lon, jdouble b2lat,
                                                             jintArray jpad, jfloat duration, jint ease) {
        auto_map(mapPtr);

        CameraUpdate update;
        update.set = set;

        update.lngLat = LngLat{lon,lat};
        update.zoom = zoom;
        update.zoomBy = zoomBy;
        update.rotation = rotation;
        update.rotationBy = rotateBy;
        update.tilt = tilt;
        update.tiltBy = tiltBy;
        update.bounds = std::array<LngLat,2>{{LngLat{b1lon, b1lat}, LngLat{b2lon, b2lat}}};
        if (jpad != NULL) {
            jint* jpadArray = jniEnv->GetIntArrayElements(jpad, NULL);
            update.padding = EdgePadding{jpadArray[0], jpadArray[1], jpadArray[2], jpadArray[3]};
            jniEnv->ReleaseIntArrayElements(jpad, jpadArray, JNI_ABORT);
        }
        map->updateCameraPosition(update, duration, static_cast<Tangram::EaseType>(ease));
    }

    JNIEXPORT void MapController(nativeGetEnclosingCameraPosition)(JNIEnv* jniEnv, jobject obj, jlong mapPtr,
                                                                   jdouble aLng, jdouble aLat,
                                                                   jdouble bLng, jdouble bLat,
                                                                   jintArray jpad, jdoubleArray lngLatZoom) {
        auto_map(mapPtr);

        EdgePadding padding;
        if (jpad != NULL) {
            jint* jpadArray = jniEnv->GetIntArrayElements(jpad, NULL);
            padding = EdgePadding(jpadArray[0], jpadArray[1], jpadArray[2], jpadArray[3]);
            jniEnv->ReleaseIntArrayElements(jpad, jpadArray, JNI_ABORT);
        }
        CameraPosition camera = map->getEnclosingCameraPosition(LngLat{aLng,aLat}, LngLat{bLng,bLat}, padding);
        jdouble* arr = jniEnv->GetDoubleArrayElements(lngLatZoom, NULL);
        arr[0] = camera.longitude;
        arr[1] = camera.latitude;
        arr[2] = camera.zoom;
        jniEnv->ReleaseDoubleArrayElements(lngLatZoom, arr, 0);
    }

    JNIEXPORT void MapController(nativeFlyTo)(JNIEnv* jniEnv, jobject obj,  jlong mapPtr, jdouble lon, jdouble lat,
                                              jfloat zoom, jfloat duration, jfloat speed) {
        auto_map(mapPtr);

        CameraPosition camera = map->getCameraPosition();
        camera.longitude = lon;
        camera.latitude = lat;
        camera.zoom = zoom;
        map->flyTo(camera, duration, speed);
    }

    JNIEXPORT void MapController(nativeCancelCameraAnimation)(JNIEnv* jniEnv, jobject obj,  jlong mapPtr) {
        auto_map(mapPtr);

        map->cancelCameraAnimation();
    }

    JNIEXPORT jboolean MapController(nativeScreenPositionToLngLat)(JNIEnv* jniEnv, jobject obj, jlong mapPtr,
                                                                   jdoubleArray coordinates) {
        auto_map(mapPtr);

        jdouble* arr = jniEnv->GetDoubleArrayElements(coordinates, NULL);
        bool result = map->screenPositionToLngLat(arr[0], arr[1], &arr[0], &arr[1]);
        jniEnv->ReleaseDoubleArrayElements(coordinates, arr, 0);
        return static_cast<jboolean>(result);
    }

    JNIEXPORT jboolean MapController(nativeLngLatToScreenPosition)(JNIEnv* jniEnv, jobject obj, jlong mapPtr,
                                                                   jdoubleArray coordinates) {
        auto_map(mapPtr);

        jdouble* arr = jniEnv->GetDoubleArrayElements(coordinates, NULL);
        bool result = map->lngLatToScreenPosition(arr[0], arr[1], &arr[0], &arr[1]);
        jniEnv->ReleaseDoubleArrayElements(coordinates, arr, 0);
        return static_cast<jboolean>(result);
    }

    JNIEXPORT jlong MapController(nativeInit)(JNIEnv* jniEnv, jobject tangramInstance, jobject assetManager) {
        auto map  = new Tangram::AndroidMap(jniEnv, assetManager, tangramInstance);
        return reinterpret_cast<jlong>(map);
    }

    JNIEXPORT void MapController(nativeDispose)(JNIEnv* jniEnv, jobject tangramInstance, jlong mapPtr) {
        auto_map(mapPtr);

        delete map;
    }

    JNIEXPORT void MapController(nativeShutdown)(JNIEnv* jniEnv, jobject tangramInstance, jlong mapPtr) {
        auto_map(mapPtr);

        map->getPlatform()->shutdown();
    }

    JNIEXPORT jint MapController(nativeLoadScene)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jstring path,
                                                  jobjectArray updateStrings) {
        auto_map(mapPtr);

        auto cPath = stringFromJString(jniEnv, path);

        auto sceneUpdates = unpackSceneUpdates(jniEnv, updateStrings);
        Url sceneUrl = Url(cPath).resolved("asset:///");
        jint sceneId = map->loadScene(sceneUrl.string(), false, sceneUpdates);

        return sceneId;
    }

    JNIEXPORT jint MapController(nativeLoadSceneAsync)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jstring path,
                                                       jobjectArray updateStrings) {
        auto_map(mapPtr);

        auto cPath = stringFromJString(jniEnv, path);

        auto sceneUpdates = unpackSceneUpdates(jniEnv, updateStrings);
        Url sceneUrl = Url(cPath).resolved("asset:///");
        jint sceneId = map->loadSceneAsync(sceneUrl.string(), false, sceneUpdates);

        return sceneId;


    }

    JNIEXPORT jint MapController(nativeLoadSceneYaml)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jstring yaml,
                                                      jstring path, jobjectArray updateStrings) {
        auto_map(mapPtr);

        auto cYaml = stringFromJString(jniEnv, yaml);
        auto cPath = stringFromJString(jniEnv, path);

        auto sceneUpdates = unpackSceneUpdates(jniEnv, updateStrings);
        Url sceneUrl = Url(cPath).resolved("asset:///");
        jint sceneId = map->loadSceneYaml(cYaml, sceneUrl.string(), false, sceneUpdates);

        return sceneId;
    }

    JNIEXPORT jint MapController(nativeLoadSceneYamlAsync)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jstring yaml,
                                                           jstring path, jobjectArray updateStrings) {
        auto_map(mapPtr);

        auto cYaml = stringFromJString(jniEnv, yaml);
        auto cPath = stringFromJString(jniEnv, path);

        auto sceneUpdates = unpackSceneUpdates(jniEnv, updateStrings);
        Url sceneUrl = Url(cPath).resolved("asset:///");
        jint sceneId = map->loadSceneYamlAsync(cYaml, sceneUrl.string(), false, sceneUpdates);

        return sceneId;
    }

    JNIEXPORT void MapController(nativeSetPixelScale)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat scale) {
        auto_map(mapPtr);
        map->setPixelScale(scale);
    }

    JNIEXPORT void MapController(nativeSetCameraType)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jint type) {
        auto_map(mapPtr);
        map->setCameraType(type);
    }

    JNIEXPORT jint MapController(nativeGetCameraType)(JNIEnv* jniEnv, jobject obj, jlong mapPtr) {
        auto_map(mapPtr);
        return map->getCameraType();
    }

    JNIEXPORT jfloat MapController(nativeGetMinZoom)(JNIEnv* jniEnv, jobject obj, jlong mapPtr) {
        auto_map(mapPtr);
        return map->getMinZoom();
    }

    JNIEXPORT void MapController(nativeSetMinZoom)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat minZoom) {
        auto_map(mapPtr);
        map->setMinZoom(minZoom);
    }

    JNIEXPORT jfloat MapController(nativeGetMaxZoom)(JNIEnv* jniEnv, jobject obj, jlong mapPtr) {
        auto_map(mapPtr);

        return map->getMaxZoom();
    }

    JNIEXPORT void MapController(nativeSetMaxZoom)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat maxZoom) {
        auto_map(mapPtr);
        map->setMaxZoom(maxZoom);
    }

    JNIEXPORT void MapController(nativeHandleTapGesture)(JNIEnv* jniEnv, jobject obj, jlong mapPtr,
                                                         jfloat posX, jfloat posY) {
        auto_map(mapPtr);
        map->handleTapGesture(posX, posY);
    }

    JNIEXPORT void MapController(nativeHandleDoubleTapGesture)(JNIEnv* jniEnv, jobject obj, jlong mapPtr,
                                                               jfloat posX, jfloat posY) {
        auto_map(mapPtr);
        map->handleDoubleTapGesture(posX, posY);
    }

    JNIEXPORT void MapController(nativeHandlePanGesture)(JNIEnv* jniEnv, jobject obj, jlong mapPtr,
                                                         jfloat startX, jfloat startY, jfloat endX, jfloat endY) {
        auto_map(mapPtr);
        map->handlePanGesture(startX, startY, endX, endY);
    }

    JNIEXPORT void MapController(nativeHandleFlingGesture)(JNIEnv* jniEnv, jobject obj, jlong mapPtr,
                                                           jfloat posX, jfloat posY, jfloat velocityX, jfloat velocityY) {
        auto_map(mapPtr);
        map->handleFlingGesture(posX, posY, velocityX, velocityY);
    }

    JNIEXPORT void MapController(nativeHandlePinchGesture)(JNIEnv* jniEnv, jobject obj, jlong mapPtr,
                                                           jfloat posX, jfloat posY, jfloat scale, jfloat velocity) {
        auto_map(mapPtr);
        map->handlePinchGesture(posX, posY, scale, velocity);
    }

    JNIEXPORT void MapController(nativeHandleRotateGesture)(JNIEnv* jniEnv, jobject obj, jlong mapPtr,
                                                            jfloat posX, jfloat posY, jfloat rotation) {
        auto_map(mapPtr);
        map->handleRotateGesture(posX, posY, rotation);
    }

    JNIEXPORT void MapController(nativeHandleShoveGesture)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat distance) {
        auto_map(mapPtr);
        map->handleShoveGesture(distance);
    }

    JNIEXPORT void MapController(nativeOnUrlComplete)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong requestHandle,
                                                      jbyteArray fetchedBytes, jstring errorString) {
        auto_map(mapPtr);

        auto platform = static_cast<AndroidPlatform*>(map->getPlatform().get());
        platform->onUrlComplete(jniEnv, requestHandle, fetchedBytes, errorString);
    }

    JNIEXPORT void MapController(nativeSetPickRadius)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat radius) {
        auto_map(mapPtr);
        map->setPickRadius(radius);
    }

    JNIEXPORT void MapController(nativePickFeature)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat posX, jfloat posY) {
        auto_map(mapPtr);
        map->pickFeature(posX, posY);

    }

    JNIEXPORT void MapController(nativePickMarker)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat posX, jfloat posY) {
        auto_map(mapPtr);
        map->pickMarker(posX, posY);
    }

    JNIEXPORT void MapController(nativePickLabel)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jfloat posX, jfloat posY) {
        auto_map(mapPtr);
        map->pickLabel(posX, posY);
    }

    // NOTE unsigned int to jlong for precision... else we can do jint return
    JNIEXPORT jlong MapController(nativeMarkerAdd)(JNIEnv* jniEnv, jobject obj, jlong mapPtr) {
        auto_map(mapPtr);
        auto markerID = map->markerAdd();
        return static_cast<jlong>(markerID);
    }

    JNIEXPORT jboolean MapController(nativeMarkerRemove)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong markerID) {
        auto_map(mapPtr);
        auto result = map->markerRemove(static_cast<unsigned int>(markerID));
        return static_cast<jboolean>(result);
    }

    JNIEXPORT jboolean MapController(nativeMarkerSetStylingFromString)(JNIEnv* jniEnv, jobject obj, jlong mapPtr,
                                                                       jlong markerID, jstring styling) {
        auto_map(mapPtr);

        auto styleString = stringFromJString(jniEnv, styling);
        auto result = map->markerSetStylingFromString(static_cast<unsigned int>(markerID), styleString.c_str());
        return static_cast<jboolean>(result);
    }

    JNIEXPORT jboolean MapController(nativeMarkerSetStylingFromPath)(JNIEnv* jniEnv, jobject obj, jlong mapPtr,
                                                                     jlong markerID, jstring path) {
        auto_map(mapPtr);

        auto pathString = stringFromJString(jniEnv, path);
        auto result = map->markerSetStylingFromPath(static_cast<unsigned int>(markerID), pathString.c_str());
        return static_cast<jboolean>(result);
    }

    JNIEXPORT jboolean MapController(nativeMarkerSetBitmap)(JNIEnv* jniEnv, jobject obj, jlong mapPtr,
                                                            jlong markerID, jobject jbitmap, jfloat density) {
        auto_map(mapPtr);

        AndroidBitmapInfo bitmapInfo;
        if (AndroidBitmap_getInfo(jniEnv, jbitmap, &bitmapInfo) != ANDROID_BITMAP_RESULT_SUCCESS) {
            return static_cast<jboolean>(false);
        }
        if (bitmapInfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
            // TODO: Add different conversion functions for other formats.
            return static_cast<jboolean>(false);
        }
        uint32_t* pixelInput;
        if (AndroidBitmap_lockPixels(jniEnv, jbitmap, (void**)&pixelInput) != ANDROID_BITMAP_RESULT_SUCCESS) {
            return static_cast<jboolean>(false);
        }
        int width = bitmapInfo.width;
        int height = bitmapInfo.height;
        uint32_t* pixelOutput = new uint32_t[height * width];
        int i = 0;
        for (int row = 0; row < height; row++) {
            // Flips image upside-down
            int flippedRow = (height - 1 - row) * width;
            for (int col = 0; col < width; col++) {
                uint32_t pixel = pixelInput[i++];
                // Undo alpha pre-multiplication.
                auto rgba = reinterpret_cast<uint8_t*>(&pixel);
                int a = rgba[3];
                if (a != 0) {
                    auto alphaInv = 255.f/a;
                    rgba[0] = static_cast<uint8_t>(rgba[0] * alphaInv);
                    rgba[1] = static_cast<uint8_t>(rgba[1] * alphaInv);
                    rgba[2] = static_cast<uint8_t>(rgba[2] * alphaInv);
                }
                pixelOutput[flippedRow + col] = pixel;
            }
        }
        AndroidBitmap_unlockPixels(jniEnv, jbitmap);
        auto result = map->markerSetBitmap(static_cast<unsigned int>(markerID), width, height, pixelOutput, density);
        delete[] pixelOutput;
        return static_cast<jboolean>(result);
    }

    JNIEXPORT jboolean MapController(nativeMarkerSetPoint)(JNIEnv* jniEnv, jobject obj, jlong mapPtr,
                                                           jlong markerID, jdouble lng, jdouble lat) {
        auto_map(mapPtr);

        auto result = map->markerSetPoint(static_cast<unsigned int>(markerID), Tangram::LngLat(lng, lat));
        return static_cast<jboolean>(result);
    }

    JNIEXPORT jboolean MapController(nativeMarkerSetPointEased)(JNIEnv* jniEnv, jobject obj, jlong mapPtr,
                                                                jlong markerID, jdouble lng, jdouble lat,
                                                                jfloat duration, jint ease) {
        auto_map(mapPtr);

        auto result = map->markerSetPointEased(static_cast<unsigned int>(markerID),
                Tangram::LngLat(lng, lat), duration, static_cast<Tangram::EaseType>(ease));
        return static_cast<jboolean>(result);
    }

    JNIEXPORT jboolean MapController(nativeMarkerSetPolyline)(JNIEnv* jniEnv, jobject obj, jlong mapPtr,
                                                              jlong markerID, jdoubleArray jcoordinates,
                                                              jint count) {
        auto_map(mapPtr);

        if (!jcoordinates || count == 0) { return false; }

        auto* coordinates = jniEnv->GetDoubleArrayElements(jcoordinates, NULL);
        std::vector<Tangram::LngLat> polyline;
        polyline.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            polyline.emplace_back(coordinates[2 * i], coordinates[2 * i + 1]);
        }

        auto result = map->markerSetPolyline(static_cast<unsigned int>(markerID), polyline.data(), count);
        return static_cast<jboolean>(result);
    }

    JNIEXPORT jboolean MapController(nativeMarkerSetPolygon)(JNIEnv* jniEnv, jobject obj, jlong mapPtr,
                                                             jlong markerID, jdoubleArray jcoordinates,
                                                             jintArray jcounts, jint rings) {
        auto_map(mapPtr);

        if (!jcoordinates || !jcounts || rings == 0) { return false; }

        auto* coordinates = jniEnv->GetDoubleArrayElements(jcoordinates, NULL);

        auto* counts = jniEnv->GetIntArrayElements(jcounts, NULL);

        std::vector<Tangram::LngLat> polygonCoords;

        size_t coordsCount = 0;
        for (size_t i = 0; i < rings; i++) {
            size_t ringCount = *(counts+i);
            for (size_t j = 0; j < ringCount; j++) {
                polygonCoords.emplace_back(coordinates[coordsCount + 2 * j], coordinates[coordsCount + 2 * j + 1]);
            }
            coordsCount += ringCount;
        }

        auto result = map->markerSetPolygon(static_cast<unsigned int>(markerID), polygonCoords.data(), counts, rings);
        return static_cast<jboolean>(result);
    }

    JNIEXPORT jboolean MapController(nativeMarkerSetVisible)(JNIEnv* jniEnv, jobject obj, jlong mapPtr,
                                                             jlong markerID, jboolean visible) {
        auto_map(mapPtr);

        auto result = map->markerSetVisible(static_cast<unsigned int>(markerID), visible);
        return static_cast<jboolean>(result);
    }

    JNIEXPORT jboolean MapController(nativeMarkerSetDrawOrder)(JNIEnv* jniEnv, jobject obj, jlong mapPtr,
                                                               jlong markerID, jint drawOrder) {
        auto_map(mapPtr);

        auto result = map->markerSetDrawOrder(markerID, drawOrder);
        return static_cast<jboolean>(result);
    }

    JNIEXPORT void MapController(nativeMarkerRemoveAll)(JNIEnv* jniEnv, jobject obj, jlong mapPtr) {
        auto_map(mapPtr);

        map->markerRemoveAll();
    }


    JNIEXPORT void MapController(nativeSetDebugFlag)(JNIEnv* jniEnv, jobject obj, jint flag, jboolean on) {
        Tangram::setDebugFlag(static_cast<Tangram::DebugFlags>(flag), on);
    }

    JNIEXPORT void MapController(nativeUseCachedGlState)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jboolean use) {
        auto_map(mapPtr);

        map->useCachedGlState(use);
    }

    JNIEXPORT jint MapController(nativeUpdateScene)(JNIEnv* jniEnv, jobject obj, jlong mapPtr,
                                                    jobjectArray updateStrings) {
        auto_map(mapPtr);

        auto sceneUpdates = unpackSceneUpdates(jniEnv, updateStrings);

        return map->updateSceneAsync(sceneUpdates);
    }

    JNIEXPORT void MapController(nativeOnLowMemory)(JNIEnv* jnienv, jobject obj, jlong mapPtr) {
        auto_map(mapPtr);

        map->onMemoryWarning();
    }

    JNIEXPORT void MapController(nativeSetDefaultBackgroundColor)(JNIEnv* jnienv, jobject obj, jlong mapPtr,
                                                                  jfloat r, jfloat g, jfloat b) {
        auto_map(mapPtr);

        map->setDefaultBackgroundColor(r, g, b);
    }

    JNIEXPORT jlong MapController(nativeAddTileSource)(JNIEnv* jniEnv, jobject obj, jlong mapPtr,
                                                       jstring name, jboolean generateCentroid) {
        auto_map(mapPtr);

        auto sourceName = stringFromJString(jniEnv, name);
        auto source = std::make_shared<Tangram::ClientGeoJsonSource>(map->getPlatform(), sourceName, "", generateCentroid);
        map->addTileSource(source);
        return reinterpret_cast<jlong>(source.get());
    }

    JNIEXPORT void MapController(nativeRemoveTileSource)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong sourcePtr) {
        auto_map(mapPtr);

        assert(sourcePtr > 0);
        auto source = reinterpret_cast<Tangram::TileSource*>(sourcePtr);
        map->removeTileSource(*source);
    }

    JNIEXPORT void MapController(nativeClearTileSource)(JNIEnv* jniEnv, jobject obj, jlong mapPtr, jlong sourcePtr) {
        auto_map(mapPtr);

        assert(sourcePtr > 0);
        auto source = reinterpret_cast<Tangram::TileSource*>(sourcePtr);
        map->clearTileSource(*source, true, true);
    }

#undef MapController


#define MapData(NAME) FUNC(MapData, NAME)

    JNIEXPORT void MapData(nativeAddFeature)(JNIEnv* jniEnv, jobject obj, jlong sourcePtr,
        jdoubleArray jcoordinates, jintArray jrings, jobjectArray jproperties) {

        assert(sourcePtr > 0);
        auto source = reinterpret_cast<Tangram::ClientGeoJsonSource*>(sourcePtr);

        size_t n_points = jniEnv->GetArrayLength(jcoordinates) / 2;
        size_t n_rings = (jrings == NULL) ? 0 : jniEnv->GetArrayLength(jrings);
        size_t n_properties = (jproperties == NULL) ? 0 : jniEnv->GetArrayLength(jproperties) / 2;

        Tangram::Properties properties;

        for (size_t i = 0; i < n_properties; ++i) {
            jstring jkey = (jstring) (jniEnv->GetObjectArrayElement(jproperties, 2 * i));
            jstring jvalue = (jstring) (jniEnv->GetObjectArrayElement(jproperties, 2 * i + 1));
            auto key = stringFromJString(jniEnv, jkey);
            auto value = stringFromJString(jniEnv, jvalue);
            properties.set(key, value);
            jniEnv->DeleteLocalRef(jkey);
            jniEnv->DeleteLocalRef(jvalue);
        }

        auto* coordinates = jniEnv->GetDoubleArrayElements(jcoordinates, NULL);

        if (n_rings > 0) {
            // If rings are defined, this is a polygon feature.
            auto* rings = jniEnv->GetIntArrayElements(jrings, NULL);
            std::vector<std::vector<Tangram::LngLat>> polygon;
            size_t ring_start = 0, ring_end = 0;
            for (size_t i = 0; i < n_rings; ++i) {
                ring_end += rings[i];
                std::vector<Tangram::LngLat> ring;
                for (; ring_start < ring_end; ++ring_start) {
                    ring.push_back({coordinates[2 * ring_start], coordinates[2 * ring_start + 1]});
                }
                polygon.push_back(std::move(ring));
            }
            source->addPoly(properties, polygon);
            jniEnv->ReleaseIntArrayElements(jrings, rings, JNI_ABORT);
        } else if (n_points > 1) {
            // If no rings defined but multiple points, this is a polyline feature.
            std::vector<Tangram::LngLat> polyline;
            for (size_t i = 0; i < n_points; ++i) {
                polyline.push_back({coordinates[2 * i], coordinates[2 * i + 1]});
            }
            source->addLine(properties, polyline);
        } else {
            // This is a point feature.
            auto point = Tangram::LngLat(coordinates[0], coordinates[1]);
            source->addPoint(properties, point);
        }

        jniEnv->ReleaseDoubleArrayElements(jcoordinates, coordinates, JNI_ABORT);

    }

    JNIEXPORT void MapData(nativeAddGeoJson)(JNIEnv* jniEnv, jobject obj,
                                                   jlong sourcePtr, jstring geojson) {
        assert(sourcePtr > 0);
        auto source = reinterpret_cast<Tangram::ClientGeoJsonSource*>(sourcePtr);
        auto data = stringFromJString(jniEnv, geojson);
        source->addData(data);
    }

#undef MapData

}
