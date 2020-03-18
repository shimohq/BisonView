// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_ANDROID_PUBLIC_BROWSER_DRAW_GL_H_
#define BISON_ANDROID_PUBLIC_BROWSER_DRAW_GL_H_

#ifdef __cplusplus
extern "C" {
#endif

// 1 is L/L MR1
//
// 2 starts at M, and added an imperfect workaround for complex clipping by
// elevating the WebView into an FBO layer. If any transform, clip, or outline
// clip occurs that would either likely use the stencil buffer for clipping, or
// require shader based clipping in HWUI, the WebView is drawn into an FBO (if
// it fits).
// This is a temporary workaround for a lack of WebView support for stencil/
// shader based round rect clipping, and should be removed when webview is
// capable of supporting these clips internally when drawing.
//
// 3 starts during development of P, when android defaults from HWUI to skia as
// the GL renderer. Skia already maintains and restores its GL state, so there
// is no need for WebView to restore this state. Skia also no longer promises
// GL state on entering draw, such as no vertex array buffer binding.
static const int kBisonDrawGLInfoVersion = 3;

// Holds the information required to trigger an OpenGL drawing operation.
struct BisonDrawGLInfo {
  int version;  // The BisonDrawGLInfo this struct was built with.

  // Input: tells the draw function what action to perform.
  enum Mode {
    kModeDraw = 0,
    kModeProcess = 1,
    kModeProcessNoContext = 2,
    kModeSync = 3,
  } mode;

  // Input: current clip rect in surface coordinates. Reflects the current state
  // of the OpenGL scissor rect. Both the OpenGL scissor rect and viewport are
  // set by the caller of the draw function and updated during View animations.
  int clip_left;
  int clip_top;
  int clip_right;
  int clip_bottom;

  // Input: current width/height of destination surface.
  int width;
  int height;

  // Input: is the View rendered into an independent layer.
  // If false, the surface is likely to hold to the full screen contents, with
  // the scissor box set by the caller to the actual View location and size.
  // Also the transformation matrix will contain at least a translation to the
  // position of the View to render, plus any other transformations required as
  // part of any ongoing View animation. View translucency (alpha) is ignored,
  // although the framework will set is_layer to true for non-opaque cases.
  // Can be requested via the View.setLayerType(View.LAYER_TYPE_NONE, ...)
  // Android API method.
  //
  // If true, the surface is dedicated to the View and should have its size.
  // The viewport and scissor box are set by the caller to the whole surface.
  // Animation transformations are handled by the caller and not reflected in
  // the provided transformation matrix. Translucency works normally.
  // Can be requested via the View.setLayerType(View.LAYER_TYPE_HARDWARE, ...)
  // Android API method.
  bool is_layer;

  // Input: current transformation matrix in surface pixels.
  // Uses the column-based OpenGL matrix format.
  float transform[16];
};

// Function to invoke a direct GL draw into the client's pre-configured
// GL context. Obtained via BisonContents.getDrawGLFunction() (static).
// |view_context| is an opaque identifier that was returned by the corresponding
// call to BisonContents.getBisonDrawGLViewContext().
// |draw_info| carries the in and out parameters for this draw.
// |spare| ignored; pass NULL.
typedef void (BisonDrawGLFunction)(long view_context,
                                BisonDrawGLInfo* draw_info,
                                void* spare);
enum BisonMapMode {
  MAP_READ_ONLY = 0,
  MAP_WRITE_ONLY = 1,
  MAP_READ_WRITE = 2,
};

// Called to create a GraphicBuffer
typedef long BisonCreateGraphicBufferFunction(int w, int h);
// Called to release a GraphicBuffer
typedef void BisonReleaseGraphicBufferFunction(long buffer_id);
// Called to map a GraphicBuffer in |mode|.
typedef int BisonMapFunction(long buffer_id, BisonMapMode mode, void** vaddr);
// Called to unmap a GraphicBuffer
typedef int BisonUnmapFunction(long buffer_id);
// Called to get a native buffer pointer
typedef void* BisonGetNativeBufferFunction(long buffer_id);
// Called to get the stride of the buffer
typedef unsigned int BisonGetStrideFunction(long buffer_id);

static const int kBisonDrawGLFunctionTableVersion = 1;

// Set of functions used in rendering in hardware mode
struct BisonDrawGLFunctionTable {
  int version;
  BisonCreateGraphicBufferFunction* create_graphic_buffer;
  BisonReleaseGraphicBufferFunction* release_graphic_buffer;
  BisonMapFunction* map;
  BisonUnmapFunction* unmap;
  BisonGetNativeBufferFunction* get_native_buffer;
  BisonGetStrideFunction* get_stride;
};

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // BISON_ANDROID_PUBLIC_BROWSER_DRAW_GL_H_
