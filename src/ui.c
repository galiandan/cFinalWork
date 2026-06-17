#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "service.h"
#include "storage.h"

static AppState* g_state = NULL;
static const char* g_courses_path = NULL;
static const char* g_students_path = NULL;
static const char* g_export_path = NULL;

static GtkTreeViewColumn* add_text_column(GtkTreeView* view, const char* title,
                                          int col_id);
static GtkTreeViewColumn* add_editable_column(GtkTreeView* view,
                                              const char* title, int col_id,
                                              GCallback on_edited);
static void refresh_all_views(void);
static void refresh_course_table(void);
static void refresh_student_table(void);
static void refresh_student_combo(GtkComboBoxText* combo);
static void refresh_selection_table(void);
static void refresh_student_info_label(void);
static void refresh_grade_table(void);
static void refresh_stats_table(gboolean sorted);
static void refresh_all_combos(void);
static int persist_state(void);
static void show_message(GtkMessageType type, const char* message);
static void on_save_all(GtkWidget* widget, gpointer user_data);
static void on_save_grades(GtkButton* btn, gpointer user_data);
static void on_add_course_to_system(GtkButton* btn, gpointer user_data);
static void on_edit_course(GtkButton* btn, gpointer user_data);
static void on_delete_course(GtkButton* btn, gpointer user_data);
static void on_add_student(GtkButton* btn, gpointer user_data);
static void on_delete_student(GtkButton* btn, gpointer user_data);
static void on_edit_student(GtkButton* btn, gpointer user_data);
static void on_add_course(GtkButton* btn, gpointer user_data);
static void on_drop_course(GtkButton* btn, gpointer user_data);
static void on_finish_selection(GtkButton* btn, gpointer user_data);
static void on_student_combo_changed(GtkComboBox* combo, gpointer user_data);
static void on_grade_cell_edited(GtkCellRendererText* cell, gchar* path_string,
                                 gchar* new_text, gpointer user_data);
static void on_sort_by_credits(GtkButton* btn, gpointer user_data);
static void on_fail_stats(GtkButton* btn, gpointer user_data);
static void on_export_csv(GtkWidget* widget, gpointer user_data);

static void show_message(GtkMessageType type, const char* message)
{
    GtkWidget* dialog = gtk_message_dialog_new(
        GTK_WINDOW(g_state->widgets.window), GTK_DIALOG_MODAL, type, GTK_BUTTONS_OK,
        "%s", message ? message : "");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static int persist_state(void)
{
    if (!storage_save_courses(g_state, g_courses_path) ||
        !storage_save_students(g_state, g_students_path))
    {
        show_message(GTK_MESSAGE_WARNING, "保存失败");
        return 0;
    }
    return 1;
}

static void refresh_course_table(void)
{
    GtkListStore* store;

    if (!g_state->widgets.course_table)
        return;

    store = GTK_LIST_STORE(
        gtk_tree_view_get_model(GTK_TREE_VIEW(g_state->widgets.course_table)));
    gtk_list_store_clear(store);
    for (int i = 0; i < g_state->course_count; i++)
    {
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, g_state->courses[i].id, 1,
                           g_state->courses[i].name, 2, g_state->courses[i].capacity,
                           3, g_state->courses[i].enrolled, 4,
                           g_state->courses[i].credits, 5,
                           g_state->courses[i].schedule, -1);
    }
}

static void refresh_student_table(void)
{
    GtkTreeView* view = GTK_TREE_VIEW(
        g_object_get_data(G_OBJECT(g_state->widgets.notebook), "student_table_view"));
    GtkListStore* store;

    if (!view)
        return;

    store = GTK_LIST_STORE(gtk_tree_view_get_model(view));
    gtk_list_store_clear(store);
    for (int i = 0; i < g_state->student_count; i++)
    {
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, i + 1, 1, g_state->students[i].name, 2,
                           g_state->students[i].student_id, 3,
                           g_state->students[i].gender, 4,
                           g_state->students[i].num_courses, 5,
                           g_state->students[i].total_credits, -1);
    }
}

static void refresh_student_combo(GtkComboBoxText* combo)
{
    gint active;

    if (!combo)
        return;

    active = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
    if (active == -1 && g_state->student_count > 0)
        active = 0;

    gtk_combo_box_text_remove_all(combo);
    for (int i = 0; i < g_state->student_count; i++)
    {
        gchar* text = g_strdup_printf("%s (%s)", g_state->students[i].name,
                                      g_state->students[i].student_id);
        gtk_combo_box_text_append(combo, NULL, text);
        g_free(text);
    }

    if (g_state->student_count == 0)
        active = -1;
    else if (active >= g_state->student_count)
        active = g_state->student_count - 1;

    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), active);
}

