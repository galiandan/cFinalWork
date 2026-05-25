#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME_LEN 20
#define MAX_ID_LEN 20
#define MAX_CNAME_LEN 50
#define MAX_COURSES 7
#define MAX_STUDENTS 200
#define MAX_SELECTS 5
#define PASS_SCORE 60
#define MAX_SCHEDULE_LEN 30

typedef struct
{
    int id;
    char name[MAX_CNAME_LEN];
    int capacity;
    int enrolled;
    int credits;
    char schedule[MAX_SCHEDULE_LEN];
} Course;

typedef struct
{
    char name[MAX_NAME_LEN];
    char student_id[MAX_ID_LEN];
    char gender[6];
    int courses[MAX_SELECTS];
    int num_courses;
    int scores[MAX_COURSES];
    int total_credits;
    int fail_count;
    double gpa;
    int gpa_warning;
    int gpa_computed;
} Student;

typedef struct
{
    GtkWidget* window;
    GtkWidget* notebook;
    GtkWidget* course_table;
    GtkWidget* selection_view;
    GtkWidget* grade_view;
    GtkWidget* stats_view;
} AppWidgets;

typedef struct
{
    Course courses[MAX_COURSES];
    Student students[MAX_STUDENTS];
    int student_count;
    int max_course_id;
    int course_index_valid;
    int course_id_to_index[MAX_COURSES];
    double gpa_cache[MAX_STUDENTS];
    int gpa_cache_valid[MAX_STUDENTS];
    AppWidgets g_widgets;
} AppState;

static AppState app_state;

static GtkTreeViewColumn* add_text_column(GtkTreeView* view, const char* title,
                                          int col_id);
static GtkTreeViewColumn* add_editable_column(GtkTreeView* view,
                                              const char* title, int col_id,
                                              GCallback on_edited);

void rebuild_course_index(void);
void refresh_all_views(void);
void refresh_course_table(void);
void refresh_student_table(void);
void refresh_student_combo(GtkComboBoxText* combo);
void refresh_selection_table(void);
void refresh_student_info_label(void);
void refresh_grade_table(void);
void refresh_stats_table(gboolean sorted);
void refresh_all_combos(void);
void create_ui(int argc, char* argv[]);

void on_save_all(GtkWidget* widget, gpointer user_data);
void on_save_grades(GtkButton* btn, gpointer user_data);
void on_add_course_to_system(GtkButton* btn, gpointer user_data);
void on_edit_course(GtkButton* btn, gpointer user_data);
void on_add_student(GtkButton* btn, gpointer user_data);
void on_delete_student(GtkButton* btn, gpointer user_data);
void on_edit_student(GtkButton* btn, gpointer user_data);
void on_add_course(GtkButton* btn, gpointer user_data);
void on_drop_course(GtkButton* btn, gpointer user_data);
void on_finish_selection(GtkButton* btn, gpointer user_data);
void on_student_combo_changed(GtkComboBox* combo, gpointer user_data);
void on_grade_cell_edited(GtkCellRendererText* cell, gchar* path_string,
                          gchar* new_text, gpointer user_data);
void on_sort_by_credits(GtkButton* btn, gpointer user_data);
void on_fail_stats(GtkButton* btn, gpointer user_data);
void on_export_csv(GtkWidget* widget, gpointer user_data);

void init_courses(void)
{
    const char* names[] = {"数据结构", "操作系统", "计算机网络", "数据库原理",
                           "软件工程", "人工智能", "编译原理"};
    const char* schedules[] = {"周一 1-2节", "周二 3-4节", "周三 5-6节",
                               "周一 3-4节", "周二 1-2节", "周三 1-2节",
                               "周四 5-6节"};
    app_state.max_course_id = 0;
    for (int i = 0; i < MAX_COURSES; i++)
    {
        app_state.courses[i].id = i + 1;
        strncpy(app_state.courses[i].name, names[i], MAX_CNAME_LEN - 1);
        app_state.courses[i].name[MAX_CNAME_LEN - 1] = '\0';
        app_state.courses[i].capacity = 10 * (i + 1);
        app_state.courses[i].credits = i + 1;
        app_state.courses[i].enrolled = 0;
        strncpy(app_state.courses[i].schedule, schedules[i], MAX_SCHEDULE_LEN - 1);
        app_state.courses[i].schedule[MAX_SCHEDULE_LEN - 1] = '\0';
        if (app_state.courses[i].id > app_state.max_course_id)
            app_state.max_course_id = app_state.courses[i].id;
    }
    rebuild_course_index();
}

void rebuild_course_index(void)
{
    for (int i = 0; i < MAX_COURSES; i++)
    {
        app_state.course_id_to_index[i] = -1;
    }
    for (int i = 0; i < MAX_COURSES; i++)
    {
        if (app_state.courses[i].id > 0 && app_state.courses[i].id <= MAX_COURSES)
        {
            app_state.course_id_to_index[app_state.courses[i].id - 1] = i;
        }
    }
    app_state.course_index_valid = 1;
}

int load_courses(const char* filename)
{
    FILE* fp = fopen(filename, "r");
    if (!fp)
    {
        init_courses();
        return 0;
    }
    int count = 0;
    app_state.max_course_id = 0;
    char buf[256];
    if (!fgets(buf, sizeof(buf), fp))
    {
        fclose(fp);
        init_courses();
        return 0;
    }
    int expected = atoi(buf);
    if (expected <= 0 || expected > MAX_COURSES)
    {
        fclose(fp);
        init_courses();
        return 0;
    }
    for (int i = 0; i < expected && i < MAX_COURSES && fgets(buf, sizeof(buf), fp); i++)
    {
        app_state.courses[i].id = 0;
        app_state.courses[i].name[0] = '\0';
        app_state.courses[i].capacity = 0;
        app_state.courses[i].enrolled = 0;
        app_state.courses[i].credits = 0;
        app_state.courses[i].schedule[0] = '\0';
        
        char* t = strtok(buf, "|\n");
        if (t)
        {
            int id_val = atoi(t);
            if (id_val > 0 && id_val <= MAX_COURSES)
            {
                app_state.courses[i].id = id_val;
            }
        }
        t = strtok(NULL, "|\n");
        if (t)
        {
            size_t len = strlen(t);
            if (len > MAX_CNAME_LEN - 1)
                len = MAX_CNAME_LEN - 1;
            strncpy(app_state.courses[i].name, t, len);
            app_state.courses[i].name[len] = '\0';
        }
        t = strtok(NULL, "|\n");
        if (t)
        {
            int cap = atoi(t);
            if (cap >= 0)
                app_state.courses[i].capacity = cap;
        }
        t = strtok(NULL, "|\n");
        if (t)
        {
            int enrolled_val = atoi(t);
            if (enrolled_val >= 0)
                app_state.courses[i].enrolled = enrolled_val;
        }
        t = strtok(NULL, "|\n");
        if (t)
        {
            int cred = atoi(t);
            if (cred > 0)
                app_state.courses[i].credits = cred;
        }
        t = strtok(NULL, "|\n");
        if (t)
        {
            size_t len = strlen(t);
            if (len > MAX_SCHEDULE_LEN - 1)
                len = MAX_SCHEDULE_LEN - 1;
            strncpy(app_state.courses[i].schedule, t, len);
            app_state.courses[i].schedule[len] = '\0';
        }
        if (app_state.courses[i].id > 0)
        {
            if (app_state.courses[i].id > app_state.max_course_id)
                app_state.max_course_id = app_state.courses[i].id;
            count++;
        }
    }
    fclose(fp);
    rebuild_course_index();
    return (count == expected) ? 1 : 0;
}

