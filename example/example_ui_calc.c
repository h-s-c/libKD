/* nuklear - v1.05 - public domain */
#include <KD/kd.h>
#include <KD/kdext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#if defined(GL_OES_mapbuffer)
PFNGLMAPBUFFEROESPROC glMapBuffer;
PFNGLUNMAPBUFFEROESPROC glUnmapBuffer;
#endif
#if defined(GL_OES_vertex_array_object)
PFNGLGENVERTEXARRAYSOESPROC glGenVertexArrays;
PFNGLBINDVERTEXARRAYOESPROC glBindVertexArray;
#endif
#if defined(GL_KHR_debug)
PFNGLDEBUGMESSAGECALLBACKKHRPROC glDebugMessageCallback;
#endif

#define NK_PRIVATE
#define NK_ASSERT kdAssert
/* TODO: Get rid of standard malloc usage. */
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#include "nuklear.h"

static KDboolean quit = KD_FALSE;

/* macros */
#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

#define UNUSED(a) (void)a
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define LEN(a) (sizeof(a) / sizeof(a)[0])
#define OFFSETOFF(st, m) ((KDsize) & (((st *)0)->m))

/* ===============================================================
 *
 *                          DEVICE
 *
 * ===============================================================*/
struct nk_vertex {
    KDfloat32 position[2];
    KDfloat32 uv[2];
    nk_byte col[4];
};

struct device {
    struct nk_buffer cmds;
    struct nk_draw_null_texture null;
    GLuint vbo, vao, ebo;
    GLuint prog;
    GLuint vert_shdr;
    GLuint frag_shdr;
    GLint attrib_pos;
    GLint attrib_uv;
    GLint attrib_col;
    GLint uniform_tex;
    GLint uniform_proj;
    GLuint font_tex;
};

