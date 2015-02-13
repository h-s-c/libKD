/** \brief Header file for GLIMGUI - An OpenGL Immediate Mode GUI library
 *
 * \author Martin Felis <martin@silef.de>
 *
 * This work is heavily inspired by the great IMGUI tutorial of Jari Komppa
 * found at (http://sol.gfxile.net/imgui/index.html)
 *
 * The MIT License
 *
 * Copyright (c) 2010 Martin Felis <martin@silef.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#include <GL/gl.h>

#include "glimgui.h"
#include "stb_easy_font.h"

struct GLIMGUIState uistate;

/* utility functions */
void glimgui_printxy (float x, float y, float r, float g, float b, const char *str, ...) {
    // we constrain ourselves to strings shorter than 256 characters
    static char msg[256];
    static int i;

    // clear the output string
    for (i = 0; i < 256; i++)
        msg[i] = 0;

    va_list ap;
    va_start(ap, str);
    vsprintf(msg, str, ap);
    va_end(ap);

    static char buffer[99999]; // ~500 chars
    int num_quads;
    
    num_quads = stb_easy_font_print(x/uistate.textscale, y/uistate.textscale, msg, NULL, buffer, sizeof(buffer));
    glPushMatrix();
    glColor4f(r, g, b, 1.0f);
    glScalef(uistate.textscale, uistate.textscale, 1.0f);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, num_quads*4);
    glDisableClientState(GL_VERTEX_ARRAY);
    glPopMatrix();
}

void glimgui_draw_rect (int x, int y, int w, int h, float r, float g, float b) {
    GLint rect[] = {
            x, y, 0,
            x, y + h, 0,
            x + w, y + h, 0,
            x + w, y, 0
    };
    glPushMatrix();
    glColor4f(r, g, b, 1.0f);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_INT, 0, rect);
    glDrawArrays(GL_TRIANGLE_FAN,0,4);
    glDisableClientState(GL_VERTEX_ARRAY);
    glPopMatrix();
}

void glimgui_draw_rect_decoration (int id, int x, int y, int w, int h) {
    glimgui_draw_rect (x + 4, y + 4, w, h, 0.2, 0.2, 0.2);

    if (uistate.hotitem == id
            || uistate.kbditem == id) {
        if (uistate.activeitem == id) {
            glimgui_draw_rect (x, y, w, h, 0.8, 0.8, 0.8);
        } else {
            glimgui_draw_rect (x, y, w, h, 0.7, 0.7, 0.7);
        }
    } else {
        glimgui_draw_rect (x, y, w, h, 0.5, 0.5, 0.5);
    }
}

/** \brief Returns 1 if the mouse is in the specified rectangle */
static int regionhit (int x, int y, int w, int h) {
    if (uistate.mousepos_x < x ||
            uistate.mousepos_y < y ||
            uistate.mousepos_x >= x + w ||
            uistate.mousepos_y >= y + h)
        return 0;
    return 1;
}

static long systemtime() {
#ifdef CLOCK_MONOTONIC
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_nsec/1000000 + now.tv_sec*1000;
#elif defined(HAVE_GETTIMEOFDAY)
    struct timeval now;
    gettimeofday( &now, NULL );
    return now.tv_usec/1000 + now.tv_sec*1000;
#else
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    return now.tv_nsec/1000000 + now.tv_sec*1000;
#endif
}

/* Initialization, etc. */
void glimgui_init() {
    uistate.inittime = systemtime();
    uistate.textscale = 1.0f;

    // Here we reset the state of the global GLIMGUIState
    uistate.hotitem = 0;
    uistate.activeitem = 0;
    uistate.clear_state = 0;
    uistate.redraw_flag = 0;
    uistate.kbditem = 0;
    uistate.lastwidget = 0;

    uistate.last_character = 0;
    uistate.last_specialkey = 0;

    uistate.left_mousebutton_state = 0;

    // some huge values that are hopefully not on the screen
    uistate.mousepos_x = INT_MIN;
    uistate.mousepos_y = INT_MIN;
}