void save_courses(const char* filename)
{
    FILE* fp = fopen(filename, "w");
    if (!fp)
        return;
    fprintf(fp, "%d\n", MAX_COURSES);
    for (int i = 0; i < MAX_COURSES; i++)
        fprintf(fp, "%d|%s|%d|%d|%d|%s\n", app_state.courses[i].id, app_state.courses[i].name,
                app_state.courses[i].capacity, app_state.courses[i].enrolled, app_state.courses[i].credits,
                app_state.courses[i].schedule);
    fclose(fp);
}

int load_students(const char* filename)
{
    FILE* fp = fopen(filename, "r");
    if (!fp)
        return 0;
    char buf[1024];
    if (!fgets(buf, sizeof(buf), fp))
    {
        fclose(fp);
        return 0;
    }
    int count = atoi(buf);
    if (count <= 0 || count > MAX_STUDENTS)
    {
        fclose(fp);
        return 0;
    }
    for (int i = 0; i < count; i++)
    {
        app_state.students[i].name[0] = '\0';
        app_state.students[i].student_id[0] = '\0';
        app_state.students[i].gender[0] = '\0';
        app_state.students[i].num_courses = 0;
        app_state.students[i].total_credits = 0;
        app_state.students[i].fail_count = 0;
        app_state.students[i].gpa = 0.0;
        app_state.students[i].gpa_warning = 0;
        app_state.students[i].gpa_computed = 0;
        for (int j = 0; j < MAX_COURSES; j++)
            app_state.students[i].scores[j] = -1;
        for (int j = 0; j < MAX_SELECTS; j++)
            app_state.students[i].courses[j] = -1;
    }
    app_state.student_count = 0;

    for (int i = 0; i < count; i++)
    {
        if (!fgets(buf, sizeof(buf), fp))
            break;
        char* t = strtok(buf, "|\n");
        if (t)
        {
            size_t len = strlen(t);
            if (len > MAX_NAME_LEN - 1)
                len = MAX_NAME_LEN - 1;
            strncpy(app_state.students[i].name, t, len);
            app_state.students[i].name[len] = '\0';
        }
        t = strtok(NULL, "|\n");
        if (t)
        {
            size_t len = strlen(t);
            if (len > MAX_ID_LEN - 1)
                len = MAX_ID_LEN - 1;
            strncpy(app_state.students[i].student_id, t, len);
            app_state.students[i].student_id[len] = '\0';
        }
        t = strtok(NULL, "|\n");
        if (t)
        {
            size_t len = strlen(t);
            if (len > 5)
                len = 5;
            strncpy(app_state.students[i].gender, t, len);
            app_state.students[i].gender[len] = '\0';
        }
        t = strtok(NULL, "|\n");
        if (t)
        {
            int num = atoi(t);
            if (num >= 0 && num <= MAX_SELECTS)
                app_state.students[i].num_courses = num;
            else
                app_state.students[i].num_courses = 0;
        }
        for (int j = 0; j < MAX_SELECTS && (t = strtok(NULL, "|\n")); j++)
        {
            int cid = atoi(t);
            if (cid >= 1 && cid <= MAX_COURSES)
                app_state.students[i].courses[j] = cid;
            else
                app_state.students[i].courses[j] = -1;
        }
        for (int j = 0; j < MAX_COURSES && (t = strtok(NULL, "|\n")); j++)
        {
            int score = atoi(t);
            if (score >= 0 && score <= 100)
                app_state.students[i].scores[j] = score;
            else
                app_state.students[i].scores[j] = -1;
        }
        t = strtok(NULL, "|\n");
        if (t)
        {
            int creds = atoi(t);
            if (creds >= 0)
                app_state.students[i].total_credits = creds;
        }
        t = strtok(NULL, "|\n");
        if (t)
        {
            int fail = atoi(t);
            if (fail >= 0)
                app_state.students[i].fail_count = fail;
        }
        app_state.students[i].gpa_computed = 0;
        app_state.student_count++;
    }
    fclose(fp);
    return 1;
}

void save_students(const char* filename)
{
    FILE* fp = fopen(filename, "w");
    if (!fp)
        return;
    fprintf(fp, "%d\n", app_state.student_count);
    for (int i = 0; i < app_state.student_count; i++)
    {
        Student* s = &app_state.students[i];
        fprintf(fp, "%s|%s|%s|%d", s->name, s->student_id, s->gender,
                s->num_courses);
        for (int j = 0; j < MAX_SELECTS; j++)
            if (j < s->num_courses)
                fprintf(fp, "|%d", s->courses[j]);
            else
                fprintf(fp, "|-1");
        for (int j = 0; j < MAX_COURSES; j++)
            fprintf(fp, "|%d", s->scores[j]);
        fprintf(fp, "|%d|%d\n", s->total_credits, s->fail_count);
    }
    fclose(fp);
}

int find_student_by_id(const char* id)
{
    if (!id)
        return -1;
    for (int i = 0; i < app_state.student_count; i++)
        if (strcmp(app_state.students[i].student_id, id) == 0)
            return i;
    return -1;
}

int get_course_index(int cid)
{
    if (cid < 1 || cid > MAX_COURSES)
        return -1;
    int idx = cid - 1;
    if (app_state.course_index_valid && app_state.course_id_to_index[idx] >= 0)
        return app_state.course_id_to_index[idx];
    for (int i = 0; i < MAX_COURSES; i++)
        if (app_state.courses[i].id == cid)
            return i;
    return -1;
}

void increase_enrolled(int cid)
{
    int idx = get_course_index(cid);
    if (idx >= 0)
        app_state.courses[idx].enrolled++;
}

void decrease_enrolled(int cid)
{
    int idx = get_course_index(cid);
    if (idx >= 0)
        app_state.courses[idx].enrolled--;
}

void compute_fail_count(int idx)
{
    if (idx < 0 || idx >= app_state.student_count)
        return;
    Student* s = &app_state.students[idx];
    s->fail_count = 0;
    s->gpa_computed = 0;
    for (int j = 0; j < s->num_courses; j++)
    {
        int ci = get_course_index(s->courses[j]);
        if (ci >= 0 && s->scores[ci] >= 0 && s->scores[ci] < PASS_SCORE)
            s->fail_count++;
    }
}

void sort_students(Student* sorted, int count)
{
    memcpy(sorted, app_state.students, count * sizeof(Student));
    for (int i = 0; i < count - 1; i++)
    {
        for (int j = 0; j < count - i - 1; j++)
        {
            if (sorted[j].total_credits > sorted[j + 1].total_credits ||
                (sorted[j].total_credits == sorted[j + 1].total_credits &&
                 strcmp(sorted[j].name, sorted[j + 1].name) > 0))
                {
                    Student temp = sorted[j];
                    sorted[j] = sorted[j + 1];
                    sorted[j + 1] = temp;
                }
        }
    }
}

double score_to_points(int score)
{
    if (score >= 90) return 4.0;
    if (score >= 85) return 3.7;
    if (score >= 82) return 3.3;
    if (score >= 78) return 3.0;
    if (score >= 75) return 2.7;
    if (score >= 72) return 2.3;
    if (score >= 68) return 2.0;
    if (score >= 64) return 1.5;
    if (score >= 60) return 1.0;
    return 0.0;
}

