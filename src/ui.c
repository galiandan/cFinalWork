#include "ui.h"

static AppWidgets widgets;

void refresh_course_table() {
    GtkTreeView* view = GTK_TREE_VIEW(widgets.course_table);
    GtkListStore* store = GTK_LIST_STORE(gtk_tree_view_get_model(view));
    gtk_list_store_clear(store);
    for (int i = 0; i < MAX_COURSES; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                          0, courses[i].id,
                          1, courses[i].name,
                          2, courses[i].capacity,
                          3, courses[i].enrolled,
                          4, courses[i].credits,
                          -1);
    }
}

void refresh_student_table() {
    GtkWidget* tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(widgets.notebook), 1);
    GtkWidget* student_table = GTK_WIDGET(g_object_get_data(G_OBJECT(tab), "student_table"));
    if (!student_table) return;
    
    GtkTreeView* view = GTK_TREE_VIEW(student_table);
    GtkListStore* store = GTK_LIST_STORE(gtk_tree_view_get_model(view));
    gtk_list_store_clear(store);
    for (int i = 0; i < student_count; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                          0, i + 1,
                          1, students[i].name,
                          2, students[i].student_id,
                          3, students[i].gender,
                          4, students[i].num_courses,
                          5, students[i].total_credits,
                          -1);
    }
}

void refresh_student_combo(GtkComboBoxText* combo) {
    gtk_combo_box_text_remove_all(combo);
    for (int i = 0; i < student_count; i++) {
        gchar* text = g_strdup_printf("%s (%s)", students[i].name, students[i].student_id);
        gtk_combo_box_text_append(combo, NULL, text);
        g_free(text);
    }
}

void refresh_selection_table() {
    GtkWidget* tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(widgets.notebook), 2);
    GtkWidget* selection_table = GTK_WIDGET(g_object_get_data(G_OBJECT(tab), "selection_table"));
    if (!selection_table) return;
    
    GtkTreeView* view = GTK_TREE_VIEW(selection_table);
    GtkListStore* store = GTK_LIST_STORE(gtk_tree_view_get_model(view));
    gtk_list_store_clear(store);
    for (int i = 0; i < MAX_COURSES; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                          0, courses[i].id,
                          1, courses[i].name,
                          2, courses[i].capacity,
                          3, courses[i].enrolled,
                          4, courses[i].credits,
                          -1);
    }
}

void refresh_student_info_label() {
    GtkWidget* tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(widgets.notebook), 2);
    GtkWidget* info_label = GTK_WIDGET(g_object_get_data(G_OBJECT(tab), "info_label"));
    if (!info_label) return;
    
    GtkComboBoxText* combo = GTK_COMBO_BOX_TEXT(g_object_get_data(G_OBJECT(tab), "student_combo"));
    gint active = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
    if (active < 0 || active >= student_count) {
        gtk_label_set_text(GTK_LABEL(info_label), "请选择学生");
        return;
    }
    
    Student* s = &students[active];
    GString* info = g_string_new("");
    g_string_append_printf(info, "学生: %s | 学号: %s | 性别: %s | 已选: %d 门 | 学分: %d | 已选: ",
                          s->name, s->student_id, s->gender, s->num_courses, s->total_credits);
    for (int i = 0; i < s->num_courses; i++) {
        int ci = get_course_index(s->courses[i]);
        if (ci >= 0) {
            g_string_append_printf(info, "%s ", courses[ci].name);
        }
    }
    gtk_label_set_text(GTK_LABEL(info_label), info->str);
    g_string_free(info, TRUE);
}

void refresh_grade_table() {
    GtkWidget* tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(widgets.notebook), 3);
    GtkWidget* grade_table = GTK_WIDGET(g_object_get_data(G_OBJECT(tab), "grade_table"));
    if (!grade_table) return;
    
    GtkComboBoxText* combo = GTK_COMBO_BOX_TEXT(g_object_get_data(G_OBJECT(tab), "student_combo"));
    gint active = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
    if (active < 0 || active >= student_count) return;
    
    Student* s = &students[active];
    GtkTreeView* view = GTK_TREE_VIEW(grade_table);
    GtkListStore* store = GTK_LIST_STORE(gtk_tree_view_get_model(view));
    gtk_list_store_clear(store);
    
    for (int i = 0; i < s->num_courses; i++) {
        int cid = s->courses[i];
        int ci = get_course_index(cid);
        if (ci < 0) continue;
        
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        
        gchar* score_text = g_strdup_printf("%d", s->scores[ci]);
        gchar* status_text;
        if (s->scores[ci] >= 0) {
            status_text = g_strdup(s->scores[ci] >= PASS_SCORE ? "及格" : "不及格");
        } else {
            status_text = g_strdup("未录入");
            g_free(score_text);
            score_text = g_strdup("");
        }
        
        gtk_list_store_set(store, &iter,
                          0, cid,
                          1, courses[ci].name,
                          2, score_text,
                          3, status_text,
                          -1);
        g_free(score_text);
        g_free(status_text);
    }
}

