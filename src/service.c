#include "service.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void set_error(char* err, size_t err_size, const char* message)
{
    if (!err || err_size == 0)
        return;
    snprintf(err, err_size, "%s", message ? message : "");
}

static void copy_text(char* dest, size_t dest_size, const char* src)
{
    if (!dest || dest_size == 0)
        return;
    if (!src)
        src = "";
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
}

static int find_score_slot(const Student* student, int course_id)
{
    for (int i = 0; i < MAX_COURSES; i++)
    {
        if (student->scores[i].course_id == course_id)
            return i;
    }
    return -1;
}

static int ensure_score_slot(Student* student, int course_id)
{
    int free_slot = -1;

    for (int i = 0; i < MAX_COURSES; i++)
    {
        if (student->scores[i].course_id == course_id)
            return i;
        if (free_slot < 0 && student->scores[i].course_id < 0)
            free_slot = i;
    }
    if (free_slot >= 0)
    {
        student->scores[free_slot].course_id = course_id;
        student->scores[free_slot].score = -1;
    }
    return free_slot;
}

static void clear_score_for_course(Student* student, int course_id)
{
    int slot = find_score_slot(student, course_id);

    if (slot < 0)
        return;
    student->scores[slot].course_id = -1;
    student->scores[slot].score = -1;
}

static double score_to_points(int score)
{
    if (score >= 90)
        return 4.0;
    if (score >= 85)
        return 3.7;
    if (score >= 82)
        return 3.3;
    if (score >= 78)
        return 3.0;
    if (score >= 75)
        return 2.7;
    if (score >= 72)
        return 2.3;
    if (score >= 68)
        return 2.0;
    if (score >= 64)
        return 1.5;
    if (score >= 60)
        return 1.0;
    return 0.0;
}

static void parse_schedule_day(const char* schedule, int* day)
{
    const char* days[] = {"周一", "周二", "周三", "周四", "周五", "周六", "周日"};

    *day = -1;
    if (!schedule)
        return;
    for (int i = 0; i < 7; i++)
    {
        if (strstr(schedule, days[i]) != NULL)
        {
            *day = i;
            return;
        }
    }
}

static void parse_schedule_slots(const char* schedule, int* start, int* end)
{
    const char* marker;
    const char* cursor;
    const char* dash;

    *start = -1;
    *end = -1;
    if (!schedule)
        return;

    marker = strstr(schedule, "节");
    if (!marker)
        return;

    cursor = marker - 1;
    while (cursor > schedule && *cursor >= '0' && *cursor <= '9')
        cursor--;
    if (!(*cursor >= '0' && *cursor <= '9'))
        cursor++;
    if (!(*cursor >= '0' && *cursor <= '9'))
        return;

    dash = strchr(cursor, '-');
    if (dash && dash < marker)
    {
        *start = atoi(cursor);
        *end = atoi(dash + 1);
        return;
    }

    *start = atoi(cursor);
    *end = *start;
}

static void normalize_student_courses(AppState* state, Student* student)
{
    int normalized[MAX_SELECTS];
    int normalized_count = 0;

    for (int i = 0; i < MAX_SELECTS; i++)
        normalized[i] = -1;

    for (int i = 0; i < MAX_SELECTS; i++)
    {
        int course_id = student->courses[i];
        int duplicate = 0;

        if (service_get_course_index(state, course_id) < 0)
            continue;

        for (int j = 0; j < normalized_count; j++)
        {
            if (normalized[j] == course_id)
            {
                duplicate = 1;
                break;
            }
        }
        if (duplicate)
            continue;

        normalized[normalized_count++] = course_id;
    }

    for (int i = 0; i < MAX_SELECTS; i++)
        student->courses[i] = normalized[i];
    student->num_courses = normalized_count;

    for (int i = 0; i < MAX_COURSES; i++)
    {
        int keep = 0;
        int course_id = student->scores[i].course_id;

        if (course_id < 0)
            continue;
        for (int j = 0; j < student->num_courses; j++)
        {
            if (student->courses[j] == course_id)
            {
                keep = 1;
                break;
            }
        }
        if (!keep || service_get_course_index(state, course_id) < 0)
        {
            student->scores[i].course_id = -1;
            student->scores[i].score = -1;
        }
    }
    for (int i = 0; i < student->num_courses; i++)
        ensure_score_slot(student, student->courses[i]);
}