void compute_gpa(int idx)
{
    if (idx < 0 || idx >= app_state.student_count)
        return;
    Student* s = &app_state.students[idx];
    if (s->gpa_computed)
    {
        s->gpa = app_state.gpa_cache[idx];
        s->gpa_warning = (s->gpa < 2.0) ? 1 : 0;
        return;
    }
    double total_points = 0.0;
    int total_credits = 0;
    for (int j = 0; j < s->num_courses; j++)
    {
        int ci = get_course_index(s->courses[j]);
        if (ci >= 0 && s->scores[ci] >= 0)
        {
            total_points += score_to_points(s->scores[ci]) * app_state.courses[ci].credits;
            total_credits += app_state.courses[ci].credits;
        }
    }
    s->gpa = (total_credits > 0) ? total_points / total_credits : 0.0;
    s->gpa_warning = (s->gpa < 2.0) ? 1 : 0;
    app_state.gpa_cache[idx] = s->gpa;
    app_state.gpa_cache_valid[idx] = 1;
    s->gpa_computed = 1;
}

void parse_schedule_day(const char* schedule, int* day)
{
    const char* days[] = {"周一", "周二", "周三", "周四", "周五", "周六", "周日"};
    *day = -1;
    for (int i = 0; i < 7; i++)
    {
        if (strstr(schedule, days[i]) != NULL)
        {
            *day = i;
            break;
        }
    }
}

void parse_schedule_slots(const char* schedule, int* start, int* end)
{
    const char* p = strstr(schedule, "节");
    if (!p) return;
    const char* q = p - 1;
    while (q > schedule && *q >= '0' && *q <= '9') q--;
    if (q == p - 1) return;
    q++;
    const char* dash = strchr(q, '-');
    if (dash)
    {
        *start = atoi(q);
        *end = atoi(dash + 1);
    }
    else
    {
        *start = atoi(q);
        *end = *start;
    }
}

int check_time_conflict(int cid1, int cid2)
{
    int idx1 = get_course_index(cid1);
    int idx2 = get_course_index(cid2);
    if (idx1 < 0 || idx2 < 0)
        return 0;
    if (strlen(app_state.courses[idx1].schedule) == 0 || strlen(app_state.courses[idx2].schedule) == 0)
        return 0;
    int d1 = -1, d2 = -1;
    int s1 = -1, e1 = -1, s2 = -1, e2 = -1;
    parse_schedule_day(app_state.courses[idx1].schedule, &d1);
    parse_schedule_day(app_state.courses[idx2].schedule, &d2);
    if (d1 != d2) return 0;
    parse_schedule_slots(app_state.courses[idx1].schedule, &s1, &e1);
    parse_schedule_slots(app_state.courses[idx2].schedule, &s2, &e2);
    return (s1 <= e2 && s2 <= e1);
}

void refresh_course_table(void)
{
    if (!app_state.g_widgets.course_table)
        return;
    GtkListStore* store = GTK_LIST_STORE(
        gtk_tree_view_get_model(GTK_TREE_VIEW(app_state.g_widgets.course_table)));
    gtk_list_store_clear(store);
    for (int i = 0; i < MAX_COURSES; i++)
    {
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, app_state.courses[i].id, 1, app_state.courses[i].name,
                           2, app_state.courses[i].capacity, 3, app_state.courses[i].enrolled, 4,
                           app_state.courses[i].credits, 5, app_state.courses[i].schedule, -1);
    }
}

void refresh_student_table(void)
{
    GtkTreeView* view = GTK_TREE_VIEW(
        g_object_get_data(G_OBJECT(app_state.g_widgets.notebook), "student_table_view"));
    if (!view)
        return;
    GtkListStore* store = GTK_LIST_STORE(gtk_tree_view_get_model(view));
    gtk_list_store_clear(store);
    for (int i = 0; i < app_state.student_count; i++)
    {
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, i + 1, 1, app_state.students[i].name, 2,
                           app_state.students[i].student_id, 3, app_state.students[i].gender, 4,
                           app_state.students[i].num_courses, 5,
                           app_state.students[i].total_credits, -1);
    }
}

void refresh_student_combo(GtkComboBoxText* combo)
{
    if (!combo)
        return;
    gint active = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
    if (active == -1 && app_state.student_count > 0)
        active = 0;
    gtk_combo_box_text_remove_all(combo);
    for (int i = 0; i < app_state.student_count; i++)
    {
        gchar* text = g_strdup_printf("%s (%s)", app_state.students[i].name,
                                      app_state.students[i].student_id);
        gtk_combo_box_text_append(combo, NULL, text);
        g_free(text);
    }
    if (active >= app_state.student_count)
        active = app_state.student_count - 1;
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), active);
}

void refresh_selection_table(void)
{
    GtkTreeView* view = GTK_TREE_VIEW(g_object_get_data(
        G_OBJECT(app_state.g_widgets.selection_view), "selection_table"));
    if (!view)
        return;
    GtkTreeSelection* sel = gtk_tree_view_get_selection(view);
    GtkTreeModel* old_model;
    GtkTreeIter iter;
    gint saved_cid = -1;
    if (gtk_tree_selection_get_selected(sel, &old_model, &iter))
        gtk_tree_model_get(old_model, &iter, 0, &saved_cid, -1);

    GtkListStore* store = GTK_LIST_STORE(gtk_tree_view_get_model(view));
    gtk_list_store_clear(store);
    for (int i = 0; i < MAX_COURSES; i++)
    {
        GtkTreeIter new_iter;
        gtk_list_store_append(store, &new_iter);
        gtk_list_store_set(store, &new_iter, 0, app_state.courses[i].id, 1, app_state.courses[i].name,
                           2, app_state.courses[i].capacity, 3, app_state.courses[i].enrolled, 4,
                           app_state.courses[i].credits, 5, app_state.courses[i].schedule, -1);
        if (saved_cid != -1 && app_state.courses[i].id == saved_cid)
        {
            GtkTreePath* path = gtk_tree_model_get_path(GTK_TREE_MODEL(store),
                                                        &new_iter);
            gtk_tree_selection_select_path(sel, path);
            gtk_tree_path_free(path);
        }
    }
}

void refresh_student_info_label(void)
{
    GtkLabel* label = GTK_LABEL(
        g_object_get_data(G_OBJECT(app_state.g_widgets.selection_view), "info_label"));
    GtkComboBoxText* combo = GTK_COMBO_BOX_TEXT(
        g_object_get_data(G_OBJECT(app_state.g_widgets.selection_view), "student_combo"));
    if (!label || !combo)
        return;
    gint active = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
    if (active < 0 || active >= app_state.student_count)
    {
        gtk_label_set_text(label, "请选择学生");
        return;
    }
    Student* s = &app_state.students[active];
    GString* info = g_string_new("");
    g_string_append_printf(
        info,
        "学生: %s | 学号: %s | 性别: %s | 已选: %d 门 | 学分: %d | 课程: ",
        s->name, s->student_id, s->gender, s->num_courses, s->total_credits);
    for (int i = 0; i < s->num_courses; i++)
    {
        int ci = get_course_index(s->courses[i]);
        if (ci >= 0)
            g_string_append_printf(info, "%s ", app_state.courses[ci].name);
    }
    gtk_label_set_text(label, info->str);
    g_string_free(info, TRUE);
}