static void on_add_course_to_system(GtkButton* btn, gpointer user_data) {
    GtkWidget* dialog = gtk_dialog_new_with_buttons("添加课程",
                          GTK_WINDOW(widgets.window),
                          GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                          "_取消", GTK_RESPONSE_CANCEL,
                          "_确定", GTK_RESPONSE_ACCEPT,
                          NULL);
    
    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    
    GtkWidget* name_label = gtk_label_new("课程名称:");
    GtkWidget* name_entry = gtk_entry_new();
    GtkWidget* capacity_label = gtk_label_new("容量:");
    GtkWidget* capacity_entry = gtk_entry_new();
    GtkWidget* credits_label = gtk_label_new("学分:");
    GtkWidget* credits_entry = gtk_entry_new();
    
    gtk_grid_attach(GTK_GRID(grid), name_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), name_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), capacity_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), capacity_entry, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), credits_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), credits_entry, 1, 2, 1, 1);
    
    gtk_container_add(GTK_CONTAINER(content), grid);
    gtk_widget_show_all(dialog);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        const gchar* name = gtk_entry_get_text(GTK_ENTRY(name_entry));
        const gchar* capacity_str = gtk_entry_get_text(GTK_ENTRY(capacity_entry));
        const gchar* credits_str = gtk_entry_get_text(GTK_ENTRY(credits_entry));
        
        if (name && strlen(name) > 0 && capacity_str && credits_str) {
            int capacity = atoi(capacity_str);
            int credits = atoi(credits_str);
            
            if (capacity > 0 && credits > 0) {
                Course* c = NULL;
                for (int i = 0; i < MAX_COURSES; i++) {
                    if (courses[i].id == 0) {
                        c = &courses[i];
                        c->id = i + 1;
                        strncpy(c->name, name, MAX_CNAME_LEN - 1);
                        c->capacity = capacity;
                        c->credits = credits;
                        c->enrolled = 0;
                        break;
                    }
                }
                
                if (c) {
                    save_courses("courses.txt");
                    refresh_course_table();
                    refresh_selection_table();
                    
                    GtkWidget* success = gtk_message_dialog_new(GTK_WINDOW(widgets.window),
                                      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                      GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "课程添加成功!");
                    gtk_dialog_run(GTK_DIALOG(success));
                    gtk_widget_destroy(success);
                } else {
                    GtkWidget* err = gtk_message_dialog_new(GTK_WINDOW(widgets.window),
                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                  GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "课程数量已达上限!");
                    gtk_dialog_run(GTK_DIALOG(err));
                    gtk_widget_destroy(err);
                }
            }
        }
    }
    gtk_widget_destroy(dialog);
}

static void on_edit_course(GtkButton* btn, gpointer user_data) {
    GtkTreeView* view = GTK_TREE_VIEW(widgets.course_table);
    GtkTreeSelection* selection = gtk_tree_view_get_selection(view);
    GtkTreeModel* model = gtk_tree_view_get_model(view);
    GtkTreeIter iter;
    
    if (!gtk_tree_selection_get_selected(selection, NULL, &iter)) {
        GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(widgets.window),
                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                             GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "请先选择课程");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    gint id;
    gtk_tree_model_get(model, &iter, 0, &id, -1);
    int ci = get_course_index(id);
    if (ci < 0) return;
    
    GtkWidget* dialog = gtk_dialog_new_with_buttons("修改课程名称",
                          GTK_WINDOW(widgets.window),
                          GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                          "_取消", GTK_RESPONSE_CANCEL,
                          "_确定", GTK_RESPONSE_ACCEPT,
                          NULL);
    
    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), courses[ci].name);
    gtk_container_add(GTK_CONTAINER(content), entry);
    
    gtk_widget_show_all(dialog);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        const gchar* new_name = gtk_entry_get_text(GTK_ENTRY(entry));
        if (new_name && strlen(new_name) > 0) {
            strncpy(courses[ci].name, new_name, MAX_CNAME_LEN - 1);
            save_courses("courses.txt");
            refresh_course_table();
        }
    }
    gtk_widget_destroy(dialog);
}

static void on_refresh_course(GtkButton* btn, gpointer user_data) {
    load_courses("courses.txt");
    refresh_course_table();
}