void glimgui_prepare () {
    if (uistate.clear_state) {
        // if clear_state was set this means we are switching from one widget
        // layout to another one. In this case we have to reset the kbditem so
        // that the first drawn widget can grab its focus. Otherwise an old and
        // invalid id could linger in uistate.kbditem.
        uistate.clear_state = 0;
        uistate.kbditem = 0;
    }
    uistate.hotitem = 0;
    uistate.redraw_flag = 0;
}

void glimgui_clear() {
    // By setting uistate.clear_state all subsequent widgets will not get drawn,
    // instead we clear the uistate and draw from scratch.
    uistate.clear_state = 1;
    uistate.redraw_flag = 1;

    uistate.hotitem = 0;
    uistate.kbditem = 0;
}

void glimgui_finish() {
    if (uistate.left_mousebutton_state == 0) {
        uistate.activeitem = 0;
    }

    uistate.last_character = 0;
    uistate.last_specialkey = 0;
}

void glimgui_label (int id, const char *caption, int x, int y, int w, int h) {

    glimgui_printxy(x , y + h * 0.5 + -8 * uistate.textscale * 0.3, 1.0f, 1.0f, 1.0f, caption);
}

int glimgui_button (int id, const char* caption, int x, int y, int w, int h) {
    // abort if we are about to switch to another widget layout
    if (uistate.clear_state)
        return 0;

    if (regionhit (x, y, w, h)) {
        if (uistate.hotitem != 0 && uistate.hotitem != id)
            uistate.redraw_flag = 1;

        uistate.hotitem = id;
        uistate.kbditem = id;

        if (uistate.activeitem == 0
                && uistate.left_mousebutton_state) {
            uistate.activeitem = id;
        }
    }

    // if we are still active, but the mouse has moved on, we need to redraw as
    // we won't be active after this frame
    if (uistate.activeitem == id && uistate.hotitem != id)
        uistate.redraw_flag = 1;

    // If nothing is selected we grab the hotness
    if (uistate.hotitem == 0 && uistate.kbditem == 0) {
        if (uistate.hotitem != id)
            uistate.redraw_flag = 1;

        uistate.hotitem = id;
        uistate.kbditem = uistate.hotitem;
    }

    if (uistate.kbditem == 0) {
        if (uistate.hotitem != 0 && uistate.hotitem != id)
            uistate.redraw_flag = 1;

        uistate.hotitem = id;
        uistate.kbditem = id;
    }

    // Draw the decoration
    glimgui_draw_rect_decoration(id, x, y, w, h);

    // Caption
    if (caption != NULL) {
        float str_width = stb_easy_font_width((char*)caption)*uistate.textscale;
        glimgui_printxy(x + w * 0.5 - str_width * 0.5, y + h * 0.5 + -8 * uistate.textscale * 0.3, 1.0f, 1.0f, 1.0f, caption);
    }

    // Mouse Logic
    if (uistate.left_mousebutton_state == 0
            && uistate.hotitem == id
            && uistate.activeitem == id) {
        uistate.lastwidget = id;
        uistate.redraw_flag = 1;

        return 1;
    }

    // Keyboard Logic
    if (uistate.kbditem == id) {

        // We have to make sure, that we always clear the uistate.last_specialkey
        // value, otherwise the same action might be repeated over and over.
        switch (uistate.last_specialkey) {
            case GLIMGUI_KEY_DOWN:
                uistate.kbditem = 0;
                uistate.hotitem = 0;
                uistate.redraw_flag = 1;
                break;
            case GLIMGUI_KEY_UP:
                uistate.kbditem = uistate.lastwidget;
                uistate.hotitem = uistate.lastwidget;
                uistate.redraw_flag = 1;
                break;
            default:
                break;
        }

        // We need to "consume" all specialkeys, otherwise they get reported to
        // all other widgets.
        uistate.last_specialkey = 0;

        switch(uistate.last_character) {
            case 13:
                // We also need to clear this value ...
                uistate.last_character = 0;
                uistate.redraw_flag = 1;
                return 1;
            default:
                break;
        }
    }

    uistate.lastwidget = id;

    return 0;
}

