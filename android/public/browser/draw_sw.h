// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_ANDROID_PUBLIC_BROWSER_DRAW_SW_H_
#define BISON_ANDROID_PUBLIC_BROWSER_DRAW_SW_H_

#include <jni.h>
#include <stddef.h>

#ifndef __cplusplus
#error "Can't mix C and C++ when using jni.h"
#endif

class SkCanvasState;
class SkPicture;

static const int kBisonPixelInfoVersion = 3;

// Holds the information required to implement the SW draw to system canvas.
struct BisonPixelInfo {
  int version;          // The kBisonPixelInfoVersion this struct was built with.
  SkCanvasState* state; // The externalize state in skia format.
  // NOTE: If you add more members, bump kBisonPixelInfoVersion.
};

// Function that can be called to fish out the underlying native pixel data
// from a Java canvas object, for optimized rendering path.
// Returns the pixel info on success, which must be freed via a call to
// BisonReleasePixelsFunction, or NULL.
typedef BisonPixelInfo* (BisonAccessPixelsFunction)(JNIEnv* env, jobject canvas);

// Must be called to balance every *successful* call to BisonAccessPixelsFunction
// (i.e. that returned true).
typedef void (BisonReleasePixelsFunction)(BisonPixelInfo* pixels);

// Called to create an Android Picture object encapsulating a native SkPicture.
typedef jobject (BisonCreatePictureFunction)(JNIEnv* env, SkPicture* picture);

// Method that returns the current Skia function.
typedef void (SkiaVersionFunction)(int* major, int* minor, int* patch);

// Called to verify if the Skia versions are compatible.
typedef bool (BisonIsSkiaVersionCompatibleFunction)(SkiaVersionFunction function);

static const int kBisonDrawSWFunctionTableVersion = 1;

// "vtable" for the functions declared in this file. An instance must be set via
// BisonContents.setBisonDrawSWFunctionTable
struct BisonDrawSWFunctionTable {
  int version;
  BisonAccessPixelsFunction* access_pixels;
  BisonReleasePixelsFunction* release_pixels;
};

#endif  // BISON_ANDROID_PUBLIC_BROWSER_DRAW_SW_H_