void service_init_default_courses(AppState* state)
{
    const char* names[] = {"数据结构", "操作系统", "计算机网络", "数据库原理",
                           "软件工程", "人工智能", "编译原理"};
    const char* schedules[] = {"周一 1-2节", "周二 3-4节", "周三 5-6节",
                               "周一 3-4节", "周二 1-2节", "周三 1-2节",
                               "周四 5-6节"};
    int defaults = (int)(sizeof(names) / sizeof(names[0]));

    state->course_count = defaults;
    state->next_course_id = defaults + 1;
    for (int i = 0; i < defaults; i++)
    {
        state->courses[i].id = i + 1;
        copy_text(state->courses[i].name, sizeof(state->courses[i].name), names[i]);
        state->courses[i].capacity = 10 * (i + 1);
        state->courses[i].enrolled = 0;
        state->courses[i].credits = i + 1;
        copy_text(state->courses[i].schedule, sizeof(state->courses[i].schedule),
                  schedules[i]);
    }
    for (int i = defaults; i < MAX_COURSES; i++)
        memset(&state->courses[i], 0, sizeof(state->courses[i]));
}

int service_find_student_by_id(const AppState* state, const char* student_id)
{
    if (!student_id || !student_id[0])
        return -1;

    for (int i = 0; i < state->student_count; i++)
    {
        if (strcmp(state->students[i].student_id, student_id) == 0)
            return i;
    }
    return -1;
}

int service_get_course_index(const AppState* state, int course_id)
{
    if (course_id <= 0)
        return -1;

    for (int i = 0; i < state->course_count; i++)
    {
        if (state->courses[i].id == course_id)
            return i;
    }
    return -1;
}

int service_get_score(const Student* student, int course_id)
{
    int slot = find_score_slot(student, course_id);

    if (slot < 0)
        return -1;
    return student->scores[slot].score;
}

void service_recalc_student_metrics(AppState* state, int student_index)
{
    Student* student;
    double total_points = 0.0;
    int graded_credits = 0;

    if (student_index < 0 || student_index >= state->student_count)
        return;

    student = &state->students[student_index];
    student->total_credits = 0;
    student->fail_count = 0;
    student->gpa = 0.0;
    student->gpa_warning = 0;

    for (int i = 0; i < student->num_courses; i++)
    {
        int course_index = service_get_course_index(state, student->courses[i]);
        int score;

        if (course_index < 0)
            continue;

        student->total_credits += state->courses[course_index].credits;
        score = service_get_score(student, student->courses[i]);
        if (score < 0)
            continue;

        if (score < PASS_SCORE)
            student->fail_count++;

        total_points += score_to_points(score) * state->courses[course_index].credits;
        graded_credits += state->courses[course_index].credits;
    }

    if (graded_credits > 0)
    {
        student->gpa = total_points / graded_credits;
        student->gpa_warning = (student->gpa < 2.0) ? 1 : 0;
    }
}

void service_sync_state(AppState* state)
{
    int max_id = 0;

    for (int i = 0; i < state->course_count; i++)
    {
        if (state->courses[i].id > max_id)
            max_id = state->courses[i].id;
        state->courses[i].enrolled = 0;
    }
    for (int i = state->course_count; i < MAX_COURSES; i++)
        memset(&state->courses[i], 0, sizeof(state->courses[i]));

    for (int i = 0; i < state->student_count; i++)
    {
        Student* student = &state->students[i];

        normalize_student_courses(state, student);
        service_recalc_student_metrics(state, i);
        for (int j = 0; j < student->num_courses; j++)
        {
            int course_index = service_get_course_index(state, student->courses[j]);
            if (course_index >= 0)
                state->courses[course_index].enrolled++;
        }
    }

    state->next_course_id = max_id + 1;
    if (state->next_course_id <= 0)
        state->next_course_id = 1;
}