static void refresh_selection_table(void)
{
    GtkTreeView* view = GTK_TREE_VIEW(g_object_get_data(
        G_OBJECT(g_state->widgets.selection_view), "selection_table"));
    GtkTreeSelection* selection;
    GtkTreeModel* old_model;
    GtkListStore* store;
    GtkTreeIter iter;
    gint saved_course_id = -1;

    if (!view)
        return;

    selection = gtk_tree_view_get_selection(view);
    if (gtk_tree_selection_get_selected(selection, &old_model, &iter))
        gtk_tree_model_get(old_model, &iter, 0, &saved_course_id, -1);

    store = GTK_LIST_STORE(gtk_tree_view_get_model(view));
    gtk_list_store_clear(store);
    for (int i = 0; i < g_state->course_count; i++)
    {
        GtkTreeIter new_iter;
        gtk_list_store_append(store, &new_iter);
        gtk_list_store_set(store, &new_iter, 0, g_state->courses[i].id, 1,
                           g_state->courses[i].name, 2, g_state->courses[i].capacity,
                           3, g_state->courses[i].enrolled, 4,
                           g_state->courses[i].credits, 5,
                           g_state->courses[i].schedule, -1);
        if (saved_course_id == g_state->courses[i].id)
        {
            GtkTreePath* path =
                gtk_tree_model_get_path(GTK_TREE_MODEL(store), &new_iter);
            gtk_tree_selection_select_path(selection, path);
            gtk_tree_path_free(path);
        }
    }
}

static void refresh_student_info_label(void)
{
    GtkLabel* label = GTK_LABEL(
        g_object_get_data(G_OBJECT(g_state->widgets.selection_view), "info_label"));
    GtkComboBoxText* combo = GTK_COMBO_BOX_TEXT(
        g_object_get_data(G_OBJECT(g_state->widgets.selection_view), "student_combo"));

    if (!label || !combo)
        return;

    gint active = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
    if (active < 0 || active >= g_state->student_count)
    {
        gtk_label_set_text(label, "请选择学生");
        return;
    }

    Student* student = &g_state->students[active];
    GString* info = g_string_new("");
    g_string_append_printf(info,
                           "学生: %s | 学号: %s | 性别: %s | 已选: %d 门 | 学分: %d | 课程: ",
                           student->name, student->student_id, student->gender,
                           student->num_courses, student->total_credits);
    for (int i = 0; i < student->num_courses; i++)
    {
        int course_index =
            service_get_course_index(g_state, student->courses[i]);
        if (course_index >= 0)
            g_string_append_printf(info, "%s ", g_state->courses[course_index].name);
    }

    gtk_label_set_text(label, info->str);
    g_string_free(info, TRUE);
}

static void refresh_grade_table(void)
{
    GtkTreeView* view = GTK_TREE_VIEW(
        g_object_get_data(G_OBJECT(g_state->widgets.grade_view), "grade_table"));
    GtkComboBoxText* combo = GTK_COMBO_BOX_TEXT(
        g_object_get_data(G_OBJECT(g_state->widgets.grade_view), "student_combo"));
    GtkLabel* gpa_label = GTK_LABEL(
        g_object_get_data(G_OBJECT(g_state->widgets.grade_view), "gpa_label"));
    GtkListStore* store;

    if (!view || !combo)
        return;

    store = GTK_LIST_STORE(gtk_tree_view_get_model(view));
    gtk_list_store_clear(store);

    gint active = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
    if (active < 0 || active >= g_state->student_count)
    {
        if (gpa_label)
            gtk_label_set_text(gpa_label, "GPA: 0.00");
        return;
    }

    Student* student = &g_state->students[active];
    for (int i = 0; i < student->num_courses; i++)
    {
        int course_id = student->courses[i];
        int course_index = service_get_course_index(g_state, course_id);
        GtkTreeIter iter;
        gchar* score_text;
        const char* status_text;

        if (course_index < 0)
            continue;

        gtk_list_store_append(store, &iter);
        {
            int score = service_get_score(student, course_id);
            score_text = score >= 0 ? g_strdup_printf("%d", score) : g_strdup("");
            if (score < 0)
                status_text = "未录入";
            else if (score >= PASS_SCORE)
                status_text = "及格";
            else
                status_text = "不及格";
        }

        gtk_list_store_set(store, &iter, 0, course_id, 1,
                           g_state->courses[course_index].name, 2, score_text, 3,
                           status_text, -1);
        g_free(score_text);
    }

    if (gpa_label)
    {
        gchar* gpa_text =
            g_strdup_printf("GPA: %.2f %s", student->gpa,
                            student->gpa_warning ? "[学业预警]" : "");
        gtk_label_set_text(gpa_label, gpa_text);
        g_free(gpa_text);
    }
}

static void refresh_stats_table(gboolean sorted)
{
    GtkTreeView* view = GTK_TREE_VIEW(
        g_object_get_data(G_OBJECT(g_state->widgets.stats_view), "stats_table"));
    GtkListStore* store;
    Student sorted_students[MAX_STUDENTS];

    if (!view)
        return;

    store = GTK_LIST_STORE(gtk_tree_view_get_model(view));
    gtk_list_store_clear(store);
    if (sorted)
        service_sort_students_by_credits(g_state, sorted_students);

    for (int i = 0; i < g_state->student_count; i++)
    {
        const Student* student = sorted ? &sorted_students[i] : &g_state->students[i];
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, i + 1, 1, student->name, 2,
                           student->student_id, 3, student->gender, 4,
                           student->num_courses, 5, student->total_credits, 6,
                           student->fail_count, -1);
    }
}