static void
device_init(struct device *dev)
{
    GLint status;
    static const GLchar *vertex_shader =
        "uniform mat4 ProjMtx;\n"
        "attribute vec2 Position;\n"
        "attribute vec2 TexCoord;\n"
        "attribute vec4 Color;\n"
        "varying vec2 Frag_UV;\n"
        "varying vec4 Frag_Color;\n"
        "void main() {\n"
        "   Frag_UV = TexCoord;\n"
        "   Frag_Color = Color;\n"
        "   gl_Position = ProjMtx * vec4(Position.xy, 0, 1);\n"
        "}\n";
    static const GLchar *fragment_shader =
        "precision mediump float;\n"
        "uniform sampler2D Texture;\n"
        "varying vec2 Frag_UV;\n"
        "varying vec4 Frag_Color;\n"
        "void main(){\n"
        "   gl_FragColor = Frag_Color * texture2D(Texture, Frag_UV);\n"
        "}\n";

    nk_buffer_init_default(&dev->cmds);
    dev->prog = glCreateProgram();
    dev->vert_shdr = glCreateShader(GL_VERTEX_SHADER);
    dev->frag_shdr = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(dev->vert_shdr, 1, &vertex_shader, 0);
    glShaderSource(dev->frag_shdr, 1, &fragment_shader, 0);
    glCompileShader(dev->vert_shdr);
    glCompileShader(dev->frag_shdr);
    glGetShaderiv(dev->vert_shdr, GL_COMPILE_STATUS, &status);
    kdAssert(status == GL_TRUE);
    glGetShaderiv(dev->frag_shdr, GL_COMPILE_STATUS, &status);
    kdAssert(status == GL_TRUE);
    glAttachShader(dev->prog, dev->vert_shdr);
    glAttachShader(dev->prog, dev->frag_shdr);
    glLinkProgram(dev->prog);
    glGetProgramiv(dev->prog, GL_LINK_STATUS, &status);
    kdAssert(status == GL_TRUE);

    dev->uniform_tex = glGetUniformLocation(dev->prog, "Texture");
    dev->uniform_proj = glGetUniformLocation(dev->prog, "ProjMtx");
    dev->attrib_pos = glGetAttribLocation(dev->prog, "Position");
    dev->attrib_uv = glGetAttribLocation(dev->prog, "TexCoord");
    dev->attrib_col = glGetAttribLocation(dev->prog, "Color");

    {
        /* buffer setup */
        GLsizei vs = sizeof(struct nk_vertex);
        KDsize vp = OFFSETOFF(struct nk_vertex, position);
        KDsize vt = OFFSETOFF(struct nk_vertex, uv);
        KDsize vc = OFFSETOFF(struct nk_vertex, col);

        glGenBuffers(1, &dev->vbo);
        glGenBuffers(1, &dev->ebo);
        glGenVertexArrays(1, &dev->vao);

        glBindVertexArray(dev->vao);
        glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

        glEnableVertexAttribArray((GLuint)dev->attrib_pos);
        glEnableVertexAttribArray((GLuint)dev->attrib_uv);
        glEnableVertexAttribArray((GLuint)dev->attrib_col);

        glVertexAttribPointer((GLuint)dev->attrib_pos, 2, GL_FLOAT, GL_FALSE, vs, (void *)vp);
        glVertexAttribPointer((GLuint)dev->attrib_uv, 2, GL_FLOAT, GL_FALSE, vs, (void *)vt);
        glVertexAttribPointer((GLuint)dev->attrib_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, vs, (void *)vc);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
#ifndef __EMSCRIPTEN__
    glBindVertexArray(0);
#endif
}

static void
device_upload_atlas(struct device *dev, const void *image, KDint width, KDint height)
{
    glGenTextures(1, &dev->font_tex);
    glBindTexture(GL_TEXTURE_2D, dev->font_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
}

static void
device_shutdown(struct device *dev)
{
    glDetachShader(dev->prog, dev->vert_shdr);
    glDetachShader(dev->prog, dev->frag_shdr);
    glDeleteShader(dev->vert_shdr);
    glDeleteShader(dev->frag_shdr);
    glDeleteProgram(dev->prog);
    glDeleteTextures(1, &dev->font_tex);
    glDeleteBuffers(1, &dev->vbo);
    glDeleteBuffers(1, &dev->ebo);
    nk_buffer_free(&dev->cmds);
}

static void
device_draw(struct device *dev, struct nk_context *ctx, KDint width, KDint height,
    enum nk_anti_aliasing AA)
{
    GLfloat ortho[4][4] = {
        {2.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, -2.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f, 1.0f},
    };
    ortho[0][0] /= (GLfloat)width;
    ortho[1][1] /= (GLfloat)height;

    /* setup global state */
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);

    /* setup program */
    glUseProgram(dev->prog);
    glUniform1i(dev->uniform_tex, 0);
    glUniformMatrix4fv(dev->uniform_proj, 1, GL_FALSE, &ortho[0][0]);
    {
        /* convert from command queue into draw list and draw to screen */
        const struct nk_draw_command *cmd;
        void *vertices, *elements;
        const nk_draw_index *offset = NULL;

        /* allocate vertex and element buffer */
        glBindVertexArray(dev->vao);
        glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

        glBufferData(GL_ARRAY_BUFFER, MAX_VERTEX_MEMORY, KD_NULL, GL_STREAM_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_ELEMENT_MEMORY, KD_NULL, GL_STREAM_DRAW);

/* load draw vertices & elements directly into vertex + element buffer */
#ifdef __EMSCRIPTEN__
        vertices = kdMalloc(MAX_VERTEX_MEMORY);
        elements = kdMalloc(MAX_ELEMENT_MEMORY);
#else
        vertices = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY_OES);
        elements = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY_OES);
#endif
        {
            /* fill convert configuration */
            struct nk_convert_config config;
            static const struct nk_draw_vertex_layout_element vertex_layout[] = {
                {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_vertex, position)},
                {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_vertex, uv)},
                {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_vertex, col)},
                {NK_VERTEX_LAYOUT_END}};
            NK_MEMSET(&config, 0, sizeof(config));
            config.vertex_layout = vertex_layout;
            config.vertex_size = sizeof(struct nk_vertex);
            config.vertex_alignment = NK_ALIGNOF(struct nk_vertex);
            config.null = dev->null;
            config.circle_segment_count = 22;
            config.curve_segment_count = 22;
            config.arc_segment_count = 22;
            config.global_alpha = 1.0f;
            config.shape_AA = AA;
            config.line_AA = AA;

            /* setup buffers to load vertices and elements */
            {
                struct nk_buffer vbuf, ebuf;
                nk_buffer_init_fixed(&vbuf, vertices, MAX_VERTEX_MEMORY);
                nk_buffer_init_fixed(&ebuf, elements, MAX_ELEMENT_MEMORY);
                nk_convert(ctx, &dev->cmds, &vbuf, &ebuf, &config);
            }
        }
