#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME_LEN 3
#define MAX_ID_LEN 20
#define MAX_CNAME_LEN 50
#define MAX_COURSES 30
#define MAX_STUDENTS 200
#define MAX_SELECTS 5
#define PASS_SCORE 60

typedef struct
{
    int id;
    char name[MAX_CNAME_LEN];
    int capacity;
    int enrolled;
    int credits;
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
} Student;

Course courses[MAX_COURSES];
Student students[MAX_STUDENTS];
int student_count = 0;
int course_count = 0;

void init_courses(void);
int load_courses(const char* filename);
void save_courses(const char* filename);
int load_students(const char* filename);
void save_students(const char* filename);
int find_student_by_id(const char* id);
int get_course_index(int cid);
void compute_fail_count(int idx);
void sort_students(Student* sorted, int count);
void recalc_all_data(void);

void refresh_all_views(void);
void refresh_course_table(void);
void refresh_student_table(void);
void refresh_student_combo(GtkComboBoxText* combo);
void refresh_selection_table(void);
void refresh_student_info_label(void);
void refresh_grade_table(void);
void refresh_stats_table(gboolean sorted);
void refresh_all_combos(void);
static void show_message(GtkMessageType type, const gchar* msg);

typedef struct
{
    GtkWidget* window;
    GtkWidget* notebook;
    GtkWidget* course_table;
    GtkWidget* selection_view;
    GtkWidget* grade_view;
    GtkWidget* stats_view;
    GtkWidget* course_add_btn;
    GtkWidget* course_edit_btn;
    GtkWidget* course_delete_btn;
    GtkWidget* student_add_btn;
    GtkWidget* student_edit_btn;
    GtkWidget* student_delete_btn;
    GtkWidget* selection_add_btn;
    GtkWidget* selection_drop_btn;
    GtkWidget* selection_finish_btn;
    GtkWidget* grade_save_btn;
} AppWidgets;

static AppWidgets g_widgets;

// --- Data Management ---

void init_courses()
{
    const char* names[] = {"数据结构", "操作系统", "计算机网络", "数据库原理",
                           "软件工程", "人工智能", "编译原理"};
    int default_count = sizeof(names) / sizeof(names[0]);
    for (int i = 0; i < MAX_COURSES; i++)
    {
        courses[i].id = 0;
        courses[i].name[0] = '\0';
        courses[i].capacity = 0;
        courses[i].credits = 0;
        courses[i].enrolled = 0;
    }
    course_count = 0;
    for (int i = 0; i < default_count && i < MAX_COURSES; i++)
    {
        courses[i].id = i + 1;
        strncpy(courses[i].name, names[i], MAX_CNAME_LEN - 1);
        courses[i].name[MAX_CNAME_LEN - 1] = '\0';
        courses[i].capacity = 10 * (i + 1);
        courses[i].credits = i + 1;
        courses[i].enrolled = 0;
        course_count++;
    }
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
    char buf[256];
    if (!fgets(buf, sizeof(buf), fp))
    {
        fclose(fp);
        return 0;
    }
    count = atoi(buf);
    for (int i = 0; i < MAX_COURSES; i++)
    {
        courses[i].id = 0;
        courses[i].name[0] = '\0';
        courses[i].capacity = 0;
        courses[i].credits = 0;
        courses[i].enrolled = 0;
    }
    course_count = 0;
    while (fgets(buf, sizeof(buf), fp))
    {
        char* t = strtok(buf, "|\n");
        if (!t)
            continue;
        int cid = atoi(t);
        if (cid <= 0 || cid > MAX_COURSES)
            continue;
        int idx = cid - 1;
        courses[idx].id = cid;
        t = strtok(NULL, "|\n");
        if (t)
        {
            strncpy(courses[idx].name, t, MAX_CNAME_LEN - 1);
            courses[idx].name[MAX_CNAME_LEN - 1] = '\0';
        }
        t = strtok(NULL, "|\n");
        if (t)
            courses[idx].capacity = atoi(t);
        t = strtok(NULL, "|\n");
        if (t)
            courses[idx].enrolled = atoi(t);
        t = strtok(NULL, "|\n");
        if (t)
            courses[idx].credits = atoi(t);
        course_count++;
        if (course_count >= count || course_count >= MAX_COURSES)
            break;
    }
    fclose(fp);
    return (course_count > 0) ? 1 : 0;
}

void save_courses(const char* filename)
{
    FILE* fp = fopen(filename, "w");
    if (!fp)
        return;
    fprintf(fp, "%d\n", course_count);
    for (int i = 0; i < MAX_COURSES; i++)
    {
        if (courses[i].id == 0)
            continue;
        fprintf(fp, "%d|%s|%d|%d|%d\n", courses[i].id, courses[i].name,
                courses[i].capacity, courses[i].enrolled, courses[i].credits);
    }
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

    memset(students, 0, sizeof(students));
    student_count = 0;

    int write_idx = 0;
    for (int i = 0; i < count; i++)
    {
        if (!fgets(buf, sizeof(buf), fp))
            break;
        Student* s = &students[write_idx];
        memset(s, 0, sizeof(Student));
        for (int j = 0; j < MAX_COURSES; j++)
            s->scores[j] = -1;

        char* tokens[512];
        int token_count = 0;
        char* t = strtok(buf, "|\n");
        while (t && token_count < (int)(sizeof(tokens) / sizeof(tokens[0])))
        {
            tokens[token_count++] = t;
            t = strtok(NULL, "|\n");
        }
        if (token_count < 4)
            continue;
        strncpy(s->name, tokens[0], MAX_NAME_LEN - 1);
        s->name[MAX_NAME_LEN - 1] = '\0';
        strncpy(s->student_id, tokens[1], MAX_ID_LEN - 1);
        s->student_id[MAX_ID_LEN - 1] = '\0';
        strncpy(s->gender, tokens[2], 5);
        s->gender[5] = '\0';
        s->num_courses = atoi(tokens[3]);

        int idx = 4;
        for (int j = 0; j < MAX_SELECTS && idx < token_count; j++, idx++)
            s->courses[j] = atoi(tokens[idx]);

        int remaining = token_count - idx;
        if (remaining >= 2)
        {
            int score_count = remaining - 2;
            for (int j = 0; j < score_count && j < MAX_COURSES; j++)
                s->scores[j] = atoi(tokens[idx + j]);
        }
        write_idx++;
    }
    student_count = write_idx;
    fclose(fp);
    return 1;
}

