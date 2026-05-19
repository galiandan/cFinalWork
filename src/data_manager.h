#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include "models.h"

void init_courses();
int load_courses(const char* filename);
void save_courses(const char* filename);
int load_students(const char* filename);
void save_students(const char* filename);
int find_student_by_id(const char* id);
int get_course_index(int cid);
void increase_enrolled(int cid);
void decrease_enrolled(int cid);
void compute_fail_count(int idx);
void sort_students(Student* sorted, int count);

extern Course courses[MAX_COURSES];
extern Student students[MAX_STUDENTS];
extern int student_count;

#endif /* DATA_MANAGER_H */