#ifdef __EMSCRIPTEN__
        glBufferSubData(GL_ARRAY_BUFFER, 0, (KDsize)MAX_VERTEX_MEMORY, vertices);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, (KDsize)MAX_ELEMENT_MEMORY, elements);
        kdFree(vertices);
        kdFree(elements);
#else
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
#endif

        /* iterate over and execute each draw command */
        nk_draw_foreach(cmd, ctx, &dev->cmds)
        {
            if(!cmd->elem_count)
                continue;
            glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
            glScissor(
                (GLint)(cmd->clip_rect.x),
                (GLint)((height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h))),
                (GLint)(cmd->clip_rect.w),
                (GLint)(cmd->clip_rect.h));
            glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
            offset += cmd->elem_count;
        }
        nk_clear(ctx);
    }

    /* default OpenGL state */
    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
#ifndef __EMSCRIPTEN__
    glBindVertexArray(0);
#endif
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
}

static void
pump_input(struct nk_context *ctx, KDWindow *win)
{
    nk_input_begin(ctx);

    const KDEvent *event = kdWaitEvent(-1);
    while(event)
    {
        switch(event->type)
        {
            case(KD_EVENT_QUIT):
            case(KD_EVENT_WINDOW_CLOSE):
            {
                quit = KD_TRUE;
                break;
            }
            case(KD_EVENT_INPUT_POINTER):
            {
                if(event->data.inputpointer.index == KD_INPUT_POINTER_SELECT)
                {
                    nk_input_button(ctx, NK_BUTTON_LEFT, event->data.inputpointer.x, event->data.inputpointer.y, event->data.inputpointer.select);
                    break;
                }
                else if(event->data.inputpointer.index == KD_INPUT_POINTER_X || event->data.inputpointer.index == KD_INPUT_POINTER_Y)
                {
                    nk_input_motion(ctx, event->data.inputpointer.x, event->data.inputpointer.y);
                    break;
                }
            }
            case(KD_EVENT_INPUT_KEY_ATX):
            {
                KDEventInputKeyATX *keyevent = (KDEventInputKeyATX *)(&event->data);
                switch(keyevent->keycode)
                {
                    case(KD_KEY_ENTER_ATX):
                    {
                        nk_input_key(ctx, NK_KEY_ENTER, keyevent->flags & KD_KEY_PRESS_ATX);
                        break;
                    }
                    case(KD_KEY_LEFT_ATX):
                    {
                        nk_input_key(ctx, NK_KEY_LEFT, keyevent->flags & KD_KEY_PRESS_ATX);
                        break;
                    }
                    case(KD_KEY_RIGHT_ATX):
                    {
                        nk_input_key(ctx, NK_KEY_RIGHT, keyevent->flags & KD_KEY_PRESS_ATX);
                        break;
                    }
                    case(KD_KEY_UP_ATX):
                    {
                        nk_input_key(ctx, NK_KEY_UP, keyevent->flags & KD_KEY_PRESS_ATX);
                        break;
                    }
                    case(KD_KEY_DOWN_ATX):
                    {
                        nk_input_key(ctx, NK_KEY_DOWN, keyevent->flags & KD_KEY_PRESS_ATX);
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
                break;
            }
            case(KD_EVENT_INPUT_KEYCHAR_ATX):
            {
                KDEventInputKeyCharATX *keycharevent = (KDEventInputKeyCharATX *)(&event->data);
                nk_input_char(ctx, keycharevent->character);
                break;
            }
            default:
            {
                kdDefaultEvent(event);
            }
        }
        event = kdWaitEvent(-1);
    }

    nk_input_end(ctx);
}

#if defined(GL_KHR_debug)
static void GL_APIENTRY gl_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    kdLogMessage(message);
}
#endif

KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
    /* Platform */
    KDint width = 0, height = 0;

    /* GUI */
    struct device device;
    struct nk_font_atlas atlas;
    struct nk_context ctx;

    /* KD */
    const EGLint egl_attributes[] =
        {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, EGL_DONT_CARE,
            EGL_DEPTH_SIZE, EGL_DONT_CARE,
            EGL_STENCIL_SIZE, EGL_DONT_CARE,
            EGL_SAMPLE_BUFFERS, 0,
            EGL_NONE};

    const EGLint egl_context_attributes[] =
    {
        EGL_CONTEXT_CLIENT_VERSION,
        2,
#if defined(EGL_KHR_create_context)
        EGL_CONTEXT_FLAGS_KHR,
        EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR,
#endif
        EGL_NONE,
    };

    EGLDisplay egl_display = eglGetDisplay(kdGetDisplayVEN());

    eglInitialize(egl_display, 0, 0);
    eglBindAPI(EGL_OPENGL_ES_API);

    EGLint egl_num_configs = 0;
    EGLConfig egl_config;
    eglChooseConfig(egl_display, egl_attributes, &egl_config, 1, &egl_num_configs);
    EGLContext egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, egl_context_attributes);

    KDWindow *kd_window = kdCreateWindow(egl_display, egl_config, KD_NULL);
    EGLNativeWindowType native_window;
    kdRealizeWindow(kd_window, &native_window);

    EGLSurface egl_surface = eglCreateWindowSurface(egl_display, egl_config, native_window, KD_NULL);

    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);

    if(eglGetError() != EGL_SUCCESS)
    {
        kdAssert(0);
    }