void save_students(const char* filename)
{
    FILE* fp = fopen(filename, "w");
    if (!fp)
        return;
    fprintf(fp, "%d\n", student_count);
    for (int i = 0; i < student_count; i++)
    {
        Student* s = &students[i];
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
    for (int i = 0; i < student_count; i++)
        if (strcmp(students[i].student_id, id) == 0)
            return i;
    return -1;
}

int get_course_index(int cid)
{
    for (int i = 0; i < MAX_COURSES; i++)
        if (courses[i].id == cid)
            return i;
    return -1;
}

void compute_fail_count(int idx)
{
    Student* s = &students[idx];
    s->fail_count = 0;
    for (int j = 0; j < s->num_courses; j++)
    {
        int ci = get_course_index(s->courses[j]);
        if (ci >= 0 && s->scores[ci] >= 0 && s->scores[ci] < PASS_SCORE)
            s->fail_count++;
    }
}

void recalc_all_data(void)
{
    int active_courses = 0;
    for (int i = 0; i < MAX_COURSES; i++)
    {
        if (courses[i].id > 0)
        {
            courses[i].enrolled = 0;
            active_courses++;
        }
    }
    course_count = active_courses;

    for (int i = 0; i < student_count; i++)
    {
        Student* s = &students[i];
        if (s->num_courses < 0)
            s->num_courses = 0;
        int filtered[MAX_SELECTS];
        int kept = 0;
        for (int j = 0; j < s->num_courses && kept < MAX_SELECTS; j++)
        {
            int cid = s->courses[j];
            if (cid <= 0)
                continue;
            if (get_course_index(cid) < 0)
                continue;
            gboolean duplicate = FALSE;
            for (int k = 0; k < kept; k++)
            {
                if (filtered[k] == cid)
                {
                    duplicate = TRUE;
                    break;
                }
            }
            if (!duplicate)
                filtered[kept++] = cid;
        }
        for (int j = 0; j < kept; j++)
            s->courses[j] = filtered[j];
        for (int j = kept; j < MAX_SELECTS; j++)
            s->courses[j] = 0;
        s->num_courses = kept;

        s->total_credits = 0;
        s->fail_count = 0;
        for (int j = 0; j < s->num_courses; j++)
        {
            int ci = get_course_index(s->courses[j]);
            if (ci < 0)
                continue;
            if (s->scores[ci] < -1)
                s->scores[ci] = -1;
            if (s->scores[ci] > 100)
                s->scores[ci] = 100;
            courses[ci].enrolled++;
            s->total_credits += courses[ci].credits;
            if (s->scores[ci] >= 0 && s->scores[ci] < PASS_SCORE)
                s->fail_count++;
        }
    }
}

void sort_students(Student* sorted, int count)
{
    memcpy(sorted, students, count * sizeof(Student));
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

// --- UI Logic ---

void refresh_course_table()
{
    if (!g_widgets.course_table)
        return;
    GtkListStore* store = GTK_LIST_STORE(
        gtk_tree_view_get_model(GTK_TREE_VIEW(g_widgets.course_table)));
    gtk_list_store_clear(store);
    for (int i = 0; i < MAX_COURSES; i++)
    {
        if (courses[i].id == 0)
            continue;
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, courses[i].id, 1, courses[i].name,
                           2, courses[i].capacity, 3, courses[i].enrolled, 4,
                           courses[i].credits, -1);
    }
}

void refresh_student_table()
{
    GtkTreeView* view = GTK_TREE_VIEW(
        g_object_get_data(G_OBJECT(g_widgets.notebook), "student_table_view"));
    if (!view)
        return;
    GtkListStore* store = GTK_LIST_STORE(gtk_tree_view_get_model(view));
    gtk_list_store_clear(store);
    for (int i = 0; i < student_count; i++)
    {
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, i + 1, 1, students[i].name, 2,
                           students[i].student_id, 3, students[i].gender, 4,
                           students[i].num_courses, 5,
                           students[i].total_credits, -1);
    }
}

void refresh_student_combo(GtkComboBoxText* combo)
{
    if (!combo)
        return;
    gint active = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
    gtk_combo_box_text_remove_all(combo);
    for (int i = 0; i < student_count; i++)
    {
        gchar* text = g_strdup_printf("%s (%s)", students[i].name,
                                      students[i].student_id);
        gtk_combo_box_text_append(combo, NULL, text);
        g_free(text);
    }
    if (active >= 0 && active < student_count)
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), active);
    else if (student_count > 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
    else
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), -1);
}

void refresh_selection_table()
{
    GtkTreeView* view = GTK_TREE_VIEW(g_object_get_data(
        G_OBJECT(g_widgets.selection_view), "selection_table"));
    if (!view)
        return;
    GtkListStore* store = GTK_LIST_STORE(gtk_tree_view_get_model(view));
    gtk_list_store_clear(store);
    for (int i = 0; i < MAX_COURSES; i++)
    {
        if (courses[i].id == 0)
            continue;
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, courses[i].id, 1, courses[i].name,
                           2, courses[i].capacity, 3, courses[i].enrolled, 4,
                           courses[i].credits, -1);
    }
}

