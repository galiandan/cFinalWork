#include "model.h"

#include <string.h>

static void init_student(Student* student)
{
    memset(student, 0, sizeof(*student));
    student->gender[0] = '\0';
    student->num_courses = 0;
    student->total_credits = 0;
    student->fail_count = 0;
    student->gpa = 0.0;
    student->gpa_warning = 0;
    for (int i = 0; i < MAX_SELECTS; i++)
        student->courses[i] = -1;
    for (int i = 0; i < MAX_COURSES; i++)
    {
        student->scores[i].course_id = -1;
        student->scores[i].score = -1;
    }
}

void app_state_init(AppState* state)
{
    memset(state, 0, sizeof(*state));
    state->course_count = 0;
    state->student_count = 0;
    state->next_course_id = 1;
    for (int i = 0; i < MAX_STUDENTS; i++)
        init_student(&state->students[i]);
}