#if defined(GL_OES_mapbuffer)
    if(kdStrstrVEN((const KDchar *)glGetString(GL_EXTENSIONS), "GL_OES_mapbuffer"))
    {
        glMapBuffer = (PFNGLMAPBUFFEROESPROC)eglGetProcAddress("glMapBufferOES");
        glUnmapBuffer = (PFNGLUNMAPBUFFEROESPROC)eglGetProcAddress("glUnmapBufferOES");
    }
#endif
#if defined(GL_OES_vertex_array_object)
    if(kdStrstrVEN((const KDchar *)glGetString(GL_EXTENSIONS), "GL_OES_vertex_array_object"))
    {
        glGenVertexArrays = (PFNGLGENVERTEXARRAYSOESPROC)eglGetProcAddress("glGenVertexArraysOES");
        glBindVertexArray = (PFNGLBINDVERTEXARRAYOESPROC)eglGetProcAddress("glBindVertexArrayOES");
    }
#endif
#if defined(GL_KHR_debug)
    if(kdStrstrVEN((const KDchar *)glGetString(GL_EXTENSIONS), "GL_KHR_debug"))
    {
        glEnable(GL_DEBUG_OUTPUT_KHR);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR);
        glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKKHRPROC)eglGetProcAddress("glDebugMessageCallbackKHR");
        glDebugMessageCallback(&gl_callback, KD_NULL);
    }
