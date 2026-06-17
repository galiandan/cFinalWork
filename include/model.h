#ifndef MODEL_H
#define MODEL_H

#include <gtk/gtk.h>

#define MAX_NAME_LEN 20
#define MAX_ID_LEN 20
#define MAX_CNAME_LEN 50
#define MAX_COURSES 100
#define MAX_STUDENTS 200
#define MAX_SELECTS 5
#define PASS_SCORE 60
#define MAX_SCHEDULE_LEN 30
#define ERROR_MSG_LEN 128

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
    int course_id;
    int score;
} CourseScore;

typedef struct
{
    char name[MAX_NAME_LEN];
    char student_id[MAX_ID_LEN];
    char gender[6];
    int courses[MAX_SELECTS];
    int num_courses;
    CourseScore scores[MAX_COURSES];
    int total_credits;
    int fail_count;
    double gpa;
    int gpa_warning;
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
    int course_count;
    Student students[MAX_STUDENTS];
    int student_count;
    int next_course_id;
    AppWidgets widgets;
} AppState;

void app_state_init(AppState* state);

#endif
