/* -*- c-file-style: "java" -*-
 * $Id: form_ui.c,v 1.9 2006-06-28 10:33:43 zeeb90au Exp $
 * This file is part of SmallBASIC
 *
 * Copyright(C) 2001-2006 Chris Warren-Smith. Gawler, South Australia
 * cwarrens@twpo.com.au
 *
 * This program is distributed under the terms of the GPL v2.0 or later
 * Download the GNU Public License(GPL) from www.gnu.org
 */ 

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "sys.h"
#include "var.h"
#include "kw.h"
#include "pproc.h"
#include "device.h"
#include "smbas.h"
#include "blib_ui.h"

#include <gtk/gtk.h>

#ifdef USE_HILDON
#include <libosso.h>
#endif

#include "output_model.h"

GtkWidget* form = 0; /* modal form */
int modeless = FALSE;
extern OutputModel output;

typedef enum ControlType {
    ctrl_button,
    ctrl_radio,
    ctrl_check,
    ctrl_text,
    ctrl_label,
    ctrl_list,
    ctrl_calendar,
    ctrl_file_button,
    ctrl_font_button,
    ctrl_color_button
} ControlType;

typedef struct WidgetInfo {
    ControlType type;
    var_t* var;
} WidgetInfo;

WidgetInfo* get_widget_info(GtkWidget* w) {
    return (WidgetInfo*)g_object_get_data(G_OBJECT(w), "widget_info");
}

void set_widget_info(GtkWidget* w, WidgetInfo* inf) {
    g_object_set_data(G_OBJECT(w), "widget_info", inf);
}

// make width and height fit within the output box
void update_dimensions(int x, int y, int* w, int* h) {
    if (*w < 1 || x+*w > output.width) {
        *w = output.width-x;
    }
    if (*h < 1 || y+*h > output.height) {
        *h = output.height-y;
    }
}

// create the form
void ui_begin() {
    if (form == 0) {
        form = gtk_layout_new(NULL, NULL);
        gtk_container_add(GTK_CONTAINER(output.widget), form);
        gtk_widget_show(form);
    }
}

// destroy the form
void ui_reset() {
    if (form != 0) {
        GList* list = gtk_container_get_children(GTK_CONTAINER(form));
        int n = g_list_length(list);
        int i;
        for (i=0; i<n; i++) {
            GtkWidget* w = (GtkWidget*)g_list_nth_data(list, i);
            WidgetInfo* inf = get_widget_info(w);
            g_free(inf);
            gtk_container_remove(GTK_CONTAINER(form), w);
        }
        gtk_container_remove(GTK_CONTAINER(output.widget), form);
        g_list_free(list);
        form = 0;
    }
}

// copy widget data into its matching basic variable
void update_vars(GtkWidget* widget) {
    WidgetInfo* inf = get_widget_info(widget);
    gchar* text = 0;
    guint year, month, day;
    char buf[11]; // DD/MM/YYYY
    GdkColor color;

    switch (inf->type) {
    case ctrl_check:
    case ctrl_radio:
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
            v_setstr(inf->var, gtk_button_get_label(GTK_BUTTON(widget)));
        }
        break;
    case ctrl_text:
        text = (gchar*)gtk_entry_get_text(GTK_ENTRY(widget));
        if (text && text[0]) {
            v_setstr(inf->var, text);
        }
        // internal text not freed
        break;
    case ctrl_list:
        text = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));
        if (text && text[0]) {
            v_setstr(inf->var, text);
        }
        g_free(text);
        break;
    case ctrl_calendar: 
        gtk_calendar_get_date(GTK_CALENDAR(widget), &year, &month, &day);
        sprintf(buf, "%02d/%02d/%d", day, month+1, year);
        v_setstr(inf->var, buf);
        break;
    case ctrl_file_button:
        text = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));
        if (text && text[0]) {
            v_setstr(inf->var, text);
        }
        g_free(text);
        break;
    case ctrl_font_button:
        text = (char*)gtk_font_button_get_font_name(GTK_FONT_BUTTON(widget));
        if (text && text[0]) {
            v_setstr(inf->var, text);
        }
        break;
    case ctrl_color_button:
        gtk_color_button_get_color(GTK_COLOR_BUTTON(widget), &color);
        text = gtk_color_selection_palette_to_string(&color, 1);
        if (text && text[0]) {
            v_setstr(inf->var, text);
        }
        g_free(text);
        break;
    default:
        break;
    }
}

// copy form data into basic variables
void ui_transfer_data() {
    if (form) {
        GList* list = gtk_container_get_children(GTK_CONTAINER(form));
        int n = g_list_length(list);
        int i;
        for (i=0; i<n; i++) {
            update_vars((GtkWidget*)g_list_nth_data(list, i));
        }
        g_list_free(list);
    }
}