void refresh_grade_table(void)
{
    GtkTreeView* view = GTK_TREE_VIEW(
        g_object_get_data(G_OBJECT(app_state.g_widgets.grade_view), "grade_table"));
    GtkComboBoxText* combo = GTK_COMBO_BOX_TEXT(
        g_object_get_data(G_OBJECT(app_state.g_widgets.grade_view), "student_combo"));
    if (!view || !combo)
        return;

    gint active = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
    if (active < 0 || active >= app_state.student_count)
    {
        GtkListStore* store = GTK_LIST_STORE(gtk_tree_view_get_model(view));
        gtk_list_store_clear(store);
        return;
    }
    Student* s = &app_state.students[active];
    compute_gpa(active);
    GtkListStore* store = GTK_LIST_STORE(gtk_tree_view_get_model(view));
    gtk_list_store_clear(store);
    for (int i = 0; i < s->num_courses; i++)
    {
        int cid = s->courses[i];
        int ci = get_course_index(cid);
        if (ci < 0)
            continue;
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gchar* score_text = (s->scores[ci] >= 0)
                                ? g_strdup_printf("%d", s->scores[ci])
                                : g_strdup("");
        gchar* status_text =
            (s->scores[ci] >= 0)
                ? g_strdup(s->scores[ci] >= PASS_SCORE ? "及格" : "不及格")
                : g_strdup("未录入");
        gtk_list_store_set(store, &iter, 0, cid, 1, app_state.courses[ci].name, 2,
                           score_text, 3, status_text, -1);
        g_free(score_text);
        g_free(status_text);
    }
    GtkLabel* gpa_label = GTK_LABEL(
        g_object_get_data(G_OBJECT(app_state.g_widgets.grade_view), "gpa_label"));
    if (gpa_label)
    {
        gchar* gpa_text = g_strdup_printf("GPA: %.2f %s", s->gpa,
                                          s->gpa_warning ? "[学业预警]" : "");
        gtk_label_set_text(gpa_label, gpa_text);
        g_free(gpa_text);
    }
}

void refresh_stats_table(gboolean sorted)
{
    GtkTreeView* view = GTK_TREE_VIEW(
        g_object_get_data(G_OBJECT(app_state.g_widgets.stats_view), "stats_table"));
    if (!view)
        return;
    GtkListStore* store = GTK_LIST_STORE(gtk_tree_view_get_model(view));
    gtk_list_store_clear(store);
    Student sorted_arr[MAX_STUDENTS];
    if (sorted)
        sort_students(sorted_arr, app_state.student_count);
    for (int i = 0; i < app_state.student_count; i++)
    {
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        const Student* s = sorted ? &sorted_arr[i] : &app_state.students[i];
        gtk_list_store_set(store, &iter, 0, i + 1, 1, s->name, 2, s->student_id,
                           3, s->gender, 4, s->num_courses, 5, s->total_credits,
                           6, s->fail_count, -1);
    }
}

void refresh_all_combos(void)
{
    GtkWidget* sel_combo = GTK_WIDGET(
        g_object_get_data(G_OBJECT(app_state.g_widgets.selection_view), "student_combo"));
    GtkWidget* grade_combo = GTK_WIDGET(
        g_object_get_data(G_OBJECT(app_state.g_widgets.grade_view), "student_combo"));
    if (sel_combo)
        refresh_student_combo(GTK_COMBO_BOX_TEXT(sel_combo));
    if (grade_combo)
        refresh_student_combo(GTK_COMBO_BOX_TEXT(grade_combo));
}

void refresh_all_views(void)
{
    if (!app_state.g_widgets.notebook)
        return;
    refresh_course_table();
    refresh_student_table();
    refresh_selection_table();
    refresh_all_combos();
    refresh_stats_table(FALSE);
    refresh_student_info_label();
    refresh_grade_table();
}

void on_save_all(GtkWidget* widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;
    save_courses("courses.txt");
    save_students("students.txt");
}

void on_save_grades(GtkButton* btn, gpointer user_data)
{
    (void)btn;
    (void)user_data;
    save_students("students.txt");
    refresh_all_views();
}

void on_add_course_to_system(GtkButton* btn, gpointer user_data)
{
    (void)btn;
    (void)user_data;
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "添加课程", GTK_WINDOW(app_state.g_widgets.window), GTK_DIALOG_MODAL, "_取消",
        GTK_RESPONSE_CANCEL, "_确定", GTK_RESPONSE_ACCEPT, NULL);
    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    GtkWidget* n_entry = gtk_entry_new();
    GtkWidget* c_entry = gtk_entry_new();
    GtkWidget* cr_entry = gtk_entry_new();
    GtkWidget* s_entry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("课程名称:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), n_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("容量:"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), c_entry, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("学分:"), 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), cr_entry, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("上课时间:"), 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), s_entry, 1, 3, 1, 1);
    gtk_container_add(GTK_CONTAINER(content), grid);
    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        const gchar* name = gtk_entry_get_text(GTK_ENTRY(n_entry));
        int cap = atoi(gtk_entry_get_text(GTK_ENTRY(c_entry)));
        int cred = atoi(gtk_entry_get_text(GTK_ENTRY(cr_entry)));
        if (name && strlen(name) && cap > 0 && cred > 0)
        {
            for (int i = 0; i < MAX_COURSES; i++)
            {
                if (app_state.courses[i].id == 0)
                {
                    app_state.courses[i].id = ++app_state.max_course_id;
                    size_t len = strlen(name);
                    if (len > MAX_CNAME_LEN - 1)
                        len = MAX_CNAME_LEN - 1;
                    strncpy(app_state.courses[i].name, name, len);
                    app_state.courses[i].name[len] = '\0';
                    app_state.courses[i].capacity = cap;
                    app_state.courses[i].credits = cred;
                    app_state.courses[i].enrolled = 0;
                    const gchar* sched = gtk_entry_get_text(GTK_ENTRY(s_entry));
                    len = sched ? strlen(sched) : 0;
                    if (len > MAX_SCHEDULE_LEN - 1)
                        len = MAX_SCHEDULE_LEN - 1;
                    strncpy(app_state.courses[i].schedule, sched, len);
                    app_state.courses[i].schedule[len] = '\0';
                    save_courses("courses.txt");
                    rebuild_course_index();
                    refresh_all_views();
                    break;
                }
            }
        }
    }
    gtk_widget_destroy(dialog);
}

void on_edit_course(GtkButton* btn, gpointer user_data)
{
    (void)btn;
    (void)user_data;
    GtkTreeSelection* sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(app_state.g_widgets.course_table));
    GtkTreeModel* model;
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected(sel, &model, &iter))
        return;
    gint id;
    gtk_tree_model_get(model, &iter, 0, &id, -1);
    int ci = get_course_index(id);
    if (ci < 0)
        return;
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "修改名称", GTK_WINDOW(app_state.g_widgets.window), GTK_DIALOG_MODAL, "_取消",
        GTK_RESPONSE_CANCEL, "_确定", GTK_RESPONSE_ACCEPT, NULL);
    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), app_state.courses[ci].name);
    gtk_container_add(GTK_CONTAINER(content), entry);
    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        const gchar* t = gtk_entry_get_text(GTK_ENTRY(entry));
        if (t && strlen(t))
        {
            size_t len = strlen(t);
            if (len > MAX_CNAME_LEN - 1)
                len = MAX_CNAME_LEN - 1;
            strncpy(app_state.courses[ci].name, t, len);
            app_state.courses[ci].name[len] = '\0';
            save_courses("courses.txt");
            refresh_all_views();
        }
    }
    gtk_widget_destroy(dialog);
}