static void refresh_all_combos(void)
{
    GtkWidget* selection_combo = GTK_WIDGET(g_object_get_data(
        G_OBJECT(g_state->widgets.selection_view), "student_combo"));
    GtkWidget* grade_combo = GTK_WIDGET(
        g_object_get_data(G_OBJECT(g_state->widgets.grade_view), "student_combo"));

    if (selection_combo)
        refresh_student_combo(GTK_COMBO_BOX_TEXT(selection_combo));
    if (grade_combo)
        refresh_student_combo(GTK_COMBO_BOX_TEXT(grade_combo));
}

static void refresh_all_views(void)
{
    if (!g_state->widgets.notebook)
        return;

    refresh_course_table();
    refresh_student_table();
    refresh_selection_table();
    refresh_all_combos();
    refresh_stats_table(FALSE);
    refresh_student_info_label();
    refresh_grade_table();
}

static void on_save_all(GtkWidget* widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;
    if (persist_state())
        show_message(GTK_MESSAGE_INFO, "保存成功");
}

static void on_save_grades(GtkButton* btn, gpointer user_data)
{
    (void)btn;
    (void)user_data;
    if (persist_state())
        refresh_all_views();
}

static void on_add_course_to_system(GtkButton* btn, gpointer user_data)
{
    GtkWidget* dialog;
    GtkWidget* content;
    GtkWidget* grid;
    GtkWidget* name_entry;
    GtkWidget* capacity_entry;
    GtkWidget* credits_entry;
    GtkWidget* schedule_entry;

    (void)btn;
    (void)user_data;

    dialog = gtk_dialog_new_with_buttons("添加课程",
                                         GTK_WINDOW(g_state->widgets.window),
                                         GTK_DIALOG_MODAL, "_取消",
                                         GTK_RESPONSE_CANCEL, "_确定",
                                         GTK_RESPONSE_ACCEPT, NULL);
    content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    name_entry = gtk_entry_new();
    capacity_entry = gtk_entry_new();
    credits_entry = gtk_entry_new();
    schedule_entry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("课程名称:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), name_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("容量:"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), capacity_entry, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("学分:"), 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), credits_entry, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("上课时间:"), 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), schedule_entry, 1, 3, 1, 1);
    gtk_container_add(GTK_CONTAINER(content), grid);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char err[ERROR_MSG_LEN];
        const gchar* name = gtk_entry_get_text(GTK_ENTRY(name_entry));
        const gchar* schedule = gtk_entry_get_text(GTK_ENTRY(schedule_entry));
        int capacity = atoi(gtk_entry_get_text(GTK_ENTRY(capacity_entry)));
        int credits = atoi(gtk_entry_get_text(GTK_ENTRY(credits_entry)));
        if (!service_add_course(g_state, name, capacity, credits, schedule, err,
                                sizeof(err)))
        {
            show_message(GTK_MESSAGE_WARNING, err);
        }
        else if (persist_state())
        {
            refresh_all_views();
        }
    }

    gtk_widget_destroy(dialog);
}

static void on_edit_course(GtkButton* btn, gpointer user_data)
{
    GtkTreeSelection* selection;
    GtkTreeModel* model;
    GtkTreeIter iter;
    gint course_id;
    int course_index;
    GtkWidget* dialog;
    GtkWidget* content;
    GtkWidget* entry;

    (void)btn;
    (void)user_data;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_state->widgets.course_table));
    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, 0, &course_id, -1);
    course_index = service_get_course_index(g_state, course_id);
    if (course_index < 0)
        return;

    dialog = gtk_dialog_new_with_buttons("修改课程名称",
                                         GTK_WINDOW(g_state->widgets.window),
                                         GTK_DIALOG_MODAL, "_取消",
                                         GTK_RESPONSE_CANCEL, "_确定",
                                         GTK_RESPONSE_ACCEPT, NULL);
    content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), g_state->courses[course_index].name);
    gtk_container_add(GTK_CONTAINER(content), entry);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char err[ERROR_MSG_LEN];
        if (!service_rename_course(g_state, course_id,
                                   gtk_entry_get_text(GTK_ENTRY(entry)), err,
                                   sizeof(err)))
        {
            show_message(GTK_MESSAGE_WARNING, err);
        }
        else if (persist_state())
        {
            refresh_all_views();
        }
    }

    gtk_widget_destroy(dialog);
}

static void on_delete_course(GtkButton* btn, gpointer user_data)
{
    GtkTreeSelection* selection;
    GtkTreeModel* model;
    GtkTreeIter iter;
    gint course_id;
    int course_index;
    GtkWidget* confirm;
    int response;

    (void)btn;
    (void)user_data;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_state->widgets.course_table));
    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, 0, &course_id, -1);
    course_index = service_get_course_index(g_state, course_id);
    if (course_index < 0)
        return;

    confirm = gtk_message_dialog_new(
        GTK_WINDOW(g_state->widgets.window), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING,
        GTK_BUTTONS_OK_CANCEL, "确定删除课程 %s ?", g_state->courses[course_index].name);
    response = gtk_dialog_run(GTK_DIALOG(confirm));
    gtk_widget_destroy(confirm);
    if (response == GTK_RESPONSE_OK)
    {
        char err[ERROR_MSG_LEN];
        if (!service_delete_course(g_state, course_id, err, sizeof(err)))
            show_message(GTK_MESSAGE_WARNING, err);
        else if (persist_state())
            refresh_all_views();
    }
}