int service_validate_schedule(const char* schedule)
{
    int day = -1;
    int start = -1;
    int end = -1;

    if (!schedule || !schedule[0])
        return 1;

    parse_schedule_day(schedule, &day);
    parse_schedule_slots(schedule, &start, &end);
    if (day < 0 || start <= 0 || end <= 0 || start > end)
        return 0;
    return 1;
}

int service_check_time_conflict(const AppState* state, int course_id_a,
                                int course_id_b)
{
    int index_a = service_get_course_index(state, course_id_a);
    int index_b = service_get_course_index(state, course_id_b);
    int day_a = -1;
    int day_b = -1;
    int start_a = -1;
    int end_a = -1;
    int start_b = -1;
    int end_b = -1;

    if (index_a < 0 || index_b < 0)
        return 0;
    if (!state->courses[index_a].schedule[0] || !state->courses[index_b].schedule[0])
        return 0;

    parse_schedule_day(state->courses[index_a].schedule, &day_a);
    parse_schedule_day(state->courses[index_b].schedule, &day_b);
    parse_schedule_slots(state->courses[index_a].schedule, &start_a, &end_a);
    parse_schedule_slots(state->courses[index_b].schedule, &start_b, &end_b);

    if (day_a < 0 || day_b < 0 || start_a < 0 || end_a < 0 || start_b < 0 ||
        end_b < 0 || day_a != day_b)
        return 0;
    return start_a <= end_b && start_b <= end_a;
}

void service_sort_students_by_credits(const AppState* state, Student* sorted_out)
{
    memcpy(sorted_out, state->students, state->student_count * sizeof(Student));
    for (int i = 0; i < state->student_count - 1; i++)
    {
        for (int j = 0; j < state->student_count - i - 1; j++)
        {
            if (sorted_out[j].total_credits > sorted_out[j + 1].total_credits ||
                (sorted_out[j].total_credits == sorted_out[j + 1].total_credits &&
                 strcmp(sorted_out[j].name, sorted_out[j + 1].name) > 0))
            {
                Student temp = sorted_out[j];
                sorted_out[j] = sorted_out[j + 1];
                sorted_out[j + 1] = temp;
            }
        }
    }
}

int service_add_course(AppState* state, const char* name, int capacity, int credits,
                       const char* schedule, char* err, size_t err_size)
{
    Course* course;

    if (!name || !name[0])
    {
        set_error(err, err_size, "课程名称不能为空");
        return 0;
    }
    if (capacity <= 0 || credits <= 0)
    {
        set_error(err, err_size, "容量和学分必须为正数");
        return 0;
    }
    if (!service_validate_schedule(schedule))
    {
        set_error(err, err_size, "上课时间格式无效，示例: 周一 1-2节");
        return 0;
    }
    if (state->course_count >= MAX_COURSES)
    {
        set_error(err, err_size, "课程数量已达到上限");
        return 0;
    }

    course = &state->courses[state->course_count];
    memset(course, 0, sizeof(*course));
    course->id = state->next_course_id++;
    copy_text(course->name, sizeof(course->name), name);
    course->capacity = capacity;
    course->credits = credits;
    course->enrolled = 0;
    copy_text(course->schedule, sizeof(course->schedule), schedule);
    state->course_count++;
    service_sync_state(state);
    return 1;
}

