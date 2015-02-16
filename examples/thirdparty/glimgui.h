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

#ifndef _GLIMGUI_H
#define _GLIMGUI_H

#define GLIMGUI_KEY_UP 1
#define GLIMGUI_KEY_DOWN 2

/** \brief The state of the IMGUI and the input devices */
struct GLIMGUIState {
    long inittime;
    float textscale;
    int width;
    int height;

    /// \brief Id of the item on which the mouse is currently
    int hotitem;
    /// \brief Id of the item on which the mouse is currently and the left button is pressed
    int activeitem;
    /// \brief Id of the idem that has currently keyboard focus
    int kbditem;

    /// \brief Defines whether the state should be cleared at the beginnig of the next frame
    int clear_state;
    /// \brief Marks whether the gui should be redrawn
    int redraw_flag;

    /// \brief Last character reported from the keyboard
    char last_character;
    /// \brief Special characters (e.g. function keys or arrow keys)
    int last_specialkey;

    /// \brief The previosly drawn widget
    int lastwidget;

    /// \brief Current mouse position
    int mousepos_x;
    int mousepos_y;

    /// \brief State of the left mouse button (is 1 if pressed, 0 otherwise)
    int left_mousebutton_state;
};

/** \brief The global uistate to which input events are reported */
extern struct GLIMGUIState uistate;

/* helper function */

/** \brief Draws the string at the given position */
void glimgui_printxy (float x, float y, float r, float g, float b, const char *str, ...);
/** \brief Draws a filled 2D recangle at given position and size */
void glimgui_draw_rect (float x, float y, float w, float h, float r, float g, float b);
/** \brief Draws a rectangular decoration for the widget with id
 * Performs the decoration dependent on hotness and activeness. 
 * */
void glimgui_draw_rect_decoration (int id, float x, float y, float w, float h);

/* initialization */
void glimgui_init ();

/** \brief State preparation before the drawing */
void glimgui_prepare ();
/** \brief State cleanup after the drawing */
void glimgui_finish ();
/** \brief Clears the state when a new gui layout should be drawn
 * After calling this function no further widgets are drawn and at the
 * beginning both uistate.hotitem and uistate.kbditem are reset. */
void glimgui_clear ();

/* widgets */

/** \brief Draws a label with the given position and caption */
void glimgui_label (int id, const char *caption, float x, float y, float w, float h);
/** \brief Draws a button with a given caption (returns 1 if it was pressed) */
int glimgui_button (int id, const char* caption, float x, float y, float w, float h);
/** \brief Draws a one-lined textfiled that modifies the given string */
int glimgui_lineedit (int id, char *text_value, int maxlength, float x, float y, float w, float h);

#endif /* _GLIMGUI_H */