static void on_add_student(GtkButton* btn, gpointer user_data) {
    if (student_count >= MAX_STUDENTS) {
        GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(widgets.window),
                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                             GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "学生人数已满!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    GtkWidget* dialog = gtk_dialog_new_with_buttons("添加学生",
                          GTK_WINDOW(widgets.window),
                          GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                          "_取消", GTK_RESPONSE_CANCEL,
                          "_确定", GTK_RESPONSE_ACCEPT,
                          NULL);
    
    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    
    GtkWidget* name_label = gtk_label_new("姓名:");
    GtkWidget* name_entry = gtk_entry_new();
    GtkWidget* id_label = gtk_label_new("学号:");
    GtkWidget* id_entry = gtk_entry_new();
    GtkWidget* gender_label = gtk_label_new("性别:");
    GtkWidget* gender_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gender_combo), "男");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gender_combo), "女");
    gtk_combo_box_set_active(GTK_COMBO_BOX(gender_combo), 0);
    
    gtk_grid_attach(GTK_GRID(grid), name_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), name_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), id_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), id_entry, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gender_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gender_combo, 1, 2, 1, 1);
    
    gtk_container_add(GTK_CONTAINER(content), grid);
    gtk_widget_show_all(dialog);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        const gchar* name = gtk_entry_get_text(GTK_ENTRY(name_entry));
        const gchar* id = gtk_entry_get_text(GTK_ENTRY(id_entry));
        gchar* gender = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(gender_combo));
        
        if (name && strlen(name) > 0 && id && strlen(id) > 0) {
            if (find_student_by_id(id) >= 0) {
                GtkWidget* err = gtk_message_dialog_new(GTK_WINDOW(widgets.window),
                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                  GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "学号已存在!");
                gtk_dialog_run(GTK_DIALOG(err));
                gtk_widget_destroy(err);
            } else {
                Student* s = &students[student_count++];
                strncpy(s->name, name, MAX_NAME_LEN - 1);
                strncpy(s->student_id, id, MAX_ID_LEN - 1);
                strncpy(s->gender, gender ? gender : "男", 5);
                memset(s->courses, -1, sizeof(s->courses));
                memset(s->scores, -1, sizeof(s->scores));
                s->num_courses = 0;
                s->total_credits = 0;
                s->fail_count = 0;
                
                gchar* msg = g_strdup_printf("学生 %s 添加成功!", name);
                GtkWidget* success = gtk_message_dialog_new(GTK_WINDOW(widgets.window),
                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                  GTK_MESSAGE_INFO, GTK_BUTTONS_OK, msg);
                gtk_dialog_run(GTK_DIALOG(success));
                gtk_widget_destroy(success);
                g_free(msg);
                
                refresh_student_table();
                
                GtkComboBoxText* sel_combo = GTK_COMBO_BOX_TEXT(g_object_get_data(G_OBJECT(widgets.selection_view), "student_combo"));
                if (sel_combo) {
                    refresh_student_combo(sel_combo);
                    if (gtk_combo_box_get_active(GTK_COMBO_BOX(sel_combo)) < 0)
                        gtk_combo_box_set_active(GTK_COMBO_BOX(sel_combo), 0);
                    refresh_student_info_label();
                }
                
                GtkComboBoxText* grade_combo = GTK_COMBO_BOX_TEXT(g_object_get_data(G_OBJECT(widgets.grade_view), "student_combo"));
                if (grade_combo) {
                    refresh_student_combo(grade_combo);
                    if (gtk_combo_box_get_active(GTK_COMBO_BOX(grade_combo)) < 0 && student_count > 0) {
                        gtk_combo_box_set_active(GTK_COMBO_BOX(grade_combo), 0);
                        refresh_grade_table();
                    }
                }
            }
        }
        if (gender) g_free(gender);
    }
    gtk_widget_destroy(dialog);
}

static void on_refresh_students(GtkButton* btn, gpointer user_data) {
    refresh_student_table();
}

static void on_refresh_student_combo_btn(GtkButton* btn, gpointer user_data) {
    GtkWidget* tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(widgets.notebook), 2);
    GtkComboBoxText* combo = GTK_COMBO_BOX_TEXT(g_object_get_data(G_OBJECT(tab), "student_combo"));
    refresh_student_combo(combo);
    refresh_student_info_label();
}

static void on_student_combo_changed(GtkComboBox* combo, gpointer user_data) {
    refresh_student_info_label();
}

