#ifndef APP_PATHS_H
#define APP_PATHS_H

#include <stddef.h>

int app_path_join(char* out, size_t out_size, const char* base_dir,
                  const char* filename);
void app_path_dirname(const char* path, char* out, size_t out_size);
const char* app_path_basename(const char* path);

#endif