void refresh_student_info_label()
{
    GtkLabel* label = GTK_LABEL(
        g_object_get_data(G_OBJECT(g_widgets.selection_view), "info_label"));
    GtkComboBoxText* combo = GTK_COMBO_BOX_TEXT(
        g_object_get_data(G_OBJECT(g_widgets.selection_view), "student_combo"));
    if (!label || !combo)
        return;
    gint active = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
    if (active < 0 || active >= student_count)
    {
        gtk_label_set_text(label, "请选择学生");
        return;
    }
    Student* s = &students[active];
    GString* info = g_string_new("");
    g_string_append_printf(
        info,
        "学生: %s | 学号: %s | 性别: %s | 已选: %d 门 | 学分: %d | 课程: ",
        s->name, s->student_id, s->gender, s->num_courses, s->total_credits);
    for (int i = 0; i < s->num_courses; i++)
    {
        int ci = get_course_index(s->courses[i]);
        if (ci >= 0)
            g_string_append_printf(info, "%s ", courses[ci].name);
    }
    gtk_label_set_text(label, info->str);
    g_string_free(info, TRUE);
}

void refresh_grade_table()
{
    GtkTreeView* view = GTK_TREE_VIEW(
        g_object_get_data(G_OBJECT(g_widgets.grade_view), "grade_table"));
    GtkComboBoxText* combo = GTK_COMBO_BOX_TEXT(
        g_object_get_data(G_OBJECT(g_widgets.grade_view), "student_combo"));
    if (!view || !combo)
        return;

    gint active = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
    if (active < 0 || active >= student_count)
    {
        GtkListStore* store = GTK_LIST_STORE(gtk_tree_view_get_model(view));
        gtk_list_store_clear(store);
        return;
    }
    Student* s = &students[active];
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
        gtk_list_store_set(store, &iter, 0, cid, 1, courses[ci].name, 2,
                           score_text, 3, status_text, -1);
        g_free(score_text);
        g_free(status_text);
    }
}

void refresh_stats_table(gboolean sorted)
{
    GtkTreeView* view = GTK_TREE_VIEW(
        g_object_get_data(G_OBJECT(g_widgets.stats_view), "stats_table"));
    if (!view)
        return;
    GtkListStore* store = GTK_LIST_STORE(gtk_tree_view_get_model(view));
    gtk_list_store_clear(store);
    Student sorted_arr[MAX_STUDENTS];
    if (sorted)
        sort_students(sorted_arr, student_count);
    for (int i = 0; i < student_count; i++)
    {
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        const Student* s = sorted ? &sorted_arr[i] : &students[i];
        gtk_list_store_set(store, &iter, 0, i + 1, 1, s->name, 2, s->student_id,
                           3, s->gender, 4, s->num_courses, 5, s->total_credits,
                           6, s->fail_count, -1);
    }
}

void refresh_all_combos()
{
    GtkWidget* sel_combo = GTK_WIDGET(
        g_object_get_data(G_OBJECT(g_widgets.selection_view), "student_combo"));
    GtkWidget* grade_combo = GTK_WIDGET(
        g_object_get_data(G_OBJECT(g_widgets.grade_view), "student_combo"));
    if (sel_combo)
        refresh_student_combo(GTK_COMBO_BOX_TEXT(sel_combo));
    if (grade_combo)
        refresh_student_combo(GTK_COMBO_BOX_TEXT(grade_combo));
}

void refresh_all_views()
{
    if (!g_widgets.notebook)
        return;
    refresh_course_table();
    refresh_student_table();
    refresh_selection_table();
    refresh_all_combos();
    refresh_stats_table(FALSE);
    refresh_student_info_label();
    refresh_grade_table();
    gboolean has_students = student_count > 0;
    gboolean has_courses = course_count > 0;
    if (g_widgets.course_edit_btn)
        gtk_widget_set_sensitive(g_widgets.course_edit_btn, has_courses);
    if (g_widgets.course_delete_btn)
        gtk_widget_set_sensitive(g_widgets.course_delete_btn, has_courses);
    if (g_widgets.student_edit_btn)
        gtk_widget_set_sensitive(g_widgets.student_edit_btn, has_students);
    if (g_widgets.student_delete_btn)
        gtk_widget_set_sensitive(g_widgets.student_delete_btn, has_students);
    if (g_widgets.selection_add_btn)
        gtk_widget_set_sensitive(g_widgets.selection_add_btn,
                                 has_students && has_courses);
    if (g_widgets.selection_drop_btn)
        gtk_widget_set_sensitive(g_widgets.selection_drop_btn,
                                 has_students && has_courses);
    if (g_widgets.selection_finish_btn)
        gtk_widget_set_sensitive(g_widgets.selection_finish_btn, has_students);
    GtkWidget* sel_combo = GTK_WIDGET(
        g_object_get_data(G_OBJECT(g_widgets.selection_view), "student_combo"));
    GtkWidget* grade_combo = GTK_WIDGET(
        g_object_get_data(G_OBJECT(g_widgets.grade_view), "student_combo"));
    if (sel_combo)
        gtk_widget_set_sensitive(sel_combo, has_students);
    if (grade_combo)
        gtk_widget_set_sensitive(grade_combo, has_students);
    if (g_widgets.grade_save_btn)
        gtk_widget_set_sensitive(g_widgets.grade_save_btn, has_students);
}

static void show_message(GtkMessageType type, const gchar* msg)
{
    GtkWidget* d = gtk_message_dialog_new(GTK_WINDOW(g_widgets.window),
                                          GTK_DIALOG_MODAL, type,
                                          GTK_BUTTONS_OK, "%s", msg);
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
}

static void on_save_all(GtkWidget* widget, gpointer user_data)
{
    save_courses("courses.txt");
    save_students("students.txt");
}

static void on_save_grades(GtkButton* btn, gpointer user_data)
{
    save_students("students.txt");
    refresh_all_views();
}

// --- Callbacks ---