static void on_add_student(GtkButton* btn, gpointer user_data)
{
    GtkWidget* dialog;
    GtkWidget* content;
    GtkWidget* grid;
    GtkWidget* name_entry;
    GtkWidget* id_entry;
    GtkWidget* gender_combo;

    (void)btn;
    (void)user_data;

    dialog = gtk_dialog_new_with_buttons("添加学生",
                                         GTK_WINDOW(g_state->widgets.window),
                                         GTK_DIALOG_MODAL, "_取消",
                                         GTK_RESPONSE_CANCEL, "_确定",
                                         GTK_RESPONSE_ACCEPT, NULL);
    content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    name_entry = gtk_entry_new();
    id_entry = gtk_entry_new();
    gender_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gender_combo), "男");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gender_combo), "女");
    gtk_combo_box_set_active(GTK_COMBO_BOX(gender_combo), 0);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("姓名:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), name_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("学号:"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), id_entry, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("性别:"), 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gender_combo, 1, 2, 1, 1);
    gtk_container_add(GTK_CONTAINER(content), grid);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char err[ERROR_MSG_LEN];
        gchar* gender =
            gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(gender_combo));
        int ok = service_add_student(g_state, gtk_entry_get_text(GTK_ENTRY(name_entry)),
                                     gtk_entry_get_text(GTK_ENTRY(id_entry)), gender,
                                     err, sizeof(err));
        if (gender)
            g_free(gender);
        if (!ok)
        {
            show_message(GTK_MESSAGE_WARNING, err);
        }
        else if (persist_state())
        {
            refresh_all_views();
        }
    }

    gtk_widget_destroy(dialog);
}

static void on_delete_student(GtkButton* btn, gpointer user_data)
{
    GtkTreeView* view = GTK_TREE_VIEW(
        g_object_get_data(G_OBJECT(g_state->widgets.notebook), "student_table_view"));
    GtkTreeSelection* selection;
    GtkTreeModel* model;
    GtkTreeIter iter;
    gchar* student_id = NULL;
    int student_index;
    GtkWidget* confirm;
    int response;

    (void)btn;
    (void)user_data;

    if (!view)
        return;

    selection = gtk_tree_view_get_selection(view);
    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, 2, &student_id, -1);
    if (!student_id)
        return;

    student_index = service_find_student_by_id(g_state, student_id);
    if (student_index < 0)
    {
        g_free(student_id);
        return;
    }

    confirm = gtk_message_dialog_new(
        GTK_WINDOW(g_state->widgets.window), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING,
        GTK_BUTTONS_OK_CANCEL, "确定删除学生 %s (%s) ?",
        g_state->students[student_index].name, g_state->students[student_index].student_id);
    response = gtk_dialog_run(GTK_DIALOG(confirm));
    gtk_widget_destroy(confirm);
    if (response == GTK_RESPONSE_OK)
    {
        char err[ERROR_MSG_LEN];
        if (!service_delete_student(g_state, student_index, err, sizeof(err)))
            show_message(GTK_MESSAGE_WARNING, err);
        else if (persist_state())
            refresh_all_views();
    }

    g_free(student_id);
}

static void on_edit_student(GtkButton* btn, gpointer user_data)
{
    GtkTreeView* view = GTK_TREE_VIEW(
        g_object_get_data(G_OBJECT(g_state->widgets.notebook), "student_table_view"));
    GtkTreeSelection* selection;
    GtkTreeModel* model;
    GtkTreeIter iter;
    gchar* original_id = NULL;
    int student_index;
    GtkWidget* dialog;
    GtkWidget* content;
    GtkWidget* grid;
    GtkWidget* name_entry;
    GtkWidget* id_entry;
    GtkWidget* gender_combo;

    (void)btn;
    (void)user_data;

    if (!view)
        return;
    selection = gtk_tree_view_get_selection(view);
    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;
    gtk_tree_model_get(model, &iter, 2, &original_id, -1);
    if (!original_id)
        return;

    student_index = service_find_student_by_id(g_state, original_id);
    if (student_index < 0)
    {
        g_free(original_id);
        return;
    }

    dialog = gtk_dialog_new_with_buttons("编辑学生信息",
                                         GTK_WINDOW(g_state->widgets.window),
                                         GTK_DIALOG_MODAL, "_取消",
                                         GTK_RESPONSE_CANCEL, "_确定",
                                         GTK_RESPONSE_ACCEPT, NULL);
    content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    name_entry = gtk_entry_new();
    id_entry = gtk_entry_new();
    gender_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gender_combo), "男");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gender_combo), "女");
    gtk_entry_set_text(GTK_ENTRY(name_entry), g_state->students[student_index].name);
    gtk_entry_set_text(GTK_ENTRY(id_entry), g_state->students[student_index].student_id);
    gtk_combo_box_set_active(
        GTK_COMBO_BOX(gender_combo),
        strcmp(g_state->students[student_index].gender, "女") == 0 ? 1 : 0);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("姓名:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), name_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("学号:"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), id_entry, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("性别:"), 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gender_combo, 1, 2, 1, 1);
    gtk_container_add(GTK_CONTAINER(content), grid);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char err[ERROR_MSG_LEN];
        gchar* gender =
            gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(gender_combo));
        int ok = service_update_student(
            g_state, student_index, gtk_entry_get_text(GTK_ENTRY(name_entry)),
            gtk_entry_get_text(GTK_ENTRY(id_entry)), gender, err, sizeof(err));
        if (gender)
            g_free(gender);
        if (!ok)
            show_message(GTK_MESSAGE_WARNING, err);
        else if (persist_state())
            refresh_all_views();
    }

    gtk_widget_destroy(dialog);
    g_free(original_id);
}