void on_add_student(GtkButton* btn, gpointer user_data)
{
    (void)btn;
    (void)user_data;
    if (app_state.student_count >= MAX_STUDENTS)
        return;
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "添加学生", GTK_WINDOW(app_state.g_widgets.window), GTK_DIALOG_MODAL, "_取消",
        GTK_RESPONSE_CANCEL, "_确定", GTK_RESPONSE_ACCEPT, NULL);
    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    GtkWidget* n_entry = gtk_entry_new();
    GtkWidget* i_entry = gtk_entry_new();
    GtkWidget* g_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_combo), "男");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_combo), "女");
    gtk_combo_box_set_active(GTK_COMBO_BOX(g_combo), 0);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("姓名:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), n_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("学号:"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), i_entry, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("性别:"), 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), g_combo, 1, 2, 1, 1);
    gtk_container_add(GTK_CONTAINER(content), grid);
    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        const gchar* name = gtk_entry_get_text(GTK_ENTRY(n_entry));
        const gchar* id = gtk_entry_get_text(GTK_ENTRY(i_entry));
        gchar* gender =
            gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(g_combo));
        if (name && strlen(name) && id && strlen(id) &&
            find_student_by_id(id) < 0)
        {
            Student* s = &app_state.students[app_state.student_count++];
            memset(s, 0, sizeof(Student));
            size_t len = strlen(name);
            if (len > MAX_NAME_LEN - 1)
                len = MAX_NAME_LEN - 1;
            strncpy(s->name, name, len);
            s->name[len] = '\0';
            len = strlen(id);
            if (len > MAX_ID_LEN - 1)
                len = MAX_ID_LEN - 1;
            strncpy(s->student_id, id, len);
            s->student_id[len] = '\0';
            if (gender)
            {
                len = strlen(gender);
                if (len > 5)
                    len = 5;
                strncpy(s->gender, gender, len);
                s->gender[len] = '\0';
            }
            save_students("students.txt");
            refresh_all_views();
        }
        if (gender)
            g_free(gender);
    }
    gtk_widget_destroy(dialog);
}

void on_delete_student(GtkButton* btn, gpointer user_data)
{
    (void)btn;
    (void)user_data;
    GtkTreeView* view = GTK_TREE_VIEW(
        g_object_get_data(G_OBJECT(app_state.g_widgets.notebook), "student_table_view"));
    if (!view)
        return;
    GtkTreeSelection* sel = gtk_tree_view_get_selection(view);
    GtkTreeModel* model;
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected(sel, &model, &iter))
        return;
    gchar* student_id = NULL;
    gtk_tree_model_get(model, &iter, 2, &student_id, -1);
    if (!student_id)
        return;
    int idx = find_student_by_id(student_id);
    if (idx < 0)
    {
        g_free(student_id);
        return;
    }
    GtkWidget* dialog = gtk_message_dialog_new(
        GTK_WINDOW(app_state.g_widgets.window), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING,
        GTK_BUTTONS_OK_CANCEL, "确定删除学生 %s (%s) ?", app_state.students[idx].name,
        app_state.students[idx].student_id);
    int resp = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    if (resp != GTK_RESPONSE_OK)
    {
        g_free(student_id);
        return;
    }

    Student* s = &app_state.students[idx];
    for (int i = 0; i < s->num_courses; i++)
        decrease_enrolled(s->courses[i]);
    for (int i = idx; i < app_state.student_count - 1; i++)
        app_state.students[i] = app_state.students[i + 1];
    app_state.student_count--;
    save_courses("courses.txt");
    save_students("students.txt");
    refresh_all_views();
    g_free(student_id);
}

void on_edit_student(GtkButton* btn, gpointer user_data)
{
    (void)btn;
    (void)user_data;
    GtkTreeView* view = GTK_TREE_VIEW(
        g_object_get_data(G_OBJECT(app_state.g_widgets.notebook), "student_table_view"));
    if (!view)
        return;
    GtkTreeSelection* sel = gtk_tree_view_get_selection(view);
    GtkTreeModel* model;
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected(sel, &model, &iter))
        return;
    gchar* student_id = NULL;
    gtk_tree_model_get(model, &iter, 2, &student_id, -1);
    if (!student_id)
        return;
    int idx = find_student_by_id(student_id);
    if (idx < 0)
    {
        g_free(student_id);
        return;
    }

    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "修改学生信息", GTK_WINDOW(app_state.g_widgets.window), GTK_DIALOG_MODAL, "_取消",
        GTK_RESPONSE_CANCEL, "_确定", GTK_RESPONSE_ACCEPT, NULL);
    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    GtkWidget* n_entry = gtk_entry_new();
    GtkWidget* i_entry = gtk_entry_new();
    GtkWidget* g_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_combo), "男");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_combo), "女");
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("姓名:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), n_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("学号:"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), i_entry, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("性别:"), 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), g_combo, 1, 2, 1, 1);
    gtk_container_add(GTK_CONTAINER(content), grid);

    Student* s = &app_state.students[idx];
    gtk_entry_set_text(GTK_ENTRY(n_entry), s->name);
    gtk_entry_set_text(GTK_ENTRY(i_entry), s->student_id);
    if (strcmp(s->gender, "女") == 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(g_combo), 1);
    else
        gtk_combo_box_set_active(GTK_COMBO_BOX(g_combo), 0);

    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        const gchar* name = gtk_entry_get_text(GTK_ENTRY(n_entry));
        const gchar* id = gtk_entry_get_text(GTK_ENTRY(i_entry));
        gchar* gender =
            gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(g_combo));
        if (!name || !strlen(name) || !id || !strlen(id))
        {
            if (gender)
                g_free(gender);
            gtk_widget_destroy(dialog);
            g_free(student_id);
            return;
        }
        if (strcmp(id, s->student_id) != 0 && find_student_by_id(id) >= 0)
        {
            GtkWidget* d = gtk_message_dialog_new(
                GTK_WINDOW(app_state.g_widgets.window), GTK_DIALOG_MODAL,
                GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "学号已存在!");
            gtk_dialog_run(GTK_DIALOG(d));
            gtk_widget_destroy(d);
            if (gender)
                g_free(gender);
            gtk_widget_destroy(dialog);
            g_free(student_id);
            return;
        }
        size_t len = strlen(name);
        if (len > MAX_NAME_LEN - 1)
            len = MAX_NAME_LEN - 1;
        strncpy(s->name, name, len);
        s->name[len] = '\0';
        len = strlen(id);
        if (len > MAX_ID_LEN - 1)
            len = MAX_ID_LEN - 1;
        strncpy(s->student_id, id, len);
        s->student_id[len] = '\0';
        if (gender)
        {
            len = strlen(gender);
            if (len > 5)
                len = 5;
            strncpy(s->gender, gender, len);
            s->gender[len] = '\0';
            g_free(gender);
        }
        save_students("students.txt");
        refresh_all_views();
    }
    gtk_widget_destroy(dialog);
    g_free(student_id);
}

