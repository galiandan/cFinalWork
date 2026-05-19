#ifndef MODELS_H
#define MODELS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME_LEN 30
#define MAX_ID_LEN 20
#define MAX_CNAME_LEN 50
#define MAX_COURSES 7
#define MAX_STUDENTS 200
#define MAX_SELECTS 5
#define PASS_SCORE 60

typedef struct {
    int id;
    char name[MAX_CNAME_LEN];
    int capacity;
    int enrolled;
    int credits;
} Course;

typedef struct {
    char name[MAX_NAME_LEN];
    char student_id[MAX_ID_LEN];
    char gender[6];
    int courses[MAX_SELECTS];
    int num_courses;
    int scores[MAX_COURSES];
    int total_credits;
    int fail_count;
} Student;

#endif /* MODELS_H */