static void on_add_course(GtkButton* btn, gpointer user_data) {
    GtkWidget* tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(widgets.notebook), 2);
    GtkComboBoxText* student_combo = GTK_COMBO_BOX_TEXT(g_object_get_data(G_OBJECT(tab), "student_combo"));
    GtkWidget* selection_table = GTK_WIDGET(g_object_get_data(G_OBJECT(tab), "selection_table"));
    
    gint s_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(student_combo));
    if (s_idx < 0) {
        GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(widgets.window),
                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                             GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "请先选择学生!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(selection_table));
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(selection_table));
    GtkTreeIter iter;
    
    if (!gtk_tree_selection_get_selected(selection, NULL, &iter)) {
        GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(widgets.window),
                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                             GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "请先选择课程!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    gint course_id;
    gtk_tree_model_get(model, &iter, 0, &course_id, -1);
    int ci = get_course_index(course_id);
    if (ci < 0) return;
    
    Student* s = &students[s_idx];
    if (s->num_courses >= MAX_SELECTS) {
        GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(widgets.window),
                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                             GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "选课已达上限(5门)!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    for (int i = 0; i < s->num_courses; i++) {
        if (s->courses[i] == course_id) {
            GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(widgets.window),
                               GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                               GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "已选该课程!");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return;
        }
    }
    
    if (courses[ci].enrolled >= courses[ci].capacity) {
        GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(widgets.window),
                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                             GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "课程已满!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    s->courses[s->num_courses++] = course_id;
    increase_enrolled(course_id);
    
    gchar* msg = g_strdup_printf("已选: %s", courses[ci].name);
    GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(widgets.window),
                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                         GTK_MESSAGE_INFO, GTK_BUTTONS_OK, msg);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_free(msg);
    
    refresh_selection_table();
    refresh_student_info_label();
}

static void on_drop_course(GtkButton* btn, gpointer user_data) {
    GtkWidget* tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(widgets.notebook), 2);
    GtkComboBoxText* student_combo = GTK_COMBO_BOX_TEXT(g_object_get_data(G_OBJECT(tab), "student_combo"));
    GtkWidget* selection_table = GTK_WIDGET(g_object_get_data(G_OBJECT(tab), "selection_table"));
    
    gint s_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(student_combo));
    if (s_idx < 0) {
        GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(widgets.window),
                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                             GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "请先选择学生!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    Student* s = &students[s_idx];
    if (s->num_courses <= 3) {
        GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(widgets.window),
                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                             GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "至少保留3门课程,不能退选!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(selection_table));
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(selection_table));
    GtkTreeIter iter;
    
    if (!gtk_tree_selection_get_selected(selection, NULL, &iter)) {
        GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(widgets.window),
                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                             GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "请选择要退选的课程!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    gint course_id;
    gtk_tree_model_get(model, &iter, 0, &course_id, -1);
    
    for (int i = 0; i < s->num_courses; i++) {
        if (s->courses[i] == course_id) {
            decrease_enrolled(course_id);
            for (int j = i; j < s->num_courses - 1; j++)
                s->courses[j] = s->courses[j + 1];
            s->num_courses--;
            s->total_credits = 0;
            
            GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(widgets.window),
                               GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                               GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "退选成功!");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            
            refresh_selection_table();
            refresh_student_info_label();
            return;
        }
    }
    
    GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(widgets.window),
                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                         GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "未选该课程!");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void on_finish_selection(GtkButton* btn, gpointer user_data) {
    GtkWidget* tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(widgets.notebook), 2);
    GtkComboBoxText* student_combo = GTK_COMBO_BOX_TEXT(g_object_get_data(G_OBJECT(tab), "student_combo"));
    
    gint s_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(student_combo));
    if (s_idx < 0) {
        GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(widgets.window),
                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                             GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "请先选择学生!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    Student* s = &students[s_idx];
    if (s->num_courses < 3) {
        gchar* msg = g_strdup_printf("至少需要3门课程,当前仅%d门!", s->num_courses);
        GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(widgets.window),
                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                             GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, msg);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_free(msg);
        return;
    }
    
    s->total_credits = 0;
    for (int j = 0; j < s->num_courses; j++) {
        int ci = get_course_index(s->courses[j]);
        if (ci >= 0) s->total_credits += courses[ci].credits;
    }
    
    save_courses("courses.txt");
    save_students("students.txt");
    
    gchar* msg = g_strdup_printf("选课完成! 共%d门, 总学分%d", s->num_courses, s->total_credits);
    GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(widgets.window),
                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                         GTK_MESSAGE_INFO, GTK_BUTTONS_OK, msg);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_free(msg);
    
    refresh_student_info_label();
}

static gchar grade_edit_buffer[16];
static gint grade_edit_row = -1;

static void on_grade_cell_editing_started(GtkCellRenderer* cell, GtkCellEditable* editable, gchar* path, gpointer user_data) {
    GtkWidget* grade_table = GTK_WIDGET(user_data);
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(grade_table));
    GtkTreePath* tree_path = gtk_tree_path_new_from_string(path);
    GtkTreeIter iter;
    gtk_tree_model_get_iter(model, &iter, tree_path);
    
    gchar* current_text;
    gtk_tree_model_get(model, &iter, 2, &current_text, -1);
    gtk_entry_set_text(GTK_ENTRY(editable), current_text);
    g_free(current_text);
    
    grade_edit_row = *gtk_tree_path_get_indices(tree_path);
    gtk_tree_path_free(tree_path);
}