void on_add_course(GtkButton* btn, gpointer user_data)
{
    (void)btn;
    (void)user_data;
    gint s_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(g_object_get_data(
        G_OBJECT(app_state.g_widgets.selection_view), "student_combo")));
    GtkWidget* table = GTK_WIDGET(g_object_get_data(
        G_OBJECT(app_state.g_widgets.selection_view), "selection_table"));
    GtkTreeSelection* sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(table));
    GtkTreeModel* model;
    GtkTreeIter iter;
    if (s_idx < 0 || !gtk_tree_selection_get_selected(sel, &model, &iter))
        return;
    gint cid;
    gtk_tree_model_get(model, &iter, 0, &cid, -1);
    int ci = get_course_index(cid);
    if (ci < 0)
        return;
    Student* s = &app_state.students[s_idx];
    if (s->num_courses >= MAX_SELECTS)
    {
        GtkWidget* d = gtk_message_dialog_new(
            GTK_WINDOW(app_state.g_widgets.window), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING,
            GTK_BUTTONS_OK, "上限(5门)!");
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        return;
    }
    if (app_state.courses[ci].enrolled >= app_state.courses[ci].capacity)
    {
        GtkWidget* d = gtk_message_dialog_new(
            GTK_WINDOW(app_state.g_widgets.window), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING,
            GTK_BUTTONS_OK, "已满!");
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        return;
    }
    for (int i = 0; i < s->num_courses; i++)
    {
        if (s->courses[i] == cid)
        {
            GtkWidget* d = gtk_message_dialog_new(
                GTK_WINDOW(app_state.g_widgets.window), GTK_DIALOG_MODAL,
                GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "已选!");
            gtk_dialog_run(GTK_DIALOG(d));
            gtk_widget_destroy(d);
            return;
        }
        if (check_time_conflict(s->courses[i], cid))
        {
            gchar* msg = g_strdup_printf("时间冲突: %s 与已选课程时间重叠",
                                         app_state.courses[ci].name);
            GtkWidget* d = gtk_message_dialog_new(
                GTK_WINDOW(app_state.g_widgets.window), GTK_DIALOG_MODAL,
                GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "%s", msg);
            gtk_dialog_run(GTK_DIALOG(d));
            gtk_widget_destroy(d);
            g_free(msg);
            return;
        }
    }
    s->courses[s->num_courses++] = cid;
    increase_enrolled(cid);
    s->gpa_computed = 0;
    save_courses("courses.txt");
    refresh_all_views();
}

void on_drop_course(GtkButton* btn, gpointer user_data)
{
    (void)btn;
    (void)user_data;
    gint s_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(g_object_get_data(
        G_OBJECT(app_state.g_widgets.selection_view), "student_combo")));
    GtkWidget* table = GTK_WIDGET(g_object_get_data(
        G_OBJECT(app_state.g_widgets.selection_view), "selection_table"));
    GtkTreeSelection* sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(table));
    GtkTreeModel* model;
    GtkTreeIter iter;
    if (s_idx < 0 || !gtk_tree_selection_get_selected(sel, &model, &iter))
        return;
    gint cid;
    gtk_tree_model_get(model, &iter, 0, &cid, -1);
    Student* s = &app_state.students[s_idx];
    if (s->num_courses <= 1)
    {
        GtkWidget* d = gtk_message_dialog_new(
            GTK_WINDOW(app_state.g_widgets.window), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING,
            GTK_BUTTONS_OK, "至少保留1门!");
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        return;
    }
    for (int i = 0; i < s->num_courses; i++)
    {
        if (s->courses[i] == cid)
        {
            decrease_enrolled(cid);
            for (int j = i; j < s->num_courses - 1; j++)
                s->courses[j] = s->courses[j + 1];
            s->num_courses--;
            s->total_credits = 0;
            for (int j = 0; j < s->num_courses; j++)
            {
                int ci = get_course_index(s->courses[j]);
                if (ci >= 0)
                    s->total_credits += app_state.courses[ci].credits;
            }
            s->gpa_computed = 0;
            save_students("students.txt");
            refresh_all_views();
            return;
        }
    }
    GtkWidget* d = gtk_message_dialog_new(GTK_WINDOW(app_state.g_widgets.window),
                                          GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING,
                                          GTK_BUTTONS_OK, "未选该课!");
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
}

void on_finish_selection(GtkButton* btn, gpointer user_data)
{
    (void)btn;
    (void)user_data;
    gint s_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(g_object_get_data(
        G_OBJECT(app_state.g_widgets.selection_view), "student_combo")));
    if (s_idx < 0)
        return;
    Student* s = &app_state.students[s_idx];
    if (s->num_courses < 3)
        return;
    s->total_credits = 0;
    for (int j = 0; j < s->num_courses; j++)
    {
        int ci = get_course_index(s->courses[j]);
        if (ci >= 0)
            s->total_credits += app_state.courses[ci].credits;
    }
    save_students("students.txt");
    refresh_all_views();
}

void on_student_combo_changed(GtkComboBox* combo, gpointer user_data)
{
    (void)combo;
    (void)user_data;
    refresh_student_info_label();
    refresh_grade_table();
}

void on_student_combo_hide(GtkWidget* widget, gpointer user_data)
{
    (void)user_data;
    refresh_student_info_label();
    refresh_grade_table();
    gtk_combo_box_set_active(GTK_COMBO_BOX(widget),
                             gtk_combo_box_get_active(GTK_COMBO_BOX(widget)));
}

void on_grade_cell_edited(GtkCellRendererText* cell, gchar* path_string,
                          gchar* new_text, gpointer user_data)
{
    (void)cell;
    (void)user_data;
    GtkWidget* table =
        g_object_get_data(G_OBJECT(app_state.g_widgets.grade_view), "grade_table");
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(table));
    GtkTreePath* path = gtk_tree_path_new_from_string(path_string);
    GtkTreeIter iter;
    gtk_tree_model_get_iter(model, &iter, path);
    gint cid;
    gtk_tree_model_get(model, &iter, 0, &cid, -1);
    gint s_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(
        g_object_get_data(G_OBJECT(app_state.g_widgets.grade_view), "student_combo")));
    if (s_idx < 0)
        return;
    Student* s = &app_state.students[s_idx];
    int score = atoi(new_text);
    if (score < 0)
        score = 0;
    if (score > 100)
        score = 100;
    int ci = get_course_index(cid);
    if (ci >= 0)
    {
        s->scores[ci] = score;
        compute_fail_count(s_idx);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, 2, new_text, 3,
                           score >= PASS_SCORE ? "及格" : "不及格", -1);
        save_students("students.txt");
        refresh_all_views();
        GtkWidget* combo = GTK_WIDGET(
            g_object_get_data(G_OBJECT(app_state.g_widgets.grade_view), "student_combo"));
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), s_idx);
    }
    gtk_tree_path_free(path);
}

void on_sort_by_credits(GtkButton* btn, gpointer user_data)
{
    (void)btn;
    (void)user_data;
    refresh_stats_table(TRUE);
}

void on_fail_stats(GtkButton* btn, gpointer user_data)
{
    (void)btn;
    (void)user_data;
    GString* res = g_string_new("===== 不及格学生统计 =====\n\n");
    g_string_append(res, "--- 1 门不及格 ---\n");
    gboolean found1 = FALSE;
    for (int i = 0; i < app_state.student_count; i++)
    {
        if (app_state.students[i].fail_count == 1)
        {
            g_string_append_printf(res, "  %s (%s) - 科目: ", app_state.students[i].name,
                                   app_state.students[i].student_id);
            for (int j = 0; j < app_state.students[i].num_courses; j++)
            {
                int ci = get_course_index(app_state.students[i].courses[j]);
                if (ci >= 0 && app_state.students[i].scores[ci] >= 0 &&
                    app_state.students[i].scores[ci] < PASS_SCORE)
                    g_string_append_printf(res, "%s(%d分) ", app_state.courses[ci].name,
                                           app_state.students[i].scores[ci]);
            }
            found1 = TRUE;
            g_string_append(res, "\n");
        }
    }
    if (!found1)
        g_string_append(res, "  (无)\n");
    g_string_append(res, "\n--- 3 门不及格 ---\n");
    gboolean found3 = FALSE;
    for (int i = 0; i < app_state.student_count; i++)
    {
        if (app_state.students[i].fail_count == 3)
        {
            g_string_append_printf(res, "  %s (%s)\n", app_state.students[i].name,
                                   app_state.students[i].student_id);
            found3 = TRUE;
        }
    }
    if (!found3)
        g_string_append(res, "  (无)\n");
    GtkWidget* dialog = gtk_message_dialog_new(
        GTK_WINDOW(app_state.g_widgets.window), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK, "%s", res->str);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_string_free(res, TRUE);
}