// button callback
void button_clicked(GtkWidget *button, gpointer user_data) {
    WidgetInfo* inf = get_widget_info(button);
    v_setstr(inf->var, gtk_button_get_label(GTK_BUTTON(button)));
    if (user_data) {
        // close the form
        output.modal_flag = FALSE;
        if (modeless) {
            ui_transfer_data();
            ui_reset();
        }
    }
}

// set the radio into a group that shares a common basic variable
void set_radio_group(var_t* v, GtkWidget* radio_widget) {
    GSList *radio_group = NULL;
    if (v == 0 || v->type != V_STR) {
        return;
    }

    GList* list = gtk_container_get_children(GTK_CONTAINER(form));
    int n = g_list_length(list);
    int i;
    for (i=0; i<n; i++) {
        GtkWidget* widget = (GtkWidget*)g_list_nth_data(list, i);
        WidgetInfo* inf = get_widget_info(widget);
        if (inf->type == ctrl_radio &&
            inf->var->type == V_STR && 
            (inf->var == v || inf->var->v.p.ptr == v->v.p.ptr)) {
            radio_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(widget));
            gtk_radio_button_set_group(GTK_RADIO_BUTTON(radio_widget), radio_group);
            break;
        }
    }
    g_list_free(list);
}

// BUTTON x, y, w, h, variable, caption [,type] 
//
// type can optionally be 'radio' | 'checkbox' | 'link' | 'choice'
// variable is set to 1 if a button or link was pressed (which 
// will have closed the form, or if a radio or checkbox was 
// selected when the form was closed
//
// TODO: try adding hildon widgets:
//  http://www.maemo.org/platform/docs/api/hildon-docs/html/ch02.html
//
void cmd_button() {
    int x, y, w, h;
    var_t* v = 0;
    char* caption = 0;
    char* type = 0;

    if (-1 != par_massget("IIIIPSs", &x, &y, &w, &h, &v, &caption, &type)) {
        GtkWidget* widget = 0;
        WidgetInfo* inf = (WidgetInfo*)g_malloc(sizeof(WidgetInfo));
        inf->var = v;

        ui_begin();
        if (type) {
            if (strncmp("radio", type, 5) == 0) {
                inf->type = ctrl_radio;
                widget = gtk_radio_button_new_with_label(NULL, caption);
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
                set_radio_group(v, widget);
                g_signal_connect((gpointer)widget, "clicked",
                                 G_CALLBACK(button_clicked),NULL);
            } else if (strncmp("checkbox", type, 8) == 0) {
                inf->type = ctrl_check;
                widget = gtk_check_button_new_with_label(caption);
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
                g_signal_connect((gpointer)widget, "clicked", 
                                 G_CALLBACK(button_clicked), NULL);
            } else if (strncmp("button", type, 6) == 0) {
                inf->type = ctrl_button;
                widget = gtk_button_new_with_mnemonic(caption);
                g_signal_connect((gpointer)widget, "clicked", 
                                 G_CALLBACK(button_clicked), NULL);
            } else if (strncmp("label", type, 5) == 0) {
                inf->type = ctrl_label;
                widget = gtk_label_new(caption);
                gtk_label_set_justify(GTK_LABEL(widget), GTK_JUSTIFY_LEFT);
            } else if (strncmp("calendar", type, 8) == 0) {
                inf->type = ctrl_calendar;
                widget = gtk_calendar_new();
                gtk_calendar_display_options(GTK_CALENDAR(widget),
                                             GTK_CALENDAR_SHOW_HEADING |
                                             GTK_CALENDAR_SHOW_DAY_NAMES);
            } else if (strncmp("file_button", type, 11) == 0) {
                inf->type = ctrl_file_button;
                widget = gtk_file_chooser_button_new(caption, GTK_FILE_CHOOSER_ACTION_OPEN);
            } else if (strncmp("font_button", type, 11) == 0) {
                inf->type = ctrl_font_button;
                widget = gtk_font_button_new();
            } else if (strncmp("color_button", type, 12) == 0) {
                inf->type = ctrl_color_button;
                widget= gtk_color_button_new();
            } else if (strncmp("choice", type, 6) == 0) {
                inf->type = ctrl_list;
                widget = gtk_combo_box_new_text();
                // "Easy|Medium|Hard"
                int itemIndex = 0;
                int len = caption ? strlen(caption) : 0;
                int i;
                for (i=0; i<len; i++) {
                    const char* c = strchr(caption+i, '|');
                    int endIndex = c ? c-caption : len;
                    GString* s = g_string_new_len(caption+i, endIndex-i);
                    gtk_combo_box_append_text(GTK_COMBO_BOX(widget), s->str);
                    i = endIndex;
                    if (v->type == V_STR && v->v.p.ptr &&
                        strcmp((const char*)v->v.p.ptr, s->str) == 0) {
                        // item text same as variable - set selected
                        gtk_combo_box_set_active(GTK_COMBO_BOX(widget), itemIndex);
                    }
                    itemIndex++;
                    g_string_free(s, TRUE);
                }
            } else {
                ui_reset();
                rt_raise("UI: UNKNOWN TYPE: %s", type);
                pfree2(caption, type);
                return;
            }
        }

        if (widget == 0) {
            // button will close the form
            inf->type = ctrl_button;
            widget = gtk_button_new_with_mnemonic(caption);
            g_signal_connect((gpointer)widget, "clicked",
                             G_CALLBACK(button_clicked), (gpointer)TRUE);
        }

        set_widget_info(widget, inf);
        gtk_layout_put(GTK_LAYOUT(form), widget, x, y);
        gtk_widget_set_size_request(widget, w, h);
        gtk_widget_modify_font(widget, output.font_desc);

        // prime input field from variable
        if (v->type == V_STR && v->v.p.ptr &&
            strcmp((const char*)v->v.p.ptr, caption) == 0) {
            if (inf->type == ctrl_check || 
                inf->type == ctrl_radio) {
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
            }
        }
    }
    pfree2(caption, type);
}