int service_delete_course(AppState* state, int course_id, char* err,
                          size_t err_size)
{
    int course_index = service_get_course_index(state, course_id);

    if (course_index < 0)
    {
        set_error(err, err_size, "课程不存在");
        return 0;
    }

    for (int i = 0; i < state->student_count; i++)
    {
        Student* student = &state->students[i];
        for (int j = 0; j < student->num_courses; j++)
        {
            if (student->courses[j] != course_id)
                continue;
            for (int k = j; k < student->num_courses - 1; k++)
                student->courses[k] = student->courses[k + 1];
            student->courses[student->num_courses - 1] = -1;
            student->num_courses--;
            break;
        }
        clear_score_for_course(student, course_id);
    }

    for (int i = course_index; i < state->course_count - 1; i++)
        state->courses[i] = state->courses[i + 1];
    memset(&state->courses[state->course_count - 1], 0,
           sizeof(state->courses[state->course_count - 1]));
    state->course_count--;
    service_sync_state(state);
    return 1;
}

int service_rename_course(AppState* state, int course_id, const char* name, char* err,
                          size_t err_size)
{
    int course_index = service_get_course_index(state, course_id);

    if (course_index < 0)
    {
        set_error(err, err_size, "课程不存在");
        return 0;
    }
    if (!name || !name[0])
    {
        set_error(err, err_size, "课程名称不能为空");
        return 0;
    }

    copy_text(state->courses[course_index].name,
              sizeof(state->courses[course_index].name), name);
    service_sync_state(state);
    return 1;
}

int service_add_student(AppState* state, const char* name, const char* student_id,
                        const char* gender, char* err, size_t err_size)
{
    Student* student;

    if (!name || !name[0] || !student_id || !student_id[0])
    {
        set_error(err, err_size, "姓名和学号不能为空");
        return 0;
    }
    if (state->student_count >= MAX_STUDENTS)
    {
        set_error(err, err_size, "学生数量已达到上限");
        return 0;
    }
    if (service_find_student_by_id(state, student_id) >= 0)
    {
        set_error(err, err_size, "学号已存在");
        return 0;
    }

    student = &state->students[state->student_count];
    memset(student, 0, sizeof(*student));
    for (int i = 0; i < MAX_SELECTS; i++)
        student->courses[i] = -1;
    for (int i = 0; i < MAX_COURSES; i++)
    {
        student->scores[i].course_id = -1;
        student->scores[i].score = -1;
    }
    copy_text(student->name, sizeof(student->name), name);
    copy_text(student->student_id, sizeof(student->student_id), student_id);
    copy_text(student->gender, sizeof(student->gender), gender ? gender : "");
    state->student_count++;
    service_sync_state(state);
    return 1;
}

int service_update_student(AppState* state, int student_index, const char* name,
                           const char* student_id, const char* gender, char* err,
                           size_t err_size)
{
    int duplicate_index;
    Student* student;

    if (student_index < 0 || student_index >= state->student_count)
    {
        set_error(err, err_size, "学生不存在");
        return 0;
    }
    if (!name || !name[0] || !student_id || !student_id[0])
    {
        set_error(err, err_size, "姓名和学号不能为空");
        return 0;
    }

    duplicate_index = service_find_student_by_id(state, student_id);
    if (duplicate_index >= 0 && duplicate_index != student_index)
    {
        set_error(err, err_size, "学号已存在");
        return 0;
    }

    student = &state->students[student_index];
    copy_text(student->name, sizeof(student->name), name);
    copy_text(student->student_id, sizeof(student->student_id), student_id);
    copy_text(student->gender, sizeof(student->gender), gender ? gender : "");
    service_sync_state(state);
    return 1;
}

int service_delete_student(AppState* state, int student_index, char* err,
                           size_t err_size)
{
    if (student_index < 0 || student_index >= state->student_count)
    {
        set_error(err, err_size, "学生不存在");
        return 0;
    }

    for (int i = student_index; i < state->student_count - 1; i++)
        state->students[i] = state->students[i + 1];
    memset(&state->students[state->student_count - 1], 0,
           sizeof(state->students[state->student_count - 1]));
    for (int i = 0; i < MAX_SELECTS; i++)
        state->students[state->student_count - 1].courses[i] = -1;
    for (int i = 0; i < MAX_COURSES; i++)
    {
        state->students[state->student_count - 1].scores[i].course_id = -1;
        state->students[state->student_count - 1].scores[i].score = -1;
    }
    state->student_count--;
    service_sync_state(state);
    return 1;
}