static void on_add_course_to_system(GtkButton* btn, gpointer user_data)
{
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "添加课程", GTK_WINDOW(g_widgets.window), GTK_DIALOG_MODAL, "_取消",
        GTK_RESPONSE_CANCEL, "_确定", GTK_RESPONSE_ACCEPT, NULL);
    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    GtkWidget *n_entry = gtk_entry_new(), *c_entry = gtk_entry_new(),
              *cr_entry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("课程名称:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), n_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("容量:"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), c_entry, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("学分:"), 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), cr_entry, 1, 2, 1, 1);
    gtk_container_add(GTK_CONTAINER(content), grid);
    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        const gchar* name = gtk_entry_get_text(GTK_ENTRY(n_entry));
        int cap = atoi(gtk_entry_get_text(GTK_ENTRY(c_entry)));
        int cred = atoi(gtk_entry_get_text(GTK_ENTRY(cr_entry)));
        if (!name || !strlen(name))
        {
            show_message(GTK_MESSAGE_WARNING, "课程名称不能为空");
            gtk_widget_destroy(dialog);
            return;
        }
        if ((int)strlen(name) >= MAX_CNAME_LEN)
        {
            show_message(GTK_MESSAGE_WARNING, "课程名称过长");
            gtk_widget_destroy(dialog);
            return;
        }
        if (cap <= 0 || cred <= 0)
        {
            show_message(GTK_MESSAGE_WARNING, "容量和学分必须为正数");
            gtk_widget_destroy(dialog);
            return;
        }
        int slot = -1;
        for (int i = 0; i < MAX_COURSES; i++)
        {
            if (courses[i].id == 0)
            {
                slot = i;
                break;
            }
        }
        if (slot < 0)
        {
            show_message(GTK_MESSAGE_WARNING, "课程数量已达上限");
            gtk_widget_destroy(dialog);
            return;
        }
        courses[slot].id = slot + 1;
        strncpy(courses[slot].name, name, MAX_CNAME_LEN - 1);
        courses[slot].name[MAX_CNAME_LEN - 1] = '\0';
        courses[slot].capacity = cap;
        courses[slot].credits = cred;
        courses[slot].enrolled = 0;
        for (int i = 0; i < student_count; i++)
            students[i].scores[slot] = -1;
        recalc_all_data();
        save_courses("courses.txt");
        save_students("students.txt");
        refresh_all_views();
    }
    gtk_widget_destroy(dialog);
}

static void on_edit_course(GtkButton* btn, gpointer user_data)
{
    GtkTreeSelection* sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(g_widgets.course_table));
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
        "修改名称", GTK_WINDOW(g_widgets.window), GTK_DIALOG_MODAL, "_取消",
        GTK_RESPONSE_CANCEL, "_确定", GTK_RESPONSE_ACCEPT, NULL);
    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), courses[ci].name);
    gtk_container_add(GTK_CONTAINER(content), entry);
    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        const gchar* t = gtk_entry_get_text(GTK_ENTRY(entry));
        if (t && strlen(t))
        {
            strncpy(courses[ci].name, t, MAX_CNAME_LEN - 1);
            courses[ci].name[MAX_CNAME_LEN - 1] = '\0';
            save_courses("courses.txt");
            refresh_all_views();
        }
    }
    gtk_widget_destroy(dialog);
}

static void on_delete_course(GtkButton* btn, gpointer user_data)
{
    GtkTreeSelection* sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(g_widgets.course_table));
    GtkTreeModel* model;
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected(sel, &model, &iter))
        return;
    gint id;
    gtk_tree_model_get(model, &iter, 0, &id, -1);
    int ci = get_course_index(id);
    if (ci < 0)
        return;

    GtkWidget* confirm = gtk_message_dialog_new(
        GTK_WINDOW(g_widgets.window), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING,
        GTK_BUTTONS_OK_CANCEL, "确认删除该课程?");
    int resp = gtk_dialog_run(GTK_DIALOG(confirm));
    gtk_widget_destroy(confirm);
    if (resp != GTK_RESPONSE_OK)
        return;

    courses[ci].id = 0;
    courses[ci].name[0] = '\0';
    courses[ci].capacity = 0;
    courses[ci].credits = 0;
    courses[ci].enrolled = 0;
    for (int i = 0; i < student_count; i++)
        students[i].scores[ci] = -1;
    recalc_all_data();
    save_courses("courses.txt");
    save_students("students.txt");
    refresh_all_views();
}

static void on_add_student(GtkButton* btn, gpointer user_data)
{
    if (student_count >= MAX_STUDENTS)
        return;
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "添加学生", GTK_WINDOW(g_widgets.window), GTK_DIALOG_MODAL, "_取消",
        GTK_RESPONSE_CANCEL, "_确定", GTK_RESPONSE_ACCEPT, NULL);
    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    GtkWidget *n_entry = gtk_entry_new(), *i_entry = gtk_entry_new(),
              *g_combo = gtk_combo_box_text_new();
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
        if (!name || !strlen(name) || !id || !strlen(id))
        {
            show_message(GTK_MESSAGE_WARNING, "姓名和学号不能为空");
        }
        else if ((int)strlen(name) >= MAX_NAME_LEN)
        {
            show_message(GTK_MESSAGE_WARNING, "姓名过长");
        }
        else if ((int)strlen(id) >= MAX_ID_LEN)
        {
            show_message(GTK_MESSAGE_WARNING, "学号过长");
        }
        else if (find_student_by_id(id) >= 0)
        {
            show_message(GTK_MESSAGE_WARNING, "学号已存在");
        }
        else
        {
            Student* s = &students[student_count++];
            memset(s, 0, sizeof(Student));
            for (int j = 0; j < MAX_COURSES; j++)
                s->scores[j] = -1;
            strncpy(s->name, name, MAX_NAME_LEN - 1);
            s->name[MAX_NAME_LEN - 1] = '\0';
            strncpy(s->student_id, id, MAX_ID_LEN - 1);
            s->student_id[MAX_ID_LEN - 1] = '\0';
            if (gender && strlen(gender))
            {
                strncpy(s->gender, gender, 5);
                s->gender[5] = '\0';
            }
            recalc_all_data();
            save_students("students.txt");
            save_courses("courses.txt");
            refresh_all_views();
        }
        if (gender)
            g_free(gender);
    }
    gtk_widget_destroy(dialog);
}

