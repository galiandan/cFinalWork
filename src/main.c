#include <stdio.h>
#include <unistd.h>

#include "app_paths.h"
#include "model.h"
#include "service.h"
#include "storage.h"
#include "ui.h"

static int file_exists(const char* path)
{
    return path && access(path, F_OK) == 0;
}

static int resolve_runtime_paths(const char* argv0, char* courses_path,
                                 size_t courses_size, char* students_path,
                                 size_t students_size, char* export_path,
                                 size_t export_size)
{
    char exe_dir[512];
    char candidate_data_dir[512];
    char cwd_data_dir[512];

    app_path_dirname(argv0, exe_dir, sizeof(exe_dir));
    if (!app_path_join(candidate_data_dir, sizeof(candidate_data_dir), exe_dir,
                       "../data") ||
        !app_path_join(courses_path, courses_size, candidate_data_dir,
                       "courses.txt") ||
        !app_path_join(students_path, students_size, candidate_data_dir,
                       "students.txt") ||
        !app_path_join(export_path, export_size, exe_dir, "students_export.csv"))
    {
        return 0;
    }

    if (file_exists(courses_path) && file_exists(students_path))
        return 1;

    if (!app_path_join(cwd_data_dir, sizeof(cwd_data_dir), ".", "data") ||
        !app_path_join(courses_path, courses_size, cwd_data_dir, "courses.txt") ||
        !app_path_join(students_path, students_size, cwd_data_dir, "students.txt"))
    {
        return 0;
    }
    return 1;
}

int main(int argc, char* argv[])
{
    AppState state;
    char courses_path[512];
    char students_path[512];
    char export_path[512];

    app_state_init(&state);
    if (!resolve_runtime_paths(argv[0], courses_path, sizeof(courses_path),
                               students_path, sizeof(students_path), export_path,
                               sizeof(export_path)))
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
