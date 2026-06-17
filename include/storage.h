#ifndef STORAGE_H
#define STORAGE_H

#include "model.h"

int storage_load_courses(AppState* state, const char* path);
int storage_save_courses(const AppState* state, const char* path);
int storage_load_students(AppState* state, const char* path);
int storage_save_students(const AppState* state, const char* path);
int storage_export_students_csv(const AppState* state, const char* path);

#endif