#endif

    /* OpenGL */
    eglQuerySurface(egl_display, egl_surface, EGL_WIDTH, &width);
    eglQuerySurface(egl_display, egl_surface, EGL_HEIGHT, &height);
    glViewport(0, 0, width, height);

    /* GUI */
    device_init(&device);
    const void *image;
    KDint w, h;
    struct nk_font *font;
    nk_font_atlas_init_default(&atlas);
    nk_font_atlas_begin(&atlas);
    font = nk_font_atlas_add_default(&atlas, 13, 0);
    image = nk_font_atlas_bake(&atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
    device_upload_atlas(&device, image, w, h);
    nk_font_atlas_end(&atlas, nk_handle_id((int)device.font_tex), &device.null);
    nk_init_default(&ctx, &font->handle);

    while(!quit)
    {
        /* input */
        pump_input(&ctx, kd_window);

        /* draw */
        if(nk_begin(&ctx, "Calculator", nk_rect((width / 2) - 90, (height / 2) - 125, 180, 250),
               NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
        {
            static KDint set = 0, prev = 0, op = 0;
            static const KDchar numbers[] = "789456123";
            static const KDchar ops[] = "+-*/";
            static KDfloat64KHR a = 0, b = 0;
            static KDfloat64KHR *current = &a;

            KDsize i = 0;
            KDint solve = 0;
            {
                KDint len;
                KDchar buffer[256];
                nk_layout_row_dynamic(&ctx, 35, 1);
                len = kdSnprintfKHR(buffer, 256, "%.2f", *current);
                nk_edit_string(&ctx, NK_EDIT_SIMPLE, buffer, &len, 255, nk_filter_float);
                buffer[len] = 0;
                *current = kdStrtodKHR(buffer, KD_NULL);
            }

            nk_layout_row_dynamic(&ctx, 35, 4);
            for(i = 0; i < 16; ++i)
            {
                if(i >= 12 && i < 15)
                {
                    if(i > 12)
                        continue;
                    if(nk_button_label(&ctx, "C"))
                    {
                        a = b = op = 0;
                        current = &a;
                        set = 0;
                    }
                    if(nk_button_label(&ctx, "0"))
                    {
                        *current = *current * 10.0f;
                        set = 0;
                    }
                    if(nk_button_label(&ctx, "="))
                    {
                        solve = 1;
                        prev = op;
                        op = 0;
                    }
                }
                else if(((i + 1) % 4))
                {
                    if(nk_button_text(&ctx, &numbers[(i / 4) * 3 + i % 4], 1))
                    {
                        *current = *current * 10.0f + numbers[(i / 4) * 3 + i % 4] - '0';
                        set = 0;
                    }
                }
                else if(nk_button_text(&ctx, &ops[i / 4], 1))
                {
                    if(!set)
                    {
                        if(current != &b)
                        {
                            current = &b;
                        }
                        else
                        {
                            prev = op;
                            solve = 1;
                        }
                    }
                    op = ops[i / 4];
                    set = 1;
                }
            }
            if(solve)
            {
                if(prev == '+')
                    a = a + b;
                if(prev == '-')
                    a = a - b;
                if(prev == '*')
                    a = a * b;
                if(prev == '/')
                    a = a / b;
                current = &a;
                if(set)
                    current = &b;
                b = 0;
                set = 0;
            }
        }
        nk_end(&ctx);

        if(eglSwapBuffers(egl_display, egl_surface) == EGL_FALSE)
        {
            EGLint egl_error = eglGetError();
            switch(egl_error)
            {
                case(EGL_BAD_SURFACE):
                {
                    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                    eglDestroySurface(egl_display, egl_surface);
                    kdRealizeWindow(kd_window, &native_window);
                    egl_surface = eglCreateWindowSurface(egl_display, egl_config, native_window, KD_NULL);
                    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
                    break;
                }
                case(EGL_BAD_MATCH):
                case(EGL_BAD_CONTEXT):
                case(EGL_CONTEXT_LOST):
                {
                    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                    eglDestroyContext(egl_display, egl_context);
                    egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, egl_context_attributes);
                    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
                    break;
                }
                case(EGL_BAD_DISPLAY):
                case(EGL_NOT_INITIALIZED):
                case(EGL_BAD_ALLOC):
                {
                    kdAssert(0);
                    break;
                }
                default:
                {
                    kdAssert(0);
                    break;
                }
            }
        }
        else
        {
            eglQuerySurface(egl_display, egl_surface, EGL_WIDTH, &width);
            eglQuerySurface(egl_display, egl_surface, EGL_HEIGHT, &height);
            glViewport(0, 0, width, height);
            glClear(GL_COLOR_BUFFER_BIT);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            device_draw(&device, &ctx, width, height, NK_ANTI_ALIASING_ON);
        }
    }

    nk_font_atlas_clear(&atlas);
    nk_free(&ctx);
    device_shutdown(&device);

    eglDestroyContext(egl_display, egl_context);
    eglDestroySurface(egl_display, egl_surface);
    eglTerminate(egl_display);
    kdDestroyWindow(kd_window);
    return 0;
}