void on_export_csv(GtkWidget* widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;
    FILE* fp = fopen("students_export.csv", "w");
    if (!fp)
    {
        GtkWidget* d = gtk_message_dialog_new(
            GTK_WINDOW(app_state.g_widgets.window), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING,
            GTK_BUTTONS_OK, "导出失败!");
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        return;
    }
    fprintf(fp, "姓名,学号,性别,");
    for (int i = 0; i < MAX_COURSES; i++)
    {
        fprintf(fp, "%s,", app_state.courses[i].name);
    }
    fprintf(fp, "GPA,学业预警\n");
    for (int i = 0; i < app_state.student_count; i++)
    {
        compute_gpa(i);
        Student* s = &app_state.students[i];
        fprintf(fp, "%s,%s,%s,", s->name, s->student_id, s->gender);
        for (int j = 0; j < MAX_COURSES; j++)
        {
            fprintf(fp, "%d,", s->scores[j] >= 0 ? s->scores[j] : -1);
        }
        fprintf(fp, "%.2f,%s\n", s->gpa, s->gpa_warning ? "是" : "否");
    }
    fclose(fp);
    GtkWidget* d = gtk_message_dialog_new(
        GTK_WINDOW(app_state.g_widgets.window), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK, "已导出至 students_export.csv");
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
}

static GtkTreeViewColumn* add_text_column(GtkTreeView* view, const char* title,
                                          int col_id)
{
    GtkTreeViewColumn* col = gtk_tree_view_column_new_with_attributes(
        title, gtk_cell_renderer_text_new(), "text", col_id, NULL);
    gtk_tree_view_append_column(view, col);
    return col;
}

static GtkTreeViewColumn* add_editable_column(GtkTreeView* view,
                                              const char* title, int col_id,
                                              GCallback on_edited)
{
    GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "editable", TRUE, NULL);
    g_signal_connect(renderer, "edited", on_edited, NULL);
    GtkTreeViewColumn* col = gtk_tree_view_column_new_with_attributes(
        title, renderer, "text", col_id, NULL);
    gtk_tree_view_append_column(view, col);
    return col;
}