static void on_edit_student(GtkButton* btn, gpointer user_data)
{
    GtkTreeView* view = GTK_TREE_VIEW(
        g_object_get_data(G_OBJECT(g_widgets.notebook), "student_table_view"));
    if (!view)
        return;
    GtkTreeSelection* sel = gtk_tree_view_get_selection(view);
    GtkTreeModel* model;
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected(sel, &model, &iter))
        return;
    gchar* id_text = NULL;
    gtk_tree_model_get(model, &iter, 2, &id_text, -1);
    if (!id_text)
        return;
    int s_idx = find_student_by_id(id_text);
    g_free(id_text);
    if (s_idx < 0)
        return;

    Student* s = &students[s_idx];
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "修改学生信息", GTK_WINDOW(g_widgets.window), GTK_DIALOG_MODAL, "_取消",
        GTK_RESPONSE_CANCEL, "_确定", GTK_RESPONSE_ACCEPT, NULL);
    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    GtkWidget *n_entry = gtk_entry_new(), *i_entry = gtk_entry_new(),
              *g_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_combo), "男");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_combo), "女");
    gtk_entry_set_text(GTK_ENTRY(n_entry), s->name);
    gtk_entry_set_text(GTK_ENTRY(i_entry), s->student_id);
    if (strcmp(s->gender, "女") == 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(g_combo), 1);
    else
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
        if (!name || !strlen(name) || !id || !strlen(id))
        {
            show_message(GTK_MESSAGE_WARNING, "姓名和学号不能为空");
        }
        else if ((int)strlen(name) >= MAX_NAME_LEN)
        {
            show_message(GTK_MESSAGE_WARNING, "姓名过长");
        }
        else if ((int)strlen(id) >= MAX_ID_LEN)
        {
            show_message(GTK_MESSAGE_WARNING, "学号过长");
        }
        else
        {
            int existing = find_student_by_id(id);
            if (existing >= 0 && existing != s_idx)
            {
                show_message(GTK_MESSAGE_WARNING, "学号已存在");
            }
            else
            {
                strncpy(s->name, name, MAX_NAME_LEN - 1);
                s->name[MAX_NAME_LEN - 1] = '\0';
                strncpy(s->student_id, id, MAX_ID_LEN - 1);
                s->student_id[MAX_ID_LEN - 1] = '\0';
                if (gender && strlen(gender))
                {
                    strncpy(s->gender, gender, 5);
                    s->gender[5] = '\0';
                }
                recalc_all_data();
                save_students("students.txt");
                save_courses("courses.txt");
                refresh_all_views();
            }
        }
        if (gender)
            g_free(gender);
    }
    gtk_widget_destroy(dialog);
}

static void on_delete_student(GtkButton* btn, gpointer user_data)
{
    GtkTreeView* view = GTK_TREE_VIEW(
        g_object_get_data(G_OBJECT(g_widgets.notebook), "student_table_view"));
    if (!view)
        return;
    GtkTreeSelection* sel = gtk_tree_view_get_selection(view);
    GtkTreeModel* model;
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected(sel, &model, &iter))
        return;
    gchar* id_text = NULL;
    gtk_tree_model_get(model, &iter, 2, &id_text, -1);
    if (!id_text)
        return;
    int s_idx = find_student_by_id(id_text);
    g_free(id_text);
    if (s_idx < 0)
        return;

    GtkWidget* confirm = gtk_message_dialog_new(
        GTK_WINDOW(g_widgets.window), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING,
        GTK_BUTTONS_OK_CANCEL, "确认删除该学生?");
    int resp = gtk_dialog_run(GTK_DIALOG(confirm));
    gtk_widget_destroy(confirm);
    if (resp != GTK_RESPONSE_OK)
        return;

    for (int i = s_idx; i < student_count - 1; i++)
        students[i] = students[i + 1];
    student_count--;
    recalc_all_data();
    save_students("students.txt");
    save_courses("courses.txt");
    refresh_all_views();
}

static void on_add_course(GtkButton* btn, gpointer user_data)
{
    gint s_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(g_object_get_data(
        G_OBJECT(g_widgets.selection_view), "student_combo")));
    GtkWidget* table = GTK_WIDGET(g_object_get_data(
        G_OBJECT(g_widgets.selection_view), "selection_table"));
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
    Student* s = &students[s_idx];
    if (s->num_courses >= MAX_SELECTS)
    {
        GtkWidget* d = gtk_message_dialog_new(
            GTK_WINDOW(g_widgets.window), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING,
            GTK_BUTTONS_OK, "上限(5门)!");
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        return;
    }
    if (courses[ci].enrolled >= courses[ci].capacity)
    {
        GtkWidget* d = gtk_message_dialog_new(
            GTK_WINDOW(g_widgets.window), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING,
            GTK_BUTTONS_OK, "已满!");
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        return;
    }
    for (int i = 0; i < s->num_courses; i++)
        if (s->courses[i] == cid)
        {
            GtkWidget* d = gtk_message_dialog_new(
                GTK_WINDOW(g_widgets.window), GTK_DIALOG_MODAL,
                GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "已选!");
            gtk_dialog_run(GTK_DIALOG(d));
            gtk_widget_destroy(d);
            return;
        }
    s->courses[s->num_courses++] = cid;
    recalc_all_data();
    save_students("students.txt");
    save_courses("courses.txt");
    refresh_all_views();
}

static void on_drop_course(GtkButton* btn, gpointer user_data)
{
    gint s_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(g_object_get_data(
        G_OBJECT(g_widgets.selection_view), "student_combo")));
    GtkWidget* table = GTK_WIDGET(g_object_get_data(
        G_OBJECT(g_widgets.selection_view), "selection_table"));
    GtkTreeSelection* sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(table));
    GtkTreeModel* model;
    GtkTreeIter iter;
    if (s_idx < 0 || !gtk_tree_selection_get_selected(sel, &model, &iter))
        return;
    gint cid;
    gtk_tree_model_get(model, &iter, 0, &cid, -1);
    Student* s = &students[s_idx];
    if (s->num_courses <= 3)
    {
        GtkWidget* d = gtk_message_dialog_new(
            GTK_WINDOW(g_widgets.window), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING,
            GTK_BUTTONS_OK, "至少保留3门!");
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        return;
    }
    for (int i = 0; i < s->num_courses; i++)
    {
        if (s->courses[i] == cid)
        {
            for (int j = i; j < s->num_courses - 1; j++)
                s->courses[j] = s->courses[j + 1];
            s->num_courses--;
            recalc_all_data();
            save_students("students.txt");
            save_courses("courses.txt");
            refresh_all_views();
            return;
        }
    }
    GtkWidget* d = gtk_message_dialog_new(GTK_WINDOW(g_widgets.window),
                                          GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING,
                                          GTK_BUTTONS_OK, "未选该课!");
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
}