static void on_add_course(GtkButton* btn, gpointer user_data)
{
    GtkComboBox* combo = GTK_COMBO_BOX(g_object_get_data(
        G_OBJECT(g_state->widgets.selection_view), "student_combo"));
    GtkWidget* table = GTK_WIDGET(g_object_get_data(
        G_OBJECT(g_state->widgets.selection_view), "selection_table"));
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(table));
    GtkTreeModel* model;
    GtkTreeIter iter;
    gint course_id;
    gint student_index = gtk_combo_box_get_active(combo);
    char err[ERROR_MSG_LEN];

    (void)btn;
    (void)user_data;

    if (student_index < 0 || !gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, 0, &course_id, -1);
    if (!service_select_course(g_state, student_index, course_id, err, sizeof(err)))
        show_message(GTK_MESSAGE_WARNING, err);
    else if (persist_state())
        refresh_all_views();
}

static void on_drop_course(GtkButton* btn, gpointer user_data)
{
    GtkComboBox* combo = GTK_COMBO_BOX(g_object_get_data(
        G_OBJECT(g_state->widgets.selection_view), "student_combo"));
    GtkWidget* table = GTK_WIDGET(g_object_get_data(
        G_OBJECT(g_state->widgets.selection_view), "selection_table"));
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(table));
    GtkTreeModel* model;
    GtkTreeIter iter;
    gint course_id;
    gint student_index = gtk_combo_box_get_active(combo);
    char err[ERROR_MSG_LEN];

    (void)btn;
    (void)user_data;

    if (student_index < 0 || !gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, 0, &course_id, -1);
    if (!service_drop_course(g_state, student_index, course_id, err, sizeof(err)))
        show_message(GTK_MESSAGE_WARNING, err);
    else if (persist_state())
        refresh_all_views();
}

static void on_finish_selection(GtkButton* btn, gpointer user_data)
{
    GtkComboBox* combo = GTK_COMBO_BOX(g_object_get_data(
        G_OBJECT(g_state->widgets.selection_view), "student_combo"));
    gint student_index = gtk_combo_box_get_active(combo);
    char err[ERROR_MSG_LEN];

    (void)btn;
    (void)user_data;

    if (!service_finalize_selection(g_state, student_index, err, sizeof(err)))
        show_message(GTK_MESSAGE_WARNING, err);
    else if (persist_state())
        refresh_all_views();
}

static void on_student_combo_changed(GtkComboBox* combo, gpointer user_data)
{
    (void)combo;
    (void)user_data;
    refresh_student_info_label();
    refresh_grade_table();
}

static void on_grade_cell_edited(GtkCellRendererText* cell, gchar* path_string,
                                 gchar* new_text, gpointer user_data)
{
    GtkWidget* table = GTK_WIDGET(
        g_object_get_data(G_OBJECT(g_state->widgets.grade_view), "grade_table"));
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(table));
    GtkTreePath* path = gtk_tree_path_new_from_string(path_string);
    GtkTreeIter iter;
    gint course_id;
    gint student_index;
    char* end = NULL;
    long parsed;
    char err[ERROR_MSG_LEN];

    (void)cell;
    (void)user_data;

    if (!gtk_tree_model_get_iter(model, &iter, path))
    {
        gtk_tree_path_free(path);
        return;
    }

    gtk_tree_model_get(model, &iter, 0, &course_id, -1);
    student_index = gtk_combo_box_get_active(GTK_COMBO_BOX(
        g_object_get_data(G_OBJECT(g_state->widgets.grade_view), "student_combo")));
    if (student_index < 0)
    {
        gtk_tree_path_free(path);
        return;
    }

    parsed = strtol(new_text, &end, 10);
    if (!new_text[0] || !end || *end != '\0' || parsed < 0 || parsed > 100)
    {
        show_message(GTK_MESSAGE_WARNING, "成绩必须是0到100之间的整数");
        gtk_tree_path_free(path);
        return;
    }

    if (!service_update_score(g_state, student_index, course_id, (int)parsed, err,
                              sizeof(err)))
    {
        show_message(GTK_MESSAGE_WARNING, err);
    }
    else if (persist_state())
    {
        refresh_all_views();
        gtk_combo_box_set_active(GTK_COMBO_BOX(g_object_get_data(
                                     G_OBJECT(g_state->widgets.grade_view),
                                     "student_combo")),
                                 student_index);
    }

    gtk_tree_path_free(path);
}

