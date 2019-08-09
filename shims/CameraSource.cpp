/*
 * Copyright (C) 2018 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "libstagefright_shim"
#include <utils/Log.h>
#include <utils/String8.h>
#include <OMX_Component.h>
#include <camera/Camera.h>
#include <camera/CameraParameters.h>
#include <media/stagefright/CameraSource.h>

namespace android {

static const char PIXEL_FORMAT_YUV420SP_NV21[] = "nv21";

static void getSupportedVideoSizes(
    const CameraParameters& params,
    bool *isSetVideoSizeSupported,
    Vector<Size>& sizes) {
    *isSetVideoSizeSupported = true;
    params.getSupportedVideoSizes(sizes);
    if (sizes.size() == 0) {
        ALOGD("Camera does not support setVideoSize()");
        params.getSupportedPreviewSizes(sizes);
        *isSetVideoSizeSupported = false;
    }
}

status_t CameraSource::configureCamera(
        CameraParameters* params,
        int32_t width, int32_t height,
        int32_t frameRate) {
    ALOGW("SHIM: hijacking %s!", __func__);
    ALOGV("configureCamera");
    Vector<Size> sizes;
    bool isSetVideoSizeSupportedByCamera = true;
    getSupportedVideoSizes(*params, &isSetVideoSizeSupportedByCamera, sizes);
    bool isCameraParamChanged = false;
    if (width != -1 && height != -1) {
        //this fixes an issue with Samsung libs where they don't expose all available resolutions
        /*if (!isVideoSizeSupported(width, height, sizes)) {
            ALOGE("Video dimension (%dx%d) is unsupported", width, height);
            return BAD_VALUE;
        }*/
        if (isSetVideoSizeSupportedByCamera) {
            params->setVideoSize(width, height);
        } else {
            params->setPreviewSize(width, height);
        }
        isCameraParamChanged = true;
    } else if ((width == -1 && height != -1) ||
               (width != -1 && height == -1)) {
        // If one and only one of the width and height is -1
        // we reject such a request.
        ALOGE("Requested video size (%dx%d) is not supported", width, height);
        return BAD_VALUE;
    } else {  // width == -1 && height == -1
        // Do not configure the camera.
        // Use the current width and height value setting from the camera.
    }
    if (frameRate != -1) {
	//changed max to 240
        if(frameRate <= 0 && frameRate > 240){
		return -1;
	}
	//commenting out because the camera driver can handle any fps range as long as it's below 240
        /*const char* supportedFrameRates =
                params->get(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES);
        CHECK(supportedFrameRates != NULL);
        ALOGV("Supported frame rates: %s", supportedFrameRates);
        char buf[4];
        snprintf(buf, 4, "%d", frameRate);
        if (strstr(supportedFrameRates, buf) == NULL) {
            ALOGE("Requested frame rate (%d) is not supported: %s",
                frameRate, supportedFrameRates);
            return BAD_VALUE;
        }*/
        // The frame rate is supported, set the camera to the requested value.
        params->setPreviewFrameRate(frameRate);
        isCameraParamChanged = true;
    } else {  // frameRate == -1
        // Do not configure the camera.
        // Use the current frame rate value setting from the camera
    }
    if (isCameraParamChanged) {
        // Either frame rate or frame size needs to be changed.
        String8 s = params->flatten();
        if (OK != mCamera->setParameters(s)) {
            ALOGE("Could not change settings."
                 " Someone else is using camera %p?", mCamera.get());
            return -EBUSY;
        }
    }
    return OK;
}
static int32_t getColorFormat(const char* colorFormat) {
    if (!colorFormat) {
        ALOGE("Invalid color format");
        return -1;
    }

    if (!strcmp(colorFormat, CameraParameters::PIXEL_FORMAT_YUV420P)) {
       return OMX_COLOR_FormatYUV420Planar;
    }

    if (!strcmp(colorFormat, CameraParameters::PIXEL_FORMAT_YUV422SP)) {
       return OMX_COLOR_FormatYUV422SemiPlanar;
    }

    if (!strcmp(colorFormat, CameraParameters::PIXEL_FORMAT_YUV420SP)) {
        return OMX_COLOR_FormatYUV420SemiPlanar;
    }

    if (!strcmp(colorFormat, PIXEL_FORMAT_YUV420SP_NV21)) {
        static const int OMX_SEC_COLOR_FormatNV21Linear = 0x7F000011;
        return OMX_SEC_COLOR_FormatNV21Linear;
    }

    if (!strcmp(colorFormat, CameraParameters::PIXEL_FORMAT_YUV422I)) {
        return OMX_COLOR_FormatYCbYCr;
    }

    if (!strcmp(colorFormat, CameraParameters::PIXEL_FORMAT_RGB565)) {
       return OMX_COLOR_Format16bitRGB565;
    }

    if (!strcmp(colorFormat, "OMX_TI_COLOR_FormatYUV420PackedSemiPlanar")) {
       return OMX_TI_COLOR_FormatYUV420PackedSemiPlanar;
    }

    if (!strcmp(colorFormat, CameraParameters::PIXEL_FORMAT_ANDROID_OPAQUE)) {
        return OMX_COLOR_FormatAndroidOpaque;
    }

    if (!strcmp(colorFormat, "YVU420SemiPlanar")) {
        return OMX_QCOM_COLOR_FormatYVU420SemiPlanar;
    }

    ALOGE("Uknown color format (%s), please add it to "
         "CameraSource::getColorFormat", colorFormat);

    //CHECK(!"Unknown color format");
    return -1;
}


/*
 * Check whether the camera has the supported color format
 * @param params CameraParameters to retrieve the information
 * @return OK if no error.
 */
status_t CameraSource::isCameraColorFormatSupported(
        const CameraParameters& params) {
    ALOGW("SHIM: hijacking %s!", __func__);

    mColorFormat = getColorFormat(params.get(
            CameraParameters::KEY_VIDEO_FRAME_FORMAT));
    if (mColorFormat == -1) {
        return BAD_VALUE;
    }
    return OK;
}

} // namespace android
