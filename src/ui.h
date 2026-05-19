#ifndef UI_H
#define UI_H

#include <gtk/gtk.h>
#include "data_manager.h"

typedef struct {
    GtkBuilder* builder;
    GtkWidget* window;
    GtkWidget* notebook;
    GtkWidget* course_view;
    GtkWidget* course_table;
    GtkWidget* student_view;
    GtkWidget* student_table;
    GtkWidget* selection_view;
    GtkWidget* student_combo;
    GtkWidget* selection_table;
    GtkWidget* selection_info;
    GtkWidget* grade_view;
    GtkWidget* grade_student_combo;
    GtkWidget* grade_table;
    GtkWidget* stats_view;
    GtkWidget* stats_table;
} AppWidgets;

void create_ui(int argc, char* argv[]);
void refresh_course_table();
void refresh_student_table();
void refresh_student_combo(GtkComboBoxText* combo);
void refresh_selection_table();
void refresh_grade_table();

#endif /* UI_H */
