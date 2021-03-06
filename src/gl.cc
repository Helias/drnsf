//
// DRNSF - An unofficial Crash Bandicoot level editor
// Copyright (C) 2017-2018  DRNSF contributors
//
// See the AUTHORS.md file for more details.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "common.hh"
#define DRNSF_FRONTEND_IMPLEMENTATION
#include "gl.hh"
#include "gui.hh"

namespace drnsf {
namespace gl {

#if USE_X11
// declared in gl.hh
Window g_wnd;

// declared in gl.hh
GLXContext g_ctx;

// declared in gl.hh
XVisualInfo *g_vi;

// (s-var) s_cmap
// Colormap for the GLX background window.
static Colormap s_cmap;
#elif USE_WINAPI
// declared in gl.hh
HWND g_hwnd;

// declared in gl.hh
HDC g_hdc;

// declared in gl.hh
HGLRC g_hglrc;

// declared in gl.hh
PIXELFORMATDESCRIPTOR g_pfd;

// declared in gl.hh
int g_pfid;
#endif

// declared in gl.hh
void init()
{
#if USE_X11
    using gui::g_display;

    int glx_attrs[] = {
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE,   GLX_RGBA_BIT,
        GLX_DOUBLEBUFFER,  True,
        None
    };
    int fbc_count;
    GLXFBConfig *fbcs = glXChooseFBConfig(
        g_display,
        DefaultScreen(g_display),
        glx_attrs,
        &fbc_count
    );
    DRNSF_ON_EXIT { XFree(fbcs); };

    if (fbc_count <= 0) {
        throw std::runtime_error("gl::init: failed to choose GLX FB config");
    }

    auto fbc = fbcs[0];

    g_vi = glXGetVisualFromFBConfig(g_display, fbc);
    if (!g_vi) {
        throw std::runtime_error("gl::init: failed to choose X visual");
    }
    // FIXME on error release g_glx_vi

    s_cmap = XCreateColormap(
        g_display,
        DefaultRootWindow(g_display),
        g_vi->visual,
        AllocNone
    );

    XSetWindowAttributes x_attrs{};
    x_attrs.colormap = s_cmap;
    g_wnd = XCreateWindow(
        g_display,
        DefaultRootWindow(g_display),
        0, 0,
        1, 1,
        0,
        g_vi->depth,
        InputOutput,
        g_vi->visual,
        CWColormap,
        &x_attrs
    );

    g_ctx = glXCreateContext(g_display, g_vi, nullptr, true);
    if (!g_ctx) {
        // NOTE: glXCreateContext may also fail on the X server side, which may
        // not return NULL.

        throw std::runtime_error("gl::init: failed to create basic context");
    }
    // FIXME release context on error

    if (!glXMakeCurrent(g_display, g_wnd, g_ctx)) {
        throw std::runtime_error("gl::init: failed to activate basic context");
    }

    auto glXCreateContextAttribs =
        reinterpret_cast<PFNGLXCREATECONTEXTATTRIBSARBPROC>(glXGetProcAddress(
            reinterpret_cast<const unsigned char *>(
                &"glXCreateContextAttribsARB"[0]
            )
        ));

    if (!glXCreateContextAttribs) {
        throw std::runtime_error(
            "gl::init: failed to find glXCreateContextAttribsARB"
        );
    }

    int core_attribs[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
        GLX_CONTEXT_MINOR_VERSION_ARB, 2,
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
        0
    };
    auto core_ctx = glXCreateContextAttribs(g_display, fbc, 0, true /* ??? */, core_attribs); //FIXME
    if (core_ctx) {
        glXMakeCurrent(g_display, None, nullptr);
        glXDestroyContext(g_display, g_ctx);
        g_ctx = core_ctx;

        if (!glXMakeCurrent(g_display, g_wnd, g_ctx)) {
            throw std::runtime_error(
                "gl::init: failed to activate core context"
            );
        }

        if (epoxy_gl_version() < 32) {
            throw std::runtime_error(
                "gl::init: core context version less than 3.2"
            );
        }

        if (!epoxy_has_gl_extension("GL_ARB_shader_bit_encoding")) {
            throw std::runtime_error(
                "gl::init: missing feature ARB_shader_bit_encoding"
            );
        }

        return;
    }

    int fwd_attribs[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
        GLX_CONTEXT_MINOR_VERSION_ARB, 1,
        GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
        0
    };
    auto fwd_ctx = glXCreateContextAttribs(g_display, fbc, 0, true /* ??? */, fwd_attribs); //FIXME
    if (fwd_ctx) {
        glXMakeCurrent(g_display, None, nullptr);
        glXDestroyContext(g_display, g_ctx);
        g_ctx = fwd_ctx;

        if (!glXMakeCurrent(g_display, g_wnd, g_ctx)) {
            throw std::runtime_error(
                "gl::init: failed to activate forward context"
            );
        }

        if (epoxy_gl_version() < 31) {
            throw std::runtime_error(
                "gl::init: forward context version less than 3.1"
            );
        }

        if (!epoxy_has_gl_extension("GL_ARB_shader_bit_encoding")) {
            throw std::runtime_error(
                "gl::init: missing feature ARB_shader_bit_encoding"
            );
        }

        return;
    }
    throw std::runtime_error(
        "gl::init failed to create a 3.1 forward or 3.2 core context"
    );
#elif USE_WINAPI
    static std::once_flag wndclass_flag;
    std::call_once(wndclass_flag, [] {
        WNDCLASSW wndclass{};
        wndclass.lpfnWndProc = DefWindowProcW;
        wndclass.hInstance = GetModuleHandleW(nullptr);
        wndclass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wndclass.hbrBackground = HBRUSH(COLOR_3DFACE + 1);
        wndclass.lpszClassName = L"DRNSF_GL";
        if (!RegisterClassW(&wndclass)) {
            throw std::runtime_error("gl::init: failed to register window class");
        }
    });

    g_hwnd = CreateWindowW(
        L"DRNSF_GL",
        nullptr,
        0,
        0, 0,
        0, 0,
        nullptr,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr
    );
    if (!g_hwnd) {
        throw std::runtime_error("gl::init: failed to create window");
    }
    // FIXME - close window on error

    g_hdc = GetDC(g_hwnd);
    if (!g_hdc) {
        throw std::runtime_error("gl::init: failed to get window hdc");
    }
    // FIXME - release dc on error

    // Find an appropriate pixel format for use with OpenGL.
    g_pfd = {};
    g_pfd.nSize = sizeof(g_pfd);
    g_pfd.nVersion = 1;
    g_pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    g_pfd.iPixelType = PFD_TYPE_RGBA;
    g_pfd.iLayerType = PFD_MAIN_PLANE;
    g_pfid = ChoosePixelFormat(g_hdc, &g_pfd);
    if (!g_pfid) {
        throw std::runtime_error("gl::init: cannot find compatible pixel format");
    }

    // Set the window to use the discovered pixel format.
    if (!SetPixelFormat(g_hdc, g_pfid, &g_pfd)) {
        throw std::runtime_error("gl::init: could not set pixel format");
    }

    // Create the initial GL context. We will use this to create the actual GL
    // context (if possible).
    g_hglrc = wglCreateContext(g_hdc);
    if (!g_hglrc) {
        throw std::runtime_error("gl::init: could not create OpenGL context");
    }
    // FIXME - destroy context on error

    // Activate the context.
    if (!wglMakeCurrent(g_hdc, g_hglrc)) {
        throw std::runtime_error("gl::init: could not use OpenGL context");
    }

    // Get the function for creating newer GL contexts, if available.
    auto wglCreateContextAttribs =
        reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(wglGetProcAddress(
            "wglCreateContextAttribsARB"
        ));
    if (!wglCreateContextAttribs) {
        throw std::runtime_error(
            "gl::init: could not find wglCreateContextAttribsARB"
        );
    }

    int core_attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 2,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
        0
    };
    auto core_ctx = wglCreateContextAttribs(g_hdc, false, core_attribs);
    if (core_ctx) {
        // Switch to the core context.
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(g_hglrc);
        g_hglrc = core_ctx;
        if (!wglMakeCurrent(g_hdc, g_hglrc)) {
            throw std::runtime_error(
                "gl::init: failed to activate core context"
            );
        }

        if (epoxy_gl_version() < 32) {
            throw std::runtime_error(
                "gl::init: core context version less than 3.2"
            );
        }

        if (!epoxy_has_gl_extension("GL_ARB_shader_bit_encoding")) {
            throw std::runtime_error(
                "gl::init: missing feature ARB_shader_bit_encoding"
            );
        }

        return;
    }