static void on_grade_cell_edited(GtkCellRendererText* cell, gchar* path_string, gchar* new_text, gpointer user_data) {
    GtkWidget* grade_table = GTK_WIDGET(user_data);
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(grade_table));
    GtkTreePath* path = gtk_tree_path_new_from_string(path_string);
    GtkTreeIter iter;
    gtk_tree_model_get_iter(model, &iter, path);
    
    gint course_id;
    gtk_tree_model_get(model, &iter, 0, &course_id, -1);
    
    GtkWidget* tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(widgets.notebook), 3);
    GtkComboBoxText* combo = GTK_COMBO_BOX_TEXT(g_object_get_data(G_OBJECT(tab), "student_combo"));
    gint s_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
    
    if (s_idx >= 0 && s_idx < student_count) {
        Student* s = &students[s_idx];
        for (int i = 0; i < s->num_courses; i++) {
            if (s->courses[i] == course_id) {
                int score = atoi(new_text);
                if (score < 0) score = 0;
                if (score > 100) score = 100;
                
                int ci = get_course_index(course_id);
                if (ci >= 0) {
                    s->scores[ci] = score;
                    compute_fail_count(s_idx);
                    
                    gtk_list_store_set(GTK_LIST_STORE(model), &iter, 2, new_text, -1);
                    gchar* status = (score >= PASS_SCORE) ? "及格" : "不及格";
                    gtk_list_store_set(GTK_LIST_STORE(model), &iter, 3, status, -1);
                }
                break;
            }
        }
    }
    gtk_tree_path_free(path);
}

static void on_save_grades(GtkButton* btn, gpointer user_data) {
    GtkWidget* tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(widgets.notebook), 3);
    GtkComboBoxText* combo = GTK_COMBO_BOX_TEXT(g_object_get_data(G_OBJECT(tab), "student_combo"));
    gint s_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
    
    if (s_idx < 0) {
        GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(widgets.window),
                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                             GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "请先选择学生!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    Student* s = &students[s_idx];
    compute_fail_count(s_idx);
    
    save_students("students.txt");
    
    GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(widgets.window),
                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                         GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "成绩已保存!");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void grade_refresh_student_combo(GtkButton* btn, gpointer user_data) {
    GtkWidget* tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(widgets.notebook), 3);
    GtkComboBoxText* combo = GTK_COMBO_BOX_TEXT(g_object_get_data(G_OBJECT(tab), "student_combo"));
    refresh_student_combo(combo);
    if (student_count > 0) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
        refresh_grade_table();
    }
}

static void on_grade_combo_changed(GtkComboBox* combo, gpointer user_data) {
    refresh_grade_table();
}

static void refresh_stats_table(gboolean sorted) {
    GtkWidget* tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(widgets.notebook), 4);
    GtkWidget* stats_table = GTK_WIDGET(g_object_get_data(G_OBJECT(tab), "stats_table"));
    if (!stats_table) return;
    
    GtkTreeView* view = GTK_TREE_VIEW(stats_table);
    GtkListStore* store = GTK_LIST_STORE(gtk_tree_view_get_model(view));
    gtk_list_store_clear(store);
    
    Student sorted_students[MAX_STUDENTS];
    if (sorted) {
        sort_students(sorted_students, student_count);
        for (int i = 0; i < student_count; i++) {
            GtkTreeIter iter;
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter,
                              0, i + 1,
                              1, sorted_students[i].name,
                              2, sorted_students[i].student_id,
                              3, sorted_students[i].gender,
                              4, sorted_students[i].num_courses,
                              5, sorted_students[i].total_credits,
                              6, sorted_students[i].fail_count,
                              -1);
        }
    } else {
        for (int i = 0; i < student_count; i++) {
            GtkTreeIter iter;
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter,
                              0, i + 1,
                              1, students[i].name,
                              2, students[i].student_id,
                              3, students[i].gender,
                              4, students[i].num_courses,
                              5, students[i].total_credits,
                              6, students[i].fail_count,
                              -1);
        }
    }
}

static void on_sort_by_credits(GtkButton* btn, gpointer user_data) {
    refresh_stats_table(TRUE);
}