static void on_finish_selection(GtkButton* btn, gpointer user_data)
{
    gint s_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(g_object_get_data(
        G_OBJECT(g_widgets.selection_view), "student_combo")));
    if (s_idx < 0)
        return;
    Student* s = &students[s_idx];
    if (s->num_courses < 3)
        return;
    recalc_all_data();
    save_students("students.txt");
    save_courses("courses.txt");
    refresh_all_views();
}

static void on_student_combo_changed(GtkComboBox* combo, gpointer user_data)
{
    refresh_student_info_label();
    refresh_grade_table();
}

static void on_grade_cell_edited(GtkCellRendererText* cell, gchar* path_string,
                                 gchar* new_text, gpointer user_data)
{
    GtkWidget* table =
        g_object_get_data(G_OBJECT(g_widgets.grade_view), "grade_table");
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(table));
    GtkTreePath* path = gtk_tree_path_new_from_string(path_string);
    GtkTreeIter iter;
    gtk_tree_model_get_iter(model, &iter, path);
    gint cid;
    gtk_tree_model_get(model, &iter, 0, &cid, -1);
    gint s_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(
        g_object_get_data(G_OBJECT(g_widgets.grade_view), "student_combo")));
    if (s_idx < 0)
        return;
    Student* s = &students[s_idx];
    int score = atoi(new_text);
    if (score < 0)
        score = 0;
    if (score > 100)
        score = 100;
    int ci = get_course_index(cid);
    if (ci >= 0)
    {
        s->scores[ci] = score;
        recalc_all_data();
        gchar* score_text = g_strdup_printf("%d", score);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, 2, score_text, 3,
                           score >= PASS_SCORE ? "及格" : "不及格", -1);
        g_free(score_text);
        save_students("students.txt");
        refresh_all_views();
        // Keep selection
        GtkWidget* combo = GTK_WIDGET(
            g_object_get_data(G_OBJECT(g_widgets.grade_view), "student_combo"));
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), s_idx);
    }
    gtk_tree_path_free(path);
}

static void on_sort_by_credits(GtkButton* btn, gpointer user_data)
{
    refresh_stats_table(TRUE);
}

static void on_fail_stats(GtkButton* btn, gpointer user_data)
{
    GString* res = g_string_new("===== 不及格学生统计 =====\n\n");
    g_string_append(res, "--- 1 门不及格 ---\n");
    gboolean found1 = FALSE;
    for (int i = 0; i < student_count; i++)
    {
        if (students[i].fail_count == 1)
        {
            g_string_append_printf(res, "  %s (%s) - 科目: ", students[i].name,
                                   students[i].student_id);
            for (int j = 0; j < students[i].num_courses; j++)
            {
                int ci = get_course_index(students[i].courses[j]);
                if (ci >= 0 && students[i].scores[ci] >= 0 &&
                    students[i].scores[ci] < PASS_SCORE)
                    g_string_append_printf(res, "%s(%d分) ", courses[ci].name,
                                           students[i].scores[ci]);
            }
            found1 = TRUE;
            g_string_append(res, "\n");
        }
    }
    if (!found1)
        g_string_append(res, "  (无)\n");
    g_string_append(res, "\n--- 3 门不及格 ---\n");
    gboolean found3 = FALSE;
    for (int i = 0; i < student_count; i++)
    {
        if (students[i].fail_count == 3)
        {
            g_string_append_printf(res, "  %s (%s)\n", students[i].name,
                                   students[i].student_id);
            found3 = TRUE;
        }
    }
    if (!found3)
        g_string_append(res, "  (无)\n");
    GtkWidget* dialog = gtk_message_dialog_new(
        GTK_WINDOW(g_widgets.window), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK, "%s", res->str);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_string_free(res, TRUE);
}