    int fwd_attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 1,
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
        0
    };
    auto fwd_ctx = wglCreateContextAttribs(g_hdc, false, fwd_attribs);
    if (fwd_ctx) {
        // Switch to the forward context.
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(g_hglrc);
        g_hglrc = fwd_ctx;
        if (!wglMakeCurrent(g_hdc, g_hglrc)) {
            throw std::runtime_error(
                "gl::init: failed to activate forward context"
            );
        }

        if (epoxy_gl_version() < 31) {
            throw std::runtime_error(
                "gl::init: forward context version less than 3.1"
            );
        }

        if (!epoxy_has_gl_extension("GL_ARB_shader_bit_encoding")) {
            throw std::runtime_error(
                "gl::init: missing feature ARB_shader_bit_encoding"
            );
        }

        return;
    }

    throw std::runtime_error(
        "gl::init: failed to create a 3.1 forward or 3.2 core context"
    );
#else
#error Unimplemented UI frontend code.
#endif
}

// declared in gl.hh
void shutdown()
{
    any_object::reset_all();

#if USE_X11
    using gui::g_display;
    glXMakeCurrent(g_display, None, nullptr);
    glXDestroyContext(g_display, g_ctx);
    XDestroyWindow(g_display, g_wnd);
#elif USE_WINAPI
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(g_hglrc);
    ReleaseDC(g_hwnd, g_hdc);
    DestroyWindow(g_hwnd);
#else
#error Unimplemented UI frontend code.
#endif
}

// declared in gl.hh
std::unordered_set<any_object *> &any_object::get_all_objects()
{
    static std::unordered_set<any_object *> s_all_objects;
    return s_all_objects;
}

// declared in gl.hh
void any_object::reset_all()
{
    for (auto &&p : get_all_objects()) {
        p->reset();
    }
}

}
}
