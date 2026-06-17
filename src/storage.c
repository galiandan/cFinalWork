#include "storage.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "service.h"

static int parse_int_strict(const char* text, int* value)
{
    char* end = NULL;
    long parsed;

    if (!text || !text[0])
        return 0;

    errno = 0;
    parsed = strtol(text, &end, 10);
    if (errno != 0 || !end)
        return 0;
    while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')
        end++;
    if (*end != '\0')
        return 0;
    if (parsed < INT_MIN || parsed > INT_MAX)
        return 0;
    *value = (int)parsed;
    return 1;
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

static int split_fields(char* line, char* fields[], int max_fields)
{
    int count = 0;
    char* cursor = line;

    while (cursor && count < max_fields)
    {
        char* separator = strchr(cursor, '|');
        fields[count++] = cursor;
        if (separator)
        {
            *separator = '\0';
            cursor = separator + 1;
        }
        else
        {
            cursor = NULL;
        }
    }

    for (int i = 0; i < count; i++)
    {
        size_t len = strcspn(fields[i], "\r\n");
        fields[i][len] = '\0';
    }
    return count;
}

int storage_load_courses(AppState* state, const char* path)
{
    FILE* file = fopen(path, "r");
    char line[512];
    int expected = 0;

    if (!file)
        return 0;
    if (!fgets(line, sizeof(line), file) || !parse_int_strict(line, &expected) ||
        expected <= 0 || expected > MAX_COURSES)
    {
        fclose(file);
        return 0;
    }

    state->course_count = 0;
    state->next_course_id = 1;

    for (int i = 0; i < expected; i++)
    {
        char* fields[6];
        int field_count;
        int id = 0;
        int capacity = 0;
        int enrolled = 0;
        int credits = 0;
        Course* course;

        if (!fgets(line, sizeof(line), file))
            break;

        field_count = split_fields(line, fields, 6);
        if (field_count < 5 || !parse_int_strict(fields[0], &id) || id <= 0 ||
            !parse_int_strict(fields[2], &capacity) || capacity < 0 ||
            !parse_int_strict(fields[3], &enrolled) || enrolled < 0 ||
            !parse_int_strict(fields[4], &credits) || credits <= 0)
        {
            continue;
        }

        course = &state->courses[state->course_count++];
        memset(course, 0, sizeof(*course));
        course->id = id;
        copy_text(course->name, sizeof(course->name), fields[1]);
        course->capacity = capacity;
        course->enrolled = enrolled;
        course->credits = credits;
        if (field_count >= 6)
            copy_text(course->schedule, sizeof(course->schedule), fields[5]);
        if (id >= state->next_course_id)
            state->next_course_id = id + 1;
    }

    fclose(file);
    service_sync_state(state);
    return state->course_count > 0;
}

int storage_save_courses(const AppState* state, const char* path)
{
    FILE* file = fopen(path, "w");

    if (!file)
        return 0;

    fprintf(file, "%d\n", state->course_count);
    for (int i = 0; i < state->course_count; i++)
    {
        const Course* course = &state->courses[i];
        fprintf(file, "%d|%s|%d|%d|%d|%s\n", course->id, course->name,
                course->capacity, course->enrolled, course->credits, course->schedule);
    }

    fclose(file);
    return 1;
}

int storage_load_students(AppState* state, const char* path)
{
    FILE* file = fopen(path, "r");
    char line[2048];
    int expected = 0;

    if (!file)
        return 0;
    if (!fgets(line, sizeof(line), file) || !parse_int_strict(line, &expected) ||
        expected < 0 || expected > MAX_STUDENTS)
    {
        fclose(file);
        return 0;
    }

    state->student_count = 0;
    for (int i = 0; i < expected; i++)
    {
        char* fields[4 + MAX_SELECTS + MAX_COURSES + 2];
        int field_count;
        int derived_start;
        int score_count;
        Student student;

        if (!fgets(line, sizeof(line), file))
            break;

        memset(&student, 0, sizeof(student));
        for (int slot = 0; slot < MAX_SELECTS; slot++)
            student.courses[slot] = -1;
        for (int slot = 0; slot < MAX_COURSES; slot++)
        {
            student.scores[slot].course_id = -1;
            student.scores[slot].score = -1;
        }

        field_count = split_fields(line, fields,
                                   (int)(sizeof(fields) / sizeof(fields[0])));
        if (field_count < 4 + MAX_SELECTS)
            continue;

        copy_text(student.name, sizeof(student.name), fields[0]);
        copy_text(student.student_id, sizeof(student.student_id), fields[1]);
        copy_text(student.gender, sizeof(student.gender), fields[2]);

        for (int slot = 0; slot < MAX_SELECTS; slot++)
        {
            int course_id = -1;
            if (parse_int_strict(fields[4 + slot], &course_id) && course_id > 0)
                student.courses[slot] = course_id;
        }

        derived_start = field_count >= (4 + MAX_SELECTS + 2) ? field_count - 2
                                                              : field_count;
        score_count = derived_start - (4 + MAX_SELECTS);
        if (score_count < 0)
            score_count = 0;
        if (score_count > MAX_COURSES)
            score_count = MAX_COURSES;

        for (int slot = 0; slot < score_count; slot++)
        {
            int score = -1;
            if (parse_int_strict(fields[4 + MAX_SELECTS + slot], &score) && score >= 0 &&
                score <= 100)
            {
                if (slot < MAX_SELECTS && student.courses[slot] > 0)
                {
                    student.scores[slot].course_id = student.courses[slot];
                    student.scores[slot].score = score;
                }
            }
        }

        state->students[state->student_count++] = student;
    }

    fclose(file);
    service_sync_state(state);
    return 1;
}

int storage_save_students(const AppState* state, const char* path)
{
    FILE* file = fopen(path, "w");

    if (!file)
        return 0;

    fprintf(file, "%d\n", state->student_count);
    for (int i = 0; i < state->student_count; i++)
    {
        const Student* student = &state->students[i];

        fprintf(file, "%s|%s|%s|%d", student->name, student->student_id,
                student->gender, student->num_courses);
        for (int slot = 0; slot < MAX_SELECTS; slot++)
            fprintf(file, "|%d", student->courses[slot]);
        for (int slot = 0; slot < student->num_courses; slot++)
            fprintf(file, "|%d", service_get_score(student, student->courses[slot]));
        fprintf(file, "|%d|%d\n", student->total_credits, student->fail_count);
    }

    fclose(file);
    return 1;
}

int storage_export_students_csv(const AppState* state, const char* path)
{
    FILE* file = fopen(path, "w");

    if (!file)
        return 0;

    fprintf(file, "姓名,学号,性别,");
    for (int i = 0; i < state->course_count; i++)
        fprintf(file, "%s,", state->courses[i].name);
    fprintf(file, "GPA,学业预警\n");

    for (int i = 0; i < state->student_count; i++)
    {
        const Student* student = &state->students[i];

        fprintf(file, "%s,%s,%s,", student->name, student->student_id,
                student->gender);
        for (int j = 0; j < state->course_count; j++)
            fprintf(file, "%d,",
                    service_get_score(student, state->courses[j].id));
        fprintf(file, "%.2f,%s\n", student->gpa, student->gpa_warning ? "是" : "否");
    }

    fclose(file);
    return 1;
}