int glimgui_lineedit (int id, char *text_value, int maxlength, int x, int y, int w, int h) {
    // abort if we are about to switch to another widget layout

    if (uistate.clear_state)
        return 0;

    if (w < 0) {
        w = (maxlength + 2) * 8 ;
    }

    if (h < 0) {
        h = 16;
    }

    int text_length = strlen(text_value);

    // Check for hotness
    if (regionhit (x, y, w, h)) {
        if (uistate.hotitem != 0 && uistate.hotitem != id)
            uistate.redraw_flag = 1;

        uistate.hotitem = id;
        if (uistate.activeitem == 0
                && uistate.left_mousebutton_state) {
            uistate.activeitem = id;
        }
    }

    // If the keyboard is currently idle, we grab it. Any remaining characters
    // get ignored so that we do not print old characters in this lineedit.
    if (uistate.hotitem == 0 && uistate.kbditem == 0) {
        if (uistate.hotitem != id)
            uistate.redraw_flag = 1;

        uistate.hotitem = id;
        uistate.kbditem = id;
        uistate.last_character = 0;
    }

    if (uistate.kbditem == id)
            uistate.redraw_flag = 1;

    // Draw the decoration
    glimgui_draw_rect_decoration(id, x, y, w, h);

    // Rendering of the current value

    // We copy the text as we might append a cursor symbol '_'. This adds some
    // allocation/deallocation overhead but makes the code a little simpler
    // here.
    char *text_copy = text_value;
    text_copy = malloc (sizeof(char) * (text_length + 2));
    memcpy (text_copy, text_value, sizeof(char) * (text_length + 1));

    long time = systemtime() - uistate.inittime;

    if (uistate.kbditem == id && time >> 9 & 1) {
        text_copy[text_length] = '_';
        text_copy[text_length + 1] = '\0';
        uistate.redraw_flag = 1;
    }

    glimgui_printxy(x + 5, y + h * 0.5 + -9 * uistate.textscale * 0.3, 1.0f, 1.0f, 1.0f, text_copy);

    free (text_copy);

    // Keyboard Logic
    if (uistate.kbditem == id) {
        switch (uistate.last_specialkey) {
            case GLIMGUI_KEY_DOWN:
                uistate.kbditem = 0;
                uistate.hotitem = 0;
                uistate.redraw_flag = 1;
                break;
            case GLIMGUI_KEY_UP:
                uistate.kbditem = uistate.lastwidget;
                uistate.hotitem = uistate.lastwidget;
                uistate.redraw_flag = 1;
                break;
            default:
                break;
        }

        // We need to "consume" all specialkeys, otherwise they get reported to
        // all other widgets.
        uistate.last_specialkey = 0;

        switch(uistate.last_character) {
            case 27: // ESC
                uistate.hotitem = 0;
                uistate.kbditem = 0;
                uistate.redraw_flag = 1;
                return 0;
            case 13: // Return
                uistate.hotitem = 0;
                uistate.kbditem = 0;
                uistate.last_character = 0;
                uistate.redraw_flag = 1;
                return 1;
            case 8:  // Backspace
                if (text_length > 0) {
                    text_value[text_length - 1] = '\0';
                    uistate.redraw_flag = 1;
                    return 1;
                }
                break;
            default:
                // The raw input processing
                if (maxlength > 0 && text_length < maxlength) {
                    if (uistate.last_character) {
                        if ((uistate.last_character) != 0) {
                            text_value[text_length] = uistate.last_character;
                            text_value[text_length + 1] = '\0';
                            uistate.redraw_flag = 1;
                            return 1;
                        } else {
                            fprintf(stderr, "Input key ' ' (%d) not supported!",
                                    uistate.last_character);
                            return 0;
                        }
                    }
                }
                break;
        }
    }

    uistate.lastwidget = id;

    // Mouse Logic
    if (uistate.left_mousebutton_state == 0
            && uistate.hotitem == id
            && uistate.activeitem == id) {
        uistate.kbditem = id;
        uistate.redraw_flag = 1;
    }

    return 0;
}

/* widgets */
