#include <stdio.h>

#include "app_paths.h"
#include "model.h"
#include "service.h"
#include "storage.h"
#include "ui.h"

int main(int argc, char* argv[])
{
    AppState state;
    char exe_dir[512];
    char data_dir[512];
    char courses_path[512];
    char students_path[512];
    char export_path[512];

    app_state_init(&state);
    app_path_dirname(argv[0], exe_dir, sizeof(exe_dir));
    if (!app_path_join(data_dir, sizeof(data_dir), exe_dir, "../data") ||
        !app_path_join(courses_path, sizeof(courses_path), data_dir, "courses.txt") ||
        !app_path_join(students_path, sizeof(students_path), data_dir,
                       "students.txt") ||
        !app_path_join(export_path, sizeof(export_path), exe_dir,
                       "students_export.csv"))
    {
        fprintf(stderr, "Failed to resolve runtime paths\n");
        return 1;
    }

    if (!storage_load_courses(&state, courses_path))
    {
        service_init_default_courses(&state);
        service_sync_state(&state);
        storage_save_courses(&state, courses_path);
    }
    if (!storage_load_students(&state, students_path))
        service_sync_state(&state);

    printf("Loaded %d students\n", state.student_count);
    create_ui(&state, argc, argv, courses_path, students_path, export_path);
    storage_save_courses(&state, courses_path);
    storage_save_students(&state, students_path);
    return 0;
}