static void on_sort_by_credits(GtkButton* btn, gpointer user_data)
{
    (void)btn;
    (void)user_data;
    refresh_stats_table(TRUE);
}

static void on_fail_stats(GtkButton* btn, gpointer user_data)
{
    GString* result = g_string_new("===== 不及格学生统计 =====\n\n");
    gboolean found_one = FALSE;
    gboolean found_three_or_more = FALSE;

    (void)btn;
    (void)user_data;

    g_string_append(result, "--- 1 门不及格 ---\n");
    for (int i = 0; i < g_state->student_count; i++)
    {
        const Student* student = &g_state->students[i];
        if (student->fail_count != 1)
            continue;
        g_string_append_printf(result, "  %s (%s) - 科目: ", student->name,
                               student->student_id);
        for (int j = 0; j < student->num_courses; j++)
        {
            int course_index =
                service_get_course_index(g_state, student->courses[j]);
            {
                int score = service_get_score(student, student->courses[j]);
                if (course_index >= 0 && score >= 0 && score < PASS_SCORE)
                {
                    g_string_append_printf(result, "%s(%d分) ",
                                           g_state->courses[course_index].name, score);
                }
            }
        }
        g_string_append(result, "\n");
        found_one = TRUE;
    }
    if (!found_one)
        g_string_append(result, "  (无)\n");

    g_string_append(result, "\n--- 3 门及以上不及格 ---\n");
    for (int i = 0; i < g_state->student_count; i++)
    {
        const Student* student = &g_state->students[i];
        if (student->fail_count < 3)
            continue;
        g_string_append_printf(result, "  %s (%s) - %d 门不及格\n", student->name,
                               student->student_id, student->fail_count);
        found_three_or_more = TRUE;
    }
    if (!found_three_or_more)
        g_string_append(result, "  (无)\n");

    show_message(GTK_MESSAGE_INFO, result->str);
    g_string_free(result, TRUE);
}

static void on_export_csv(GtkWidget* widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    if (!storage_export_students_csv(g_state, g_export_path))
    {
        show_message(GTK_MESSAGE_WARNING, "导出失败");
        return;
    }
    show_message(GTK_MESSAGE_INFO, "已导出CSV");
}

static GtkTreeViewColumn* add_text_column(GtkTreeView* view, const char* title,
                                          int col_id)
{
    GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes(
        title, gtk_cell_renderer_text_new(), "text", col_id, NULL);
    gtk_tree_view_append_column(view, column);
    return column;
}

static GtkTreeViewColumn* add_editable_column(GtkTreeView* view,
                                              const char* title, int col_id,
                                              GCallback on_edited)
{
    GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn* column;

    g_object_set(renderer, "editable", TRUE, NULL);
    g_signal_connect(renderer, "edited", on_edited, NULL);
    column = gtk_tree_view_column_new_with_attributes(title, renderer, "text",
                                                      col_id, NULL);
    gtk_tree_view_append_column(view, column);
    return column;
}