static void on_fail_stats(GtkButton* btn, gpointer user_data) {
    GString* result = g_string_new("===== 不及格学生统计 =====\n\n");
    
    g_string_append(result, "--- 1 门不及格 ---\n");
    gboolean found1 = FALSE;
    for (int i = 0; i < student_count; i++) {
        if (students[i].fail_count == 1) {
            g_string_append_printf(result, "  %s (%s) - 科目: ", students[i].name, students[i].student_id);
            for (int j = 0; j < students[i].num_courses; j++) {
                int ci = get_course_index(students[i].courses[j]);
                if (ci >= 0 && students[i].scores[ci] >= 0 && students[i].scores[ci] < PASS_SCORE) {
                    g_string_append_printf(result, "%s(%d分) ", courses[ci].name, students[i].scores[ci]);
                }
            }
            g_string_append(result, "\n");
            found1 = TRUE;
        }
    }
    if (!found1) g_string_append(result, "  (无)\n");
    
    g_string_append(result, "\n--- 3 门不及格 ---\n");
    gboolean found3 = FALSE;
    for (int i = 0; i < student_count; i++) {
        if (students[i].fail_count == 3) {
            g_string_append_printf(result, "  %s (%s)\n", students[i].name, students[i].student_id);
            found3 = TRUE;
        }
    }
    if (!found3) g_string_append(result, "  (无)\n");
    
    GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(widgets.window),
                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                         GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", result->str);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_string_free(result, TRUE);
}

static void on_stats_refresh(GtkButton* btn, gpointer user_data) {
    refresh_stats_table(FALSE);
}

static GtkWidget* create_course_view() {
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    GtkWidget* label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>课程列表</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    GtkWidget* scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scrolled, TRUE);

    widgets.course_table = gtk_tree_view_new();
    GtkListStore* store = gtk_list_store_new(5, G_TYPE_INT, G_TYPE_STRING, 
                                              G_TYPE_INT, G_TYPE_INT, G_TYPE_INT);
    gtk_tree_view_set_model(GTK_TREE_VIEW(widgets.course_table), GTK_TREE_MODEL(store));
    g_object_unref(store);

    const char* titles[] = {"编号", "课程名称", "容量", "已选", "学分"};
    for (int i = 0; i < 5; i++) {
        GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn* col = gtk_tree_view_column_new_with_attributes(titles[i], renderer, "text", i, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(widgets.course_table), col);
    }

    gtk_container_add(GTK_CONTAINER(scrolled), widgets.course_table);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    GtkWidget* add_btn = gtk_button_new_with_label("添加课程");
    GtkWidget* edit_btn = gtk_button_new_with_label("修改课程名称");
    GtkWidget* refresh_btn = gtk_button_new_with_label("刷新");
    gtk_box_pack_start(GTK_BOX(hbox), add_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), edit_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), refresh_btn, FALSE, FALSE, 0);

    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_course_to_system), NULL);
    g_signal_connect(edit_btn, "clicked", G_CALLBACK(on_edit_course), NULL);
    g_signal_connect(refresh_btn, "clicked", G_CALLBACK(on_refresh_course), NULL);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 10);

    return vbox;
}

static GtkWidget* create_student_view() {
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    GtkWidget* label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>学生列表</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    GtkWidget* scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scrolled, TRUE);

    GtkWidget* student_table = gtk_tree_view_new();
    GtkListStore* store = gtk_list_store_new(6, G_TYPE_INT, G_TYPE_STRING, 
                                              G_TYPE_STRING, G_TYPE_STRING, 
                                              G_TYPE_INT, G_TYPE_INT);
    gtk_tree_view_set_model(GTK_TREE_VIEW(student_table), GTK_TREE_MODEL(store));
    g_object_unref(store);

    const char* titles[] = {"序号", "姓名", "学号", "性别", "选课数", "总学分"};
    for (int i = 0; i < 6; i++) {
        GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn* col = gtk_tree_view_column_new_with_attributes(titles[i], renderer, "text", i, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(student_table), col);
    }

    gtk_container_add(GTK_CONTAINER(scrolled), student_table);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    GtkWidget* add_btn = gtk_button_new_with_label("添加学生");
    GtkWidget* refresh_btn = gtk_button_new_with_label("刷新");
    gtk_box_pack_start(GTK_BOX(hbox), add_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), refresh_btn, FALSE, FALSE, 0);

    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_student), NULL);
    g_signal_connect(refresh_btn, "clicked", G_CALLBACK(on_refresh_students), NULL);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 10);

    g_object_set_data(G_OBJECT(vbox), "student_table", student_table);
    return vbox;
}

