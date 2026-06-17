#ifndef SERVICE_H
#define SERVICE_H

#include <stddef.h>

#include "model.h"

void service_init_default_courses(AppState* state);
void service_sync_state(AppState* state);
void service_sort_students_by_credits(const AppState* state, Student* sorted_out);

int service_find_student_by_id(const AppState* state, const char* student_id);
int service_get_course_index(const AppState* state, int course_id);
int service_get_score(const Student* student, int course_id);
int service_check_time_conflict(const AppState* state, int course_id_a,
                                int course_id_b);
int service_validate_schedule(const char* schedule);
void service_recalc_student_metrics(AppState* state, int student_index);

int service_add_course(AppState* state, const char* name, int capacity, int credits,
                       const char* schedule, char* err, size_t err_size);
int service_delete_course(AppState* state, int course_id, char* err,
                          size_t err_size);
int service_rename_course(AppState* state, int course_id, const char* name, char* err,
                          size_t err_size);
int service_add_student(AppState* state, const char* name, const char* student_id,
                        const char* gender, char* err, size_t err_size);
int service_update_student(AppState* state, int student_index, const char* name,
                           const char* student_id, const char* gender, char* err,
                           size_t err_size);
int service_delete_student(AppState* state, int student_index, char* err,
                           size_t err_size);
int service_select_course(AppState* state, int student_index, int course_id, char* err,
                          size_t err_size);
int service_drop_course(AppState* state, int student_index, int course_id, char* err,
                        size_t err_size);
int service_finalize_selection(AppState* state, int student_index, char* err,
                               size_t err_size);
int service_update_score(AppState* state, int student_index, int course_id, int score,
                         char* err, size_t err_size);

#endif