void create_ui(AppState* state, int argc, char* argv[], const char* courses_path,
               const char* students_path, const char* export_path)
{
    GtkWidget* vbox;
    GtkWidget* menubar;
    GtkWidget* file_menu;
    GtkWidget* file_submenu;
    GtkWidget* save_item;
    GtkWidget* exit_item;
    GtkWidget* export_item;

    g_state = state;
    g_courses_path = courses_path;
    g_students_path = students_path;
    g_export_path = export_path;
    gtk_init(&argc, &argv);
    g_state->widgets.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(g_state->widgets.window), "学生选课及学籍管理系统");
    gtk_window_set_default_size(GTK_WINDOW(g_state->widgets.window), 1000, 700);
    g_signal_connect(g_state->widgets.window, "destroy", G_CALLBACK(gtk_main_quit),
                     NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    menubar = gtk_menu_bar_new();
    file_menu = gtk_menu_item_new_with_label("文件");
    file_submenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_menu), file_submenu);
    save_item = gtk_menu_item_new_with_label("保存");
    export_item = gtk_menu_item_new_with_label("导出CSV");
    exit_item = gtk_menu_item_new_with_label("退出");
    gtk_menu_shell_append(GTK_MENU_SHELL(file_submenu), save_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_submenu), export_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_submenu), exit_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file_menu);
    g_signal_connect(save_item, "activate", G_CALLBACK(on_save_all), NULL);
    g_signal_connect(export_item, "activate", G_CALLBACK(on_export_csv), NULL);
    g_signal_connect(exit_item, "activate", G_CALLBACK(gtk_main_quit), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    g_state->widgets.notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(vbox), g_state->widgets.notebook, TRUE, TRUE, 0);

    {
        GtkWidget* view = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        GtkWidget* scroll;
        GtkWidget* btn_box;
        GtkWidget* add_btn;
        GtkWidget* edit_btn;
        GtkWidget* del_btn;
        GtkListStore* store;
        const char* titles[] = {"编号", "课程名称", "容量", "已选", "学分", "上课时间"};

        gtk_container_set_border_width(GTK_CONTAINER(view), 10);
        g_state->widgets.course_table = gtk_tree_view_new();
        store = gtk_list_store_new(6, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT,
                                   G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING);
        gtk_tree_view_set_model(GTK_TREE_VIEW(g_state->widgets.course_table),
                                GTK_TREE_MODEL(store));
        g_object_unref(store);
        for (int i = 0; i < 6; i++)
            add_text_column(GTK_TREE_VIEW(g_state->widgets.course_table), titles[i], i);
        scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_widget_set_vexpand(scroll, TRUE);
        gtk_container_add(GTK_CONTAINER(scroll), g_state->widgets.course_table);
        gtk_box_pack_start(GTK_BOX(view), scroll, TRUE, TRUE, 0);
        btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        add_btn = gtk_button_new_with_label("添加课程");
        edit_btn = gtk_button_new_with_label("修改名称");
        del_btn = gtk_button_new_with_label("删除课程");
        gtk_box_pack_start(GTK_BOX(btn_box), add_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(btn_box), edit_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(btn_box), del_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(view), btn_box, FALSE, FALSE, 5);
        gtk_notebook_append_page(GTK_NOTEBOOK(g_state->widgets.notebook), view,
                                 gtk_label_new("课程管理"));
        g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_course_to_system), NULL);
        g_signal_connect(edit_btn, "clicked", G_CALLBACK(on_edit_course), NULL);
        g_signal_connect(del_btn, "clicked", G_CALLBACK(on_delete_course), NULL);
    }

    {
        GtkWidget* view = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        GtkWidget* table = gtk_tree_view_new();
        GtkWidget* scroll;
        GtkWidget* btn_box;
        GtkWidget* add_btn;
        GtkWidget* edit_btn;
        GtkWidget* del_btn;
        GtkListStore* store =
            gtk_list_store_new(6, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING,
                               G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);
        const char* titles[] = {"序号", "姓名", "学号", "性别", "选课", "学分"};

        gtk_container_set_border_width(GTK_CONTAINER(view), 10);
        gtk_tree_view_set_model(GTK_TREE_VIEW(table), GTK_TREE_MODEL(store));
        g_object_unref(store);
        g_object_set_data(G_OBJECT(g_state->widgets.notebook), "student_table_view",
                          table);
        for (int i = 0; i < 6; i++)
            add_text_column(GTK_TREE_VIEW(table), titles[i], i);
        scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_widget_set_vexpand(scroll, TRUE);
        gtk_container_add(GTK_CONTAINER(scroll), table);
        gtk_box_pack_start(GTK_BOX(view), scroll, TRUE, TRUE, 0);
        btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        add_btn = gtk_button_new_with_label("添加学生");
        edit_btn = gtk_button_new_with_label("编辑学生");
        del_btn = gtk_button_new_with_label("删除学生");
        gtk_box_pack_start(GTK_BOX(btn_box), add_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(btn_box), edit_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(btn_box), del_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(view), btn_box, FALSE, FALSE, 5);
        g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_student), NULL);
        g_signal_connect(edit_btn, "clicked", G_CALLBACK(on_edit_student), NULL);
        g_signal_connect(del_btn, "clicked", G_CALLBACK(on_delete_student), NULL);
        gtk_notebook_append_page(GTK_NOTEBOOK(g_state->widgets.notebook), view,
                                 gtk_label_new("学生管理"));
    }

    {
        GtkWidget* view = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        GtkWidget* combo = gtk_combo_box_text_new();
        GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        GtkWidget* table = gtk_tree_view_new();
        GtkWidget* scroll;
        GtkWidget* info_label = gtk_label_new("待选课");
        GtkWidget* btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        GtkWidget* add_btn = gtk_button_new_with_label("添加");
        GtkWidget* drop_btn = gtk_button_new_with_label("退选");
        GtkWidget* finish_btn = gtk_button_new_with_label("完成");
        GtkListStore* store = gtk_list_store_new(
            6, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT,
            G_TYPE_STRING);
        const char* titles[] = {"编号", "课程名称", "容量", "已选", "学分", "上课时间"};

        gtk_container_set_border_width(GTK_CONTAINER(view), 10);
        g_state->widgets.selection_view = view;
        gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("学生:"), FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(view), hbox, FALSE, FALSE, 5);
        gtk_tree_view_set_model(GTK_TREE_VIEW(table), GTK_TREE_MODEL(store));
        g_object_unref(store);
        for (int i = 0; i < 6; i++)
            add_text_column(GTK_TREE_VIEW(table), titles[i], i);
        scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_widget_set_vexpand(scroll, TRUE);
        gtk_container_add(GTK_CONTAINER(scroll), table);
        gtk_box_pack_start(GTK_BOX(view), scroll, TRUE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(view), info_label, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(btn_box), add_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(btn_box), drop_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(btn_box), finish_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(view), btn_box, FALSE, FALSE, 5);
        g_object_set_data(G_OBJECT(view), "student_combo", combo);
        g_object_set_data(G_OBJECT(view), "selection_table", table);
        g_object_set_data(G_OBJECT(view), "info_label", info_label);
        gtk_notebook_append_page(GTK_NOTEBOOK(g_state->widgets.notebook), view,
                                 gtk_label_new("选课操作"));
        g_signal_connect(combo, "changed", G_CALLBACK(on_student_combo_changed), NULL);
        g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_course), NULL);
        g_signal_connect(drop_btn, "clicked", G_CALLBACK(on_drop_course), NULL);
        g_signal_connect(finish_btn, "clicked", G_CALLBACK(on_finish_selection), NULL);
    }

    {
        GtkWidget* view = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        GtkWidget* combo = gtk_combo_box_text_new();
        GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        GtkWidget* table = gtk_tree_view_new();
        GtkWidget* scroll;
        GtkWidget* gpa_label = gtk_label_new("GPA: 0.00");
        GtkWidget* save_btn = gtk_button_new_with_label("保存成绩");
        GtkListStore* store =
            gtk_list_store_new(4, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING,
                               G_TYPE_STRING);
        const char* titles[] = {"ID", "科目", "成绩", "状态"};

        gtk_container_set_border_width(GTK_CONTAINER(view), 10);
        g_state->widgets.grade_view = view;
        gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("学生:"), FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(view), hbox, FALSE, FALSE, 5);
        gtk_tree_view_set_model(GTK_TREE_VIEW(table), GTK_TREE_MODEL(store));
        g_object_unref(store);
        for (int i = 0; i < 4; i++)
        {
            if (i == 2)
                add_editable_column(GTK_TREE_VIEW(table), titles[i], i,
                                    G_CALLBACK(on_grade_cell_edited));
            else
                add_text_column(GTK_TREE_VIEW(table), titles[i], i);
        }
        scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_widget_set_vexpand(scroll, TRUE);
        gtk_container_add(GTK_CONTAINER(scroll), table);
        gtk_box_pack_start(GTK_BOX(view), scroll, TRUE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(view), gpa_label, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(view), save_btn, FALSE, FALSE, 5);
        g_object_set_data(G_OBJECT(view), "student_combo", combo);
        g_object_set_data(G_OBJECT(view), "grade_table", table);
        g_object_set_data(G_OBJECT(view), "gpa_label", gpa_label);
        gtk_notebook_append_page(GTK_NOTEBOOK(g_state->widgets.notebook), view,
                                 gtk_label_new("成绩录入"));
        g_signal_connect(combo, "changed", G_CALLBACK(on_student_combo_changed), NULL);
        g_signal_connect(save_btn, "clicked", G_CALLBACK(on_save_grades), NULL);
    }

    {
        GtkWidget* view = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        GtkWidget* sort_btn = gtk_button_new_with_label("排序");
        GtkWidget* fail_btn = gtk_button_new_with_label("不及格");
        GtkWidget* table = gtk_tree_view_new();
        GtkWidget* scroll;
        GtkListStore* store = gtk_list_store_new(
            7, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT,
            G_TYPE_INT, G_TYPE_INT);
        const char* titles[] = {"排名", "姓名", "学号", "性别", "选", "学分", "不及格"};

        gtk_container_set_border_width(GTK_CONTAINER(view), 10);
        g_state->widgets.stats_view = view;
        gtk_box_pack_start(GTK_BOX(hbox), sort_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), fail_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(view), hbox, FALSE, FALSE, 5);
        gtk_tree_view_set_model(GTK_TREE_VIEW(table), GTK_TREE_MODEL(store));
        g_object_unref(store);
        for (int i = 0; i < 7; i++)
            add_text_column(GTK_TREE_VIEW(table), titles[i], i);
        scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_widget_set_vexpand(scroll, TRUE);
        gtk_container_add(GTK_CONTAINER(scroll), table);
        gtk_box_pack_start(GTK_BOX(view), scroll, TRUE, TRUE, 0);
        g_object_set_data(G_OBJECT(view), "stats_table", table);
        gtk_notebook_append_page(GTK_NOTEBOOK(g_state->widgets.notebook), view,
                                 gtk_label_new("统计排序"));
        g_signal_connect(sort_btn, "clicked", G_CALLBACK(on_sort_by_credits), NULL);
        g_signal_connect(fail_btn, "clicked", G_CALLBACK(on_fail_stats), NULL);
    }

    gtk_container_add(GTK_CONTAINER(g_state->widgets.window), vbox);
    refresh_all_views();

    GtkWidget* selection_combo = GTK_WIDGET(g_object_get_data(
        G_OBJECT(g_state->widgets.selection_view), "student_combo"));
    GtkWidget* grade_combo = GTK_WIDGET(
        g_object_get_data(G_OBJECT(g_state->widgets.grade_view), "student_combo"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(selection_combo),
                             g_state->student_count > 0 ? 0 : -1);
    gtk_combo_box_set_active(GTK_COMBO_BOX(grade_combo),
                             g_state->student_count > 0 ? 0 : -1);
    refresh_student_info_label();
    refresh_grade_table();

    gtk_widget_show_all(g_state->widgets.window);
    gtk_main();
}