static GtkWidget* create_selection_view() {
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("选择学生:"), FALSE, FALSE, 5);

    GtkWidget* student_combo = gtk_combo_box_text_new();
    gtk_combo_box_set_active(GTK_COMBO_BOX(student_combo), -1);
    gtk_widget_set_hexpand(student_combo, TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), student_combo, TRUE, TRUE, 0);

    GtkWidget* refresh_student_btn = gtk_button_new_with_label("刷新学生列表");
    gtk_box_pack_start(GTK_BOX(hbox), refresh_student_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

    g_signal_connect(refresh_student_btn, "clicked", G_CALLBACK(on_refresh_student_combo_btn), NULL);
    g_signal_connect(student_combo, "changed", G_CALLBACK(on_student_combo_changed), NULL);

    GtkWidget* scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scrolled, TRUE);

    GtkWidget* selection_table = gtk_tree_view_new();
    GtkListStore* store = gtk_list_store_new(5, G_TYPE_INT, G_TYPE_STRING, 
                                              G_TYPE_INT, G_TYPE_INT, G_TYPE_INT);
    gtk_tree_view_set_model(GTK_TREE_VIEW(selection_table), GTK_TREE_MODEL(store));
    g_object_unref(store);

    const char* titles[] = {"编号", "课程名称", "容量", "已选", "学分"};
    for (int i = 0; i < 5; i++) {
        GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn* col = gtk_tree_view_column_new_with_attributes(titles[i], renderer, "text", i, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(selection_table), col);
    }
    gtk_container_add(GTK_CONTAINER(scrolled), selection_table);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 5);

    GtkWidget* info_label = gtk_label_new("请选择学生");
    gtk_widget_set_halign(info_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), info_label, FALSE, FALSE, 5);

    GtkWidget* btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget* add_btn = gtk_button_new_with_label("添加课程");
    GtkWidget* drop_btn = gtk_button_new_with_label("退选课程");
    GtkWidget* finish_btn = gtk_button_new_with_label("完成选课");
    gtk_box_pack_start(GTK_BOX(btn_box), add_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btn_box), drop_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btn_box), finish_btn, FALSE, FALSE, 0);

    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_course), NULL);
    g_signal_connect(drop_btn, "clicked", G_CALLBACK(on_drop_course), NULL);
    g_signal_connect(finish_btn, "clicked", G_CALLBACK(on_finish_selection), NULL);

    gtk_box_pack_start(GTK_BOX(vbox), btn_box, FALSE, FALSE, 10);

    g_object_set_data(G_OBJECT(vbox), "student_combo", student_combo);
    g_object_set_data(G_OBJECT(vbox), "selection_table", selection_table);
    g_object_set_data(G_OBJECT(vbox), "info_label", info_label);

    return vbox;
}

static GtkWidget* create_grade_view() {
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("选择学生:"), FALSE, FALSE, 5);

    GtkWidget* student_combo = gtk_combo_box_text_new();
    gtk_box_pack_start(GTK_BOX(hbox), student_combo, TRUE, TRUE, 0);

    GtkWidget* refresh_btn = gtk_button_new_with_label("刷新");
    gtk_box_pack_start(GTK_BOX(hbox), refresh_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

    g_signal_connect(refresh_btn, "clicked", G_CALLBACK(grade_refresh_student_combo), NULL);
    g_signal_connect(student_combo, "changed", G_CALLBACK(on_grade_combo_changed), NULL);

    GtkWidget* scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scrolled, TRUE);

    GtkWidget* grade_table = gtk_tree_view_new();
    GtkListStore* store = gtk_list_store_new(4, G_TYPE_INT, G_TYPE_STRING, 
                                              G_TYPE_STRING, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(grade_table), GTK_TREE_MODEL(store));
    g_object_unref(store);

    const char* titles[] = {"课程编号", "课程名称", "成绩", "状态"};
    for (int i = 0; i < 4; i++) {
        GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
        if (i == 2) {
            g_object_set(renderer, "editable", TRUE, NULL);
            g_signal_connect(renderer, "editing-started", G_CALLBACK(on_grade_cell_editing_started), grade_table);
            g_signal_connect(renderer, "edited", G_CALLBACK(on_grade_cell_edited), grade_table);
        }
        GtkTreeViewColumn* col = gtk_tree_view_column_new_with_attributes(titles[i], renderer, "text", i, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(grade_table), col);
    }

    gtk_container_add(GTK_CONTAINER(scrolled), grade_table);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

    GtkWidget* save_btn = gtk_button_new_with_label("保存成绩");
    gtk_box_pack_start(GTK_BOX(vbox), save_btn, FALSE, FALSE, 10);
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_save_grades), NULL);

    g_object_set_data(G_OBJECT(vbox), "student_combo", student_combo);
    g_object_set_data(G_OBJECT(vbox), "grade_table", grade_table);

    return vbox;
}