int service_select_course(AppState* state, int student_index, int course_id, char* err,
                          size_t err_size)
{
    Student* student;
    int course_index;

    if (student_index < 0 || student_index >= state->student_count)
    {
        set_error(err, err_size, "请选择学生");
        return 0;
    }

    student = &state->students[student_index];
    course_index = service_get_course_index(state, course_id);
    if (course_index < 0)
    {
        set_error(err, err_size, "课程不存在");
        return 0;
    }
    if (student->num_courses >= MAX_SELECTS)
    {
        set_error(err, err_size, "已达到5门选课上限");
        return 0;
    }
    if (state->courses[course_index].enrolled >= state->courses[course_index].capacity)
    {
        set_error(err, err_size, "课程容量已满");
        return 0;
    }
    for (int i = 0; i < student->num_courses; i++)
    {
        if (student->courses[i] == course_id)
        {
            set_error(err, err_size, "该课程已选择");
            return 0;
        }
        if (service_check_time_conflict(state, student->courses[i], course_id))
        {
            set_error(err, err_size, "课程时间冲突");
            return 0;
        }
    }

    student->courses[student->num_courses++] = course_id;
    ensure_score_slot(student, course_id);
    service_sync_state(state);
    return 1;
}

int service_drop_course(AppState* state, int student_index, int course_id, char* err,
                        size_t err_size)
{
    Student* student;

    if (student_index < 0 || student_index >= state->student_count)
    {
        set_error(err, err_size, "请选择学生");
        return 0;
    }

    student = &state->students[student_index];
    if (student->num_courses <= 1)
    {
        set_error(err, err_size, "至少保留1门课程");
        return 0;
    }

    for (int i = 0; i < student->num_courses; i++)
    {
        if (student->courses[i] != course_id)
            continue;
        for (int j = i; j < student->num_courses - 1; j++)
            student->courses[j] = student->courses[j + 1];
        student->courses[student->num_courses - 1] = -1;
        student->num_courses--;
        clear_score_for_course(student, course_id);
        service_sync_state(state);
        return 1;
    }

    set_error(err, err_size, "该学生未选择这门课");
    return 0;
}

int service_finalize_selection(AppState* state, int student_index, char* err,
                               size_t err_size)
{
    if (student_index < 0 || student_index >= state->student_count)
    {
        set_error(err, err_size, "请选择学生");
        return 0;
    }
    if (state->students[student_index].num_courses < 3)
    {
        set_error(err, err_size, "至少选择3门课程");
        return 0;
    }

    service_sync_state(state);
    return 1;
}

int service_update_score(AppState* state, int student_index, int course_id, int score,
                         char* err, size_t err_size)
{
    int course_index;
    Student* student;
    int selected = 0;

    if (student_index < 0 || student_index >= state->student_count)
    {
        set_error(err, err_size, "请选择学生");
        return 0;
    }
    if (score < 0 || score > 100)
    {
        set_error(err, err_size, "成绩必须在0到100之间");
        return 0;
    }

    student = &state->students[student_index];
    course_index = service_get_course_index(state, course_id);
    if (course_index < 0)
    {
        set_error(err, err_size, "课程不存在");
        return 0;
    }

    for (int i = 0; i < student->num_courses; i++)
    {
        if (student->courses[i] == course_id)
        {
            selected = 1;
            break;
        }
    }
    if (!selected)
    {
        set_error(err, err_size, "该学生未选择这门课程");
        return 0;
    }

    course_index = ensure_score_slot(student, course_id);
    if (course_index < 0)
    {
        set_error(err, err_size, "成绩存储空间不足");
        return 0;
    }
    student->scores[course_index].score = score;
    service_sync_state(state);
    return 1;
}