void create_ui(int argc, char* argv[])
{
    gtk_init(&argc, &argv);
    app_state.g_widgets.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app_state.g_widgets.window),
                         "学生选课及学籍管理系统");
    gtk_window_set_default_size(GTK_WINDOW(app_state.g_widgets.window), 1000, 700);
    g_signal_connect(app_state.g_widgets.window, "destroy", G_CALLBACK(gtk_main_quit),
                     NULL);
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    GtkWidget* menubar = gtk_menu_bar_new();
    GtkWidget* file_menu = gtk_menu_item_new_with_label("文件");
    GtkWidget* file_submenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_menu), file_submenu);
    GtkWidget* save_item = gtk_menu_item_new_with_label("保存");
    GtkWidget* exit_item = gtk_menu_item_new_with_label("退出");
    gtk_menu_shell_append(GTK_MENU_SHELL(file_submenu), save_item);
    GtkWidget* export_item = gtk_menu_item_new_with_label("导出CSV");
    gtk_menu_shell_append(GTK_MENU_SHELL(file_submenu), export_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_submenu), exit_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), GTK_WIDGET(file_menu));
    g_signal_connect(save_item, "activate", G_CALLBACK(on_save_all), NULL);
    g_signal_connect(export_item, "activate", G_CALLBACK(on_export_csv), NULL);
    g_signal_connect(exit_item, "activate", G_CALLBACK(gtk_main_quit), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    app_state.g_widgets.notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(vbox), app_state.g_widgets.notebook, TRUE, TRUE, 0);

    const char* c_titles[] = {"编号", "课程名称", "容量", "已选", "学分"};

    {
        GtkWidget* view = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_container_set_border_width(GTK_CONTAINER(view), 10);
        app_state.g_widgets.course_table = gtk_tree_view_new();
        GtkListStore* store = gtk_list_store_new(
            6, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT,
            G_TYPE_STRING);
        gtk_tree_view_set_model(GTK_TREE_VIEW(app_state.g_widgets.course_table),
                                GTK_TREE_MODEL(store));
        g_object_unref(store);
        const char* course_titles[] = {"编号", "课程名称", "容量", "已选", "学分",
                                       "上课时间"};
        for (int i = 0; i < 6; i++)
            add_text_column(GTK_TREE_VIEW(app_state.g_widgets.course_table),
                            course_titles[i], i);
        GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_widget_set_vexpand(scroll, TRUE);
        gtk_container_add(GTK_CONTAINER(scroll), app_state.g_widgets.course_table);
        gtk_box_pack_start(GTK_BOX(view), scroll, TRUE, TRUE, 0);
        GtkWidget* btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        GtkWidget* add_btn = gtk_button_new_with_label("添加课程");
        GtkWidget* edit_btn = gtk_button_new_with_label("修改名称");
        gtk_box_pack_start(GTK_BOX(btn_box), add_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(btn_box), edit_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(view), btn_box, FALSE, FALSE, 5);
        gtk_notebook_append_page(GTK_NOTEBOOK(app_state.g_widgets.notebook), view,
                                 gtk_label_new("课程管理"));
        g_signal_connect(add_btn, "clicked",
                         G_CALLBACK(on_add_course_to_system), NULL);
        g_signal_connect(edit_btn, "clicked", G_CALLBACK(on_edit_course), NULL);
    }

    {
        GtkWidget* view = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_container_set_border_width(GTK_CONTAINER(view), 10);
        GtkWidget* table = gtk_tree_view_new();
        GtkListStore* store =
            gtk_list_store_new(6, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING,
                               G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);
        gtk_tree_view_set_model(GTK_TREE_VIEW(table), GTK_TREE_MODEL(store));
        g_object_unref(store);
        g_object_set_data(G_OBJECT(app_state.g_widgets.notebook), "student_table_view",
                          table);
        const char* titles[] = {"序号", "姓名", "学号", "性别", "选课", "学分"};
        for (int i = 0; i < 6; i++)
            add_text_column(GTK_TREE_VIEW(table), titles[i], i);
        GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_widget_set_vexpand(scroll, TRUE);
        gtk_container_add(GTK_CONTAINER(scroll), table);
        gtk_box_pack_start(GTK_BOX(view), scroll, TRUE, TRUE, 0);
        GtkWidget* btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        GtkWidget* add_btn = gtk_button_new_with_label("添加学生");
        GtkWidget* edit_btn = gtk_button_new_with_label("编辑学生");
        GtkWidget* del_btn = gtk_button_new_with_label("删除学生");
        gtk_box_pack_start(GTK_BOX(btn_box), add_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(btn_box), edit_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(btn_box), del_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(view), btn_box, FALSE, FALSE, 5);
        g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_student), NULL);
        g_signal_connect(edit_btn, "clicked", G_CALLBACK(on_edit_student),
                         NULL);
        g_signal_connect(del_btn, "clicked", G_CALLBACK(on_delete_student),
                         NULL);
        gtk_notebook_append_page(GTK_NOTEBOOK(app_state.g_widgets.notebook), view,
                                 gtk_label_new("学生管理"));
    }

    {
        GtkWidget* view = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_container_set_border_width(GTK_CONTAINER(view), 10);
        app_state.g_widgets.selection_view = view;
        GtkWidget* combo = gtk_combo_box_text_new();
        GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("学生:"), FALSE, FALSE,
                           5);
        gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(view), hbox, FALSE, FALSE, 5);
        GtkWidget* table = gtk_tree_view_new();
        GtkListStore* store = gtk_list_store_new(
            6, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT,
            G_TYPE_STRING);
        gtk_tree_view_set_model(GTK_TREE_VIEW(table), GTK_TREE_MODEL(store));
        g_object_unref(store);
        const char* sel_titles[] = {"编号", "课程名称", "容量", "已选", "学分",
                                     "上课时间"};
        for (int i = 0; i < 6; i++)
            add_text_column(GTK_TREE_VIEW(table), sel_titles[i], i);
        GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_widget_set_vexpand(scroll, TRUE);
        gtk_container_add(GTK_CONTAINER(scroll), table);
        gtk_box_pack_start(GTK_BOX(view), scroll, TRUE, TRUE, 5);
        GtkWidget* info_label = gtk_label_new("待选课");
        gtk_box_pack_start(GTK_BOX(view), info_label, FALSE, FALSE, 5);
        GtkWidget* btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        GtkWidget* add_btn = gtk_button_new_with_label("添加");
        GtkWidget* drop_btn = gtk_button_new_with_label("退选");
        GtkWidget* fin_btn = gtk_button_new_with_label("完成");
        gtk_box_pack_start(GTK_BOX(btn_box), add_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(btn_box), drop_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(btn_box), fin_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(view), btn_box, FALSE, FALSE, 5);
        g_object_set_data(G_OBJECT(view), "student_combo", combo);
        g_object_set_data(G_OBJECT(view), "selection_table", table);
        g_object_set_data(G_OBJECT(view), "info_label", info_label);
        gtk_notebook_append_page(GTK_NOTEBOOK(app_state.g_widgets.notebook), view,
                                 gtk_label_new("选课操作"));
        g_signal_connect(combo, "changed", G_CALLBACK(on_student_combo_changed),
                         NULL);
        g_signal_connect(combo, "hide", G_CALLBACK(on_student_combo_hide), NULL);
        g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_course), NULL);
        g_signal_connect(drop_btn, "clicked", G_CALLBACK(on_drop_course), NULL);
        g_signal_connect(fin_btn, "clicked", G_CALLBACK(on_finish_selection),
                         NULL);
    }

    {
        GtkWidget* view = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_container_set_border_width(GTK_CONTAINER(view), 10);
        app_state.g_widgets.grade_view = view;
        GtkWidget* combo = gtk_combo_box_text_new();
        GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("学生:"), FALSE, FALSE,
                           5);
        gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(view), hbox, FALSE, FALSE, 5);
        GtkWidget* table = gtk_tree_view_new();
        GtkListStore* store = gtk_list_store_new(4, G_TYPE_INT, G_TYPE_STRING,
                                                 G_TYPE_STRING, G_TYPE_STRING);
        gtk_tree_view_set_model(GTK_TREE_VIEW(table), GTK_TREE_MODEL(store));
        g_object_unref(store);
        const char* titles[] = {"ID", "科目", "成绩", "状态"};
        for (int i = 0; i < 4; i++)
        {
            if (i == 2)
                add_editable_column(GTK_TREE_VIEW(table), titles[i], i,
                                    G_CALLBACK(on_grade_cell_edited));
            else
                add_text_column(GTK_TREE_VIEW(table), titles[i], i);
        }
        GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_widget_set_vexpand(scroll, TRUE);
        gtk_container_add(GTK_CONTAINER(scroll), table);
        GtkWidget* gpa_label = gtk_label_new("GPA: 0.00");
        gtk_box_pack_start(GTK_BOX(view), scroll, TRUE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(view), gpa_label, FALSE, FALSE, 5);
        GtkWidget* save_btn = gtk_button_new_with_label("保存成绩");
        gtk_box_pack_start(GTK_BOX(view), save_btn, FALSE, FALSE, 5);
        g_object_set_data(G_OBJECT(view), "student_combo", combo);
        g_object_set_data(G_OBJECT(view), "grade_table", table);
        g_object_set_data(G_OBJECT(view), "gpa_label", gpa_label);
        gtk_notebook_append_page(GTK_NOTEBOOK(app_state.g_widgets.notebook), view,
                                 gtk_label_new("成绩录入"));
        g_signal_connect(combo, "changed", G_CALLBACK(on_student_combo_changed),
                         NULL);
        g_signal_connect(combo, "hide", G_CALLBACK(on_student_combo_hide), NULL);
        g_signal_connect(save_btn, "clicked", G_CALLBACK(on_save_grades), NULL);
    }

    {
        GtkWidget* view = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_container_set_border_width(GTK_CONTAINER(view), 10);
        app_state.g_widgets.stats_view = view;
        GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        GtkWidget* sort_btn = gtk_button_new_with_label("排序");
        GtkWidget* fail_btn = gtk_button_new_with_label("不及格");
        gtk_box_pack_start(GTK_BOX(hbox), sort_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), fail_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(view), hbox, FALSE, FALSE, 5);
        GtkWidget* table = gtk_tree_view_new();
        GtkListStore* store = gtk_list_store_new(
            7, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
            G_TYPE_INT, G_TYPE_INT, G_TYPE_INT);
        gtk_tree_view_set_model(GTK_TREE_VIEW(table), GTK_TREE_MODEL(store));
        g_object_unref(store);
        const char* titles[] = {"排名", "姓名", "学号",  "性别",
                                "选",   "学分", "不及格"};
        for (int i = 0; i < 7; i++)
            add_text_column(GTK_TREE_VIEW(table), titles[i], i);
        GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_widget_set_vexpand(scroll, TRUE);
        gtk_container_add(GTK_CONTAINER(scroll), table);
        gtk_box_pack_start(GTK_BOX(view), scroll, TRUE, TRUE, 0);
        g_object_set_data(G_OBJECT(view), "stats_table", table);
        gtk_notebook_append_page(GTK_NOTEBOOK(app_state.g_widgets.notebook), view,
                                 gtk_label_new("统计排序"));
        g_signal_connect(sort_btn, "clicked", G_CALLBACK(on_sort_by_credits),
                         NULL);
        g_signal_connect(fail_btn, "clicked", G_CALLBACK(on_fail_stats), NULL);
    }

    gtk_container_add(GTK_CONTAINER(app_state.g_widgets.window), vbox);
    refresh_all_views();

    GtkWidget* sel_combo = GTK_WIDGET(
        g_object_get_data(G_OBJECT(app_state.g_widgets.selection_view), "student_combo"));
    GtkWidget* grade_combo = GTK_WIDGET(
        g_object_get_data(G_OBJECT(app_state.g_widgets.grade_view), "student_combo"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(sel_combo),
                             (app_state.student_count > 0) ? 0 : -1);
    gtk_combo_box_set_active(GTK_COMBO_BOX(grade_combo),
                             (app_state.student_count > 0) ? 0 : -1);
    refresh_student_info_label();
    refresh_grade_table();

    gtk_widget_show_all(app_state.g_widgets.window);
    gtk_main();
}

int main(int argc, char* argv[])
{
    memset(&app_state, 0, sizeof(app_state));
    app_state.course_index_valid = 0;
    for (int i = 0; i < MAX_STUDENTS; i++)
        app_state.gpa_cache_valid[i] = 0;
    
    if (!load_courses("courses.txt"))
    {
        init_courses();
        save_courses("courses.txt");
    }
    load_students("students.txt");
    printf("Loaded %d students\n", app_state.student_count);
    create_ui(argc, argv);
    save_courses("courses.txt");
    save_students("students.txt");
    return 0;
}