static GtkWidget* create_stats_view() {
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget* sort_btn = gtk_button_new_with_label("按学分排序");
    GtkWidget* fail_btn = gtk_button_new_with_label("不及格统计");
    GtkWidget* refresh_btn = gtk_button_new_with_label("刷新");
    gtk_box_pack_start(GTK_BOX(hbox), sort_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), fail_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), refresh_btn, FALSE, FALSE, 0);

    g_signal_connect(sort_btn, "clicked", G_CALLBACK(on_sort_by_credits), NULL);
    g_signal_connect(fail_btn, "clicked", G_CALLBACK(on_fail_stats), NULL);
    g_signal_connect(refresh_btn, "clicked", G_CALLBACK(on_stats_refresh), NULL);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

    GtkWidget* scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scrolled, TRUE);

    GtkWidget* stats_table = gtk_tree_view_new();
    GtkListStore* store = gtk_list_store_new(7, G_TYPE_INT, G_TYPE_STRING, 
                                              G_TYPE_STRING, G_TYPE_STRING,
                                              G_TYPE_INT, G_TYPE_INT, G_TYPE_INT);
    gtk_tree_view_set_model(GTK_TREE_VIEW(stats_table), GTK_TREE_MODEL(store));
    g_object_unref(store);

    const char* titles[] = {"排名", "姓名", "学号", "性别", "选课数", "总学分", "不及格门数"};
    for (int i = 0; i < 7; i++) {
        GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn* col = gtk_tree_view_column_new_with_attributes(titles[i], renderer, "text", i, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(stats_table), col);
    }

    gtk_container_add(GTK_CONTAINER(scrolled), stats_table);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

    g_object_set_data(G_OBJECT(vbox), "stats_table", stats_table);

    return vbox;
}

void create_ui(int argc, char* argv[]) {
    gtk_init(&argc, &argv);

    widgets.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(widgets.window), "学生选课及学籍管理系统");
    gtk_window_set_default_size(GTK_WINDOW(widgets.window), 1000, 700);
    g_signal_connect(widgets.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget* main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    GtkMenuBar* menubar = GTK_MENU_BAR(gtk_menu_bar_new());
    GtkWidget* file_menu = gtk_menu_item_new_with_label("文件");
    GtkWidget* file_submenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_menu), file_submenu);

    GtkWidget* save_all_item = gtk_menu_item_new_with_label("保存所有数据");
    GtkWidget* sep = gtk_separator_menu_item_new();
    GtkWidget* exit_item = gtk_menu_item_new_with_label("退出");

    gtk_menu_shell_append(GTK_MENU_SHELL(file_submenu), save_all_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_submenu), sep);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_submenu), exit_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), GTK_WIDGET(file_menu));

    g_signal_connect_swapped(save_all_item, "activate", G_CALLBACK(gtk_widget_activate), widgets.window);
    g_signal_connect(exit_item, "activate", G_CALLBACK(gtk_main_quit), NULL);

    gtk_box_pack_start(GTK_BOX(main_vbox), GTK_WIDGET(menubar), FALSE, FALSE, 0);

    widgets.notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(main_vbox), widgets.notebook, TRUE, TRUE, 0);

    widgets.course_view = create_course_view();
    widgets.student_view = create_student_view();
    widgets.selection_view = create_selection_view();
    widgets.grade_view = create_grade_view();
    widgets.stats_view = create_stats_view();

    gtk_notebook_append_page(GTK_NOTEBOOK(widgets.notebook), widgets.course_view, gtk_label_new("课程管理"));
    gtk_notebook_append_page(GTK_NOTEBOOK(widgets.notebook), widgets.student_view, gtk_label_new("学生管理"));
    gtk_notebook_append_page(GTK_NOTEBOOK(widgets.notebook), widgets.selection_view, gtk_label_new("选课操作"));
    gtk_notebook_append_page(GTK_NOTEBOOK(widgets.notebook), widgets.grade_view, gtk_label_new("成绩录入"));
    gtk_notebook_append_page(GTK_NOTEBOOK(widgets.notebook), widgets.stats_view, gtk_label_new("统计排序"));

    refresh_course_table();
    refresh_student_table();
    refresh_selection_table();
    refresh_stats_table(FALSE);

    GtkWidget* selection_combo = GTK_WIDGET(g_object_get_data(G_OBJECT(widgets.selection_view), "student_combo"));
    if (selection_combo) {
        refresh_student_combo(GTK_COMBO_BOX_TEXT(selection_combo));
        if (student_count > 0) {
            gtk_combo_box_set_active(GTK_COMBO_BOX(selection_combo), 0);
            refresh_student_info_label();
        }
    }

    GtkWidget* grade_combo = GTK_WIDGET(g_object_get_data(G_OBJECT(widgets.grade_view), "student_combo"));
    if (grade_combo) {
        refresh_student_combo(GTK_COMBO_BOX_TEXT(grade_combo));
        if (student_count > 0) {
            gtk_combo_box_set_active(GTK_COMBO_BOX(grade_combo), 0);
            refresh_grade_table();
        }
    }

    gtk_container_add(GTK_CONTAINER(widgets.window), main_vbox);

    gtk_widget_show_all(widgets.window);
    gtk_main();
}
