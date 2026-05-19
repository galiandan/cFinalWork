#include "data_manager.h"

Course courses[MAX_COURSES];
Student students[MAX_STUDENTS];
int student_count = 0;

void init_courses() {
    const char* names[] = {"数据结构", "操作系统", "计算机网络",
                           "数据库原理", "软件工程", "人工智能", "编译原理"};
    for (int i = 0; i < MAX_COURSES; i++) {
        courses[i].id = i + 1;
        strncpy(courses[i].name, names[i], MAX_CNAME_LEN - 1);
        courses[i].capacity = 10 * (i + 1);
        courses[i].credits = i + 1;
        courses[i].enrolled = 0;
    }
}

int load_courses(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) { init_courses(); return 0; }
    int count = 0;
    char buf[256];
    fgets(buf, sizeof(buf), fp);
    for (int i = 0; i < MAX_COURSES && fgets(buf, sizeof(buf), fp); i++) {
        char* t = strtok(buf, "|\n");
        if (t) courses[i].id = atoi(t);
        t = strtok(NULL, "|\n");
        if (t) strncpy(courses[i].name, t, MAX_CNAME_LEN - 1);
        t = strtok(NULL, "|\n");
        if (t) courses[i].capacity = atoi(t);
        t = strtok(NULL, "|\n");
        if (t) courses[i].enrolled = atoi(t);
        t = strtok(NULL, "|\n");
        if (t) courses[i].credits = atoi(t);
        count++;
    }
    fclose(fp);
    return (count == MAX_COURSES) ? 1 : 0;
}

void save_courses(const char* filename) {
    FILE* fp = fopen(filename, "w");
    if (!fp) return;
    fprintf(fp, "%d\n", MAX_COURSES);
    for (int i = 0; i < MAX_COURSES; i++)
        fprintf(fp, "%d|%s|%d|%d|%d\n", courses[i].id, courses[i].name,
                courses[i].capacity, courses[i].enrolled, courses[i].credits);
    fclose(fp);
}

int load_students(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) return 0;
    
    char buf[1024];
    if (!fgets(buf, sizeof(buf), fp)) { fclose(fp); return 0; }
    student_count = atoi(buf);
    if (student_count <= 0 || student_count > MAX_STUDENTS) { fclose(fp); return 0; }
    
    for (int i = 0; i < student_count; i++) {
        memset(&students[i], 0, sizeof(Student));
        if (!fgets(buf, sizeof(buf), fp)) break;
        
        char line_copy[1024];
        strncpy(line_copy, buf, sizeof(line_copy));
        char* t = strtok(line_copy, "|\n");
        if (t) strncpy(students[i].name, t, MAX_NAME_LEN - 1);
        t = strtok(NULL, "|\n");
        if (t) strncpy(students[i].student_id, t, MAX_ID_LEN - 1);
        t = strtok(NULL, "|\n");
        if (t) strncpy(students[i].gender, t, 5);
        t = strtok(NULL, "|\n");
        if (t) students[i].num_courses = atoi(t);
        for (int j = 0; j < MAX_SELECTS && (t = strtok(NULL, "|\n")); j++)
            students[i].courses[j] = atoi(t);
        for (int j = 0; j < MAX_COURSES && (t = strtok(NULL, "|\n")); j++)
            students[i].scores[j] = atoi(t);
        t = strtok(NULL, "|\n");
        if (t) students[i].total_credits = atoi(t);
        t = strtok(NULL, "|\n");
        if (t) students[i].fail_count = atoi(t);
    }
    fclose(fp);
    return 1;
}

void save_students(const char* filename) {
    FILE* fp = fopen(filename, "w");
    if (!fp) return;
    fprintf(fp, "%d\n", student_count);
    for (int i = 0; i < student_count; i++) {
        Student* s = &students[i];
        fprintf(fp, "%s|%s|%s|%d", s->name, s->student_id, s->gender, s->num_courses);
        for (int j = 0; j < MAX_COURSES; j++)
            if (j < s->num_courses) fprintf(fp, "|%d", s->courses[j]);
            else fprintf(fp, "|-1");
        for (int j = 0; j < MAX_COURSES; j++)
            fprintf(fp, "|%d", s->scores[j]);
        fprintf(fp, "|%d|%d\n", s->total_credits, s->fail_count);
    }
    fclose(fp);
}

int find_student_by_id(const char* id) {
    for (int i = 0; i < student_count; i++)
        if (strcmp(students[i].student_id, id) == 0) return i;
    return -1;
}

int get_course_index(int cid) {
    for (int i = 0; i < MAX_COURSES; i++)
        if (courses[i].id == cid) return i;
    return -1;
}

void increase_enrolled(int cid) {
    int idx = get_course_index(cid);
    if (idx >= 0) courses[idx].enrolled++;
}

void decrease_enrolled(int cid) {
    int idx = get_course_index(cid);
    if (idx >= 0) courses[idx].enrolled--;
}

void compute_fail_count(int idx) {
    Student* s = &students[idx];
    s->fail_count = 0;
    for (int j = 0; j < s->num_courses; j++) {
        int ci = s->courses[j] - 1;
        if (ci >= 0 && ci < MAX_COURSES && s->scores[ci] >= 0 && s->scores[ci] < PASS_SCORE)
            s->fail_count++;
    }
}

void sort_students(Student* sorted, int count) {
    memcpy(sorted, students, count * sizeof(Student));
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (sorted[j].total_credits > sorted[j + 1].total_credits ||
                (sorted[j].total_credits == sorted[j + 1].total_credits &&
                 strcmp(sorted[j].name, sorted[j + 1].name) > 0)) {
                Student temp = sorted[j];
                sorted[j] = sorted[j + 1];
                sorted[j + 1] = temp;
            }
        }
    }
}