void create_ui(int argc, char* argv[])
{
    gtk_init(&argc, &argv);
    g_widgets.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(g_widgets.window),
                         "学生选课及学籍管理系统");
    gtk_window_set_default_size(GTK_WINDOW(g_widgets.window), 1000, 700);
    g_signal_connect(g_widgets.window, "destroy", G_CALLBACK(gtk_main_quit),
                     NULL);
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget* menubar = gtk_menu_bar_new();
    GtkWidget* file_menu = gtk_menu_item_new_with_label("文件");
    GtkWidget* file_submenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_menu), file_submenu);
    GtkWidget* save_item = gtk_menu_item_new_with_label("保存");
    GtkWidget* exit_item = gtk_menu_item_new_with_label("退出");
    gtk_menu_shell_append(GTK_MENU_SHELL(file_submenu), save_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_submenu), exit_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), GTK_WIDGET(file_menu));
    g_signal_connect(save_item, "activate", G_CALLBACK(on_save_all), NULL);
    g_signal_connect(exit_item, "activate", G_CALLBACK(gtk_main_quit), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
    g_widgets.notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(vbox), g_widgets.notebook, TRUE, TRUE, 0);

    // Course View
    GtkWidget* course_view = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(course_view), 10);
    g_widgets.course_table = gtk_tree_view_new();
    GtkListStore* c_store = gtk_list_store_new(
        5, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT);
    gtk_tree_view_set_model(GTK_TREE_VIEW(g_widgets.course_table),
                            GTK_TREE_MODEL(c_store));
    g_object_unref(c_store);
    const char* c_titles[] = {"编号", "课程名称", "容量", "已选", "学分"};
    for (int i = 0; i < 5; i++)
        gtk_tree_view_append_column(
            GTK_TREE_VIEW(g_widgets.course_table),
            gtk_tree_view_column_new_with_attributes(
                c_titles[i], gtk_cell_renderer_text_new(), "text", i, NULL));
    GtkWidget* c_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(c_scroll, TRUE);
    gtk_container_add(GTK_CONTAINER(c_scroll), g_widgets.course_table);
    gtk_box_pack_start(GTK_BOX(course_view), c_scroll, TRUE, TRUE, 0);
    GtkWidget* c_btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget* add_c_btn = gtk_button_new_with_label("添加课程");
    GtkWidget* edit_c_btn = gtk_button_new_with_label("修改名称");
    GtkWidget* del_c_btn = gtk_button_new_with_label("删除课程");
    gtk_box_pack_start(GTK_BOX(c_btn_box), add_c_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(c_btn_box), edit_c_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(c_btn_box), del_c_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(course_view), c_btn_box, FALSE, FALSE, 5);
    gtk_notebook_append_page(GTK_NOTEBOOK(g_widgets.notebook), course_view,
                             gtk_label_new("课程管理"));
    g_widgets.course_add_btn = add_c_btn;
    g_widgets.course_edit_btn = edit_c_btn;
    g_widgets.course_delete_btn = del_c_btn;
    g_signal_connect(add_c_btn, "clicked", G_CALLBACK(on_add_course_to_system),
                     NULL);
    g_signal_connect(edit_c_btn, "clicked", G_CALLBACK(on_edit_course), NULL);
    g_signal_connect(del_c_btn, "clicked", G_CALLBACK(on_delete_course), NULL);

    // Student View
    GtkWidget* student_view = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(student_view), 10);
    GtkWidget* student_table = gtk_tree_view_new();
    GtkListStore* s_store =
        gtk_list_store_new(6, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING,
                           G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);
    gtk_tree_view_set_model(GTK_TREE_VIEW(student_table),
                            GTK_TREE_MODEL(s_store));
    g_object_unref(s_store);
    g_object_set_data(G_OBJECT(g_widgets.notebook), "student_table_view",
                      student_table);
    const char* s_titles[] = {"序号", "姓名", "学号", "性别", "选课", "学分"};
    for (int i = 0; i < 6; i++)
        gtk_tree_view_append_column(
            GTK_TREE_VIEW(student_table),
            gtk_tree_view_column_new_with_attributes(
                s_titles[i], gtk_cell_renderer_text_new(), "text", i, NULL));
    GtkWidget* s_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(s_scroll, TRUE);
    gtk_container_add(GTK_CONTAINER(s_scroll), student_table);
    gtk_box_pack_start(GTK_BOX(student_view), s_scroll, TRUE, TRUE, 0);
    GtkWidget* s_btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget* s_add_btn = gtk_button_new_with_label("添加学生");
    GtkWidget* s_edit_btn = gtk_button_new_with_label("修改学生信息");
    GtkWidget* s_del_btn = gtk_button_new_with_label("删除学生");
    gtk_box_pack_start(GTK_BOX(s_btn_box), s_add_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(s_btn_box), s_edit_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(s_btn_box), s_del_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(student_view), s_btn_box, FALSE, FALSE, 5);
    g_widgets.student_add_btn = s_add_btn;
    g_widgets.student_edit_btn = s_edit_btn;
    g_widgets.student_delete_btn = s_del_btn;
    g_signal_connect(s_add_btn, "clicked", G_CALLBACK(on_add_student), NULL);
    g_signal_connect(s_edit_btn, "clicked", G_CALLBACK(on_edit_student), NULL);
    g_signal_connect(s_del_btn, "clicked", G_CALLBACK(on_delete_student), NULL);
    gtk_notebook_append_page(GTK_NOTEBOOK(g_widgets.notebook), student_view,
                             gtk_label_new("学生管理"));

    // Selection View
    GtkWidget* sel_view = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(sel_view), 10);
    g_widgets.selection_view = sel_view;
    GtkWidget* sel_combo = gtk_combo_box_text_new();
    GtkWidget* sel_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(sel_hbox), gtk_label_new("学生:"), FALSE, FALSE,
                       5);
    gtk_box_pack_start(GTK_BOX(sel_hbox), sel_combo, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(sel_view), sel_hbox, FALSE, FALSE, 5);
    GtkWidget* sel_table = gtk_tree_view_new();
    GtkListStore* sel_store = gtk_list_store_new(
        5, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT);
    gtk_tree_view_set_model(GTK_TREE_VIEW(sel_table),
                            GTK_TREE_MODEL(sel_store));
    g_object_unref(sel_store);
    for (int i = 0; i < 5; i++)
        gtk_tree_view_append_column(
            GTK_TREE_VIEW(sel_table),
            gtk_tree_view_column_new_with_attributes(
                c_titles[i], gtk_cell_renderer_text_new(), "text", i, NULL));
    GtkWidget* sel_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(sel_scroll, TRUE);
    gtk_container_add(GTK_CONTAINER(sel_scroll), sel_table);
    gtk_box_pack_start(GTK_BOX(sel_view), sel_scroll, TRUE, TRUE, 5);
    GtkWidget* info_label = gtk_label_new("待选课");
    gtk_box_pack_start(GTK_BOX(sel_view), info_label, FALSE, FALSE, 5);
    GtkWidget* b_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget* add_btn = gtk_button_new_with_label("添加");
    GtkWidget* drop_btn = gtk_button_new_with_label("退选");
    GtkWidget* fin_btn = gtk_button_new_with_label("完成");
    gtk_box_pack_start(GTK_BOX(b_box), add_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(b_box), drop_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(b_box), fin_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(sel_view), b_box, FALSE, FALSE, 5);
    g_widgets.selection_add_btn = add_btn;
    g_widgets.selection_drop_btn = drop_btn;
    g_widgets.selection_finish_btn = fin_btn;
    g_object_set_data(G_OBJECT(sel_view), "student_combo", sel_combo);
    g_object_set_data(G_OBJECT(sel_view), "selection_table", sel_table);
    g_object_set_data(G_OBJECT(sel_view), "info_label", info_label);
    gtk_notebook_append_page(GTK_NOTEBOOK(g_widgets.notebook), sel_view,
                             gtk_label_new("选课操作"));
    g_signal_connect(sel_combo, "changed", G_CALLBACK(on_student_combo_changed),
                     NULL);
    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_course), NULL);
    g_signal_connect(drop_btn, "clicked", G_CALLBACK(on_drop_course), NULL);
    g_signal_connect(fin_btn, "clicked", G_CALLBACK(on_finish_selection), NULL);

    // Grade View
    GtkWidget* grade_view = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(grade_view), 10);
    g_widgets.grade_view = grade_view;
    GtkWidget* grade_combo = gtk_combo_box_text_new();
    GtkWidget* g_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(g_hbox), gtk_label_new("学生:"), FALSE, FALSE,
                       5);
    gtk_box_pack_start(GTK_BOX(g_hbox), grade_combo, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(grade_view), g_hbox, FALSE, FALSE, 5);
    GtkWidget* grade_tbl = gtk_tree_view_new();
    GtkListStore* g_store = gtk_list_store_new(4, G_TYPE_INT, G_TYPE_STRING,
                                               G_TYPE_STRING, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(grade_tbl), GTK_TREE_MODEL(g_store));
    g_object_unref(g_store);
    const char* g_titles[] = {"ID", "科目", "成绩", "状态"};
    for (int i = 0; i < 4; i++)
    {
        GtkCellRenderer* r = gtk_cell_renderer_text_new();
        if (i == 2)
        {
            g_object_set(r, "editable", TRUE, NULL);
            g_signal_connect(r, "edited", G_CALLBACK(on_grade_cell_edited),
                             NULL);
        }
        gtk_tree_view_append_column(GTK_TREE_VIEW(grade_tbl),
                                    gtk_tree_view_column_new_with_attributes(
                                        g_titles[i], r, "text", i, NULL));
    }
    GtkWidget* g_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(g_scroll, TRUE);
    gtk_container_add(GTK_CONTAINER(g_scroll), grade_tbl);
    gtk_box_pack_start(GTK_BOX(grade_view), g_scroll, TRUE, TRUE, 5);
    GtkWidget* save_g_btn = gtk_button_new_with_label("保存成绩");
    gtk_box_pack_start(GTK_BOX(grade_view), save_g_btn, FALSE, FALSE, 5);
    g_widgets.grade_save_btn = save_g_btn;
    g_object_set_data(G_OBJECT(grade_view), "student_combo", grade_combo);
    g_object_set_data(G_OBJECT(grade_view), "grade_table", grade_tbl);
    gtk_notebook_append_page(GTK_NOTEBOOK(g_widgets.notebook), grade_view,
                             gtk_label_new("成绩录入"));
    g_signal_connect(grade_combo, "changed",
                     G_CALLBACK(on_student_combo_changed), NULL);
    g_signal_connect(save_g_btn, "clicked", G_CALLBACK(on_save_grades), NULL);

    // Stats View
    GtkWidget* stats_view = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(stats_view), 10);
    g_widgets.stats_view = stats_view;
    GtkWidget* st_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget* sort_btn = gtk_button_new_with_label("排序");
    GtkWidget* fail_btn = gtk_button_new_with_label("不及格");
    gtk_box_pack_start(GTK_BOX(st_hbox), sort_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(st_hbox), fail_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(stats_view), st_hbox, FALSE, FALSE, 5);
    GtkWidget* stats_tbl = gtk_tree_view_new();
    GtkListStore* st_store =
        gtk_list_store_new(7, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING,
                           G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT);
    gtk_tree_view_set_model(GTK_TREE_VIEW(stats_tbl), GTK_TREE_MODEL(st_store));
    g_object_unref(st_store);
    const char* st_titles[] = {"排名", "姓名", "学号",  "性别",
                               "选",   "学分", "不及格"};
    for (int i = 0; i < 7; i++)
        gtk_tree_view_append_column(
            GTK_TREE_VIEW(stats_tbl),
            gtk_tree_view_column_new_with_attributes(
                st_titles[i], gtk_cell_renderer_text_new(), "text", i, NULL));
    GtkWidget* st_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(st_scroll, TRUE);
    gtk_container_add(GTK_CONTAINER(st_scroll), stats_tbl);
    gtk_box_pack_start(GTK_BOX(stats_view), st_scroll, TRUE, TRUE, 0);
    g_object_set_data(G_OBJECT(stats_view), "stats_table", stats_tbl);
    gtk_notebook_append_page(GTK_NOTEBOOK(g_widgets.notebook), stats_view,
                             gtk_label_new("统计排序"));
    g_signal_connect(sort_btn, "clicked", G_CALLBACK(on_sort_by_credits), NULL);
    g_signal_connect(fail_btn, "clicked", G_CALLBACK(on_fail_stats), NULL);

    gtk_container_add(GTK_CONTAINER(g_widgets.window), vbox);
    refresh_all_views();

    // Init combo selection
    gtk_combo_box_set_active(GTK_COMBO_BOX(sel_combo),
                             (student_count > 0) ? 0 : -1);
    gtk_combo_box_set_active(GTK_COMBO_BOX(grade_combo),
                             (student_count > 0) ? 0 : -1);
    refresh_student_info_label();
    refresh_grade_table();

    gtk_widget_show_all(g_widgets.window);
    gtk_main();
}

int main(int argc, char* argv[])
{
    if (!load_courses("courses.txt"))
    {
        init_courses();
        save_courses("courses.txt");
    }
    load_students("students.txt");
    recalc_all_data();
    save_courses("courses.txt");
    save_students("students.txt");
    printf("Loaded %d students\n", student_count);
    create_ui(argc, argv);
    save_courses("courses.txt");
    save_students("students.txt");
    return 0;
}