// TEXT x, y, w, h, variable
// When DOFORM returns the variable contains the user entered value
//
void cmd_text() {
    int x, y, w, h;
    var_t* v = 0;

    if (-1 != par_massget("IIIIP", &x, &y, &w, &h, &v)) {
        ui_begin();
        GtkWidget* entry = gtk_entry_new();

        // prime field from var_t
        if (v->type == V_STR && v->v.p.ptr) {
            gtk_entry_set_text(GTK_ENTRY(entry), (const char*)v->v.p.ptr);
        }

        gtk_layout_put(GTK_LAYOUT(form), entry, x, y);
        gtk_entry_set_has_frame(GTK_ENTRY(entry), TRUE);
        gtk_entry_set_max_length(GTK_ENTRY(entry), 100);
        gtk_widget_set_size_request(entry, w, h);
        gtk_widget_modify_font(entry, output.font_desc);
        
        GtkIMContext* imctx = gtk_im_multicontext_new();
        gtk_im_context_set_client_window(imctx, output.widget->window);
        gtk_im_context_focus_in(imctx);
        gtk_widget_grab_focus(entry);

#ifdef USE_HILDON
        g_object_set(entry, "hildon-input-mode", 
                     HILDON_GTK_INPUT_MODE_AUTOCAP, NULL); 
        gtk_im_context_show(imctx);
#endif
        WidgetInfo* inf = (WidgetInfo*)g_malloc(sizeof(WidgetInfo));
        inf->var = v;
        inf->type = ctrl_text;
        set_widget_info(entry, inf);
    }
}

// DOFORM [x,y,w,h]
//
// Modal syntax:
//   BUTTON ...
//   DOFORM ...
//
// Modeless syntax:
//   DOFORM 'begin modeless form
//   BUTTON ....
//   DOFORM x,y,w,h
//   DOFORM 'end modeless form - can also be closed with default button
//
void cmd_doform() {
    int x, y, w, h, box, bg;
    int num_args;

    x = y = w = h = box = bg = 0;
    num_args = par_massget("iiii", &x, &y, &w, &h, &box, &bg);

    if (num_args == 0) {
        // begin or end modeless form state
        if (modeless && form) {
            modeless = FALSE;
            ui_transfer_data();
            ui_reset();
            return;
        }

        if (form == 0) {
            modeless = TRUE;
            return;
        }
    }

    if (num_args < 4 || num_args > 6) {
        // box and bg are for compatibility with fltk version
        ui_reset();
        rt_raise("UI: INVALID FORM ARGUMENTS: %d", num_args);
        return;
    }

    if (w < 1 || x+w > output.width) {
        w = output.width-x;
    }
    if (h < 1 || y+h > output.height) {
        h = output.height-y;
    }

    gtk_layout_move(GTK_LAYOUT(output.widget), form, x, y);
    gtk_layout_set_size(GTK_LAYOUT(form), w, h);
    gtk_widget_set_size_request(GTK_WIDGET(form), w, h);
    gtk_widget_grab_focus(form);
    gtk_widget_show_all(form);

    if (modeless == FALSE) {
        output.modal_flag = TRUE;
        while (output.modal_flag && output.break_exec == 0) {
            gtk_main_iteration_do(TRUE);
        }
        
        ui_transfer_data();
        ui_reset();
    }
}

/* End of "$Id: form_ui.c,v 1.9 2006-06-28 10:33:43 zeeb90au Exp $". */
