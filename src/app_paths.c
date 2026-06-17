#include "app_paths.h"

#include <stdio.h>
#include <string.h>

int app_path_join(char* out, size_t out_size, const char* base_dir,
                  const char* filename)
{
    if (!out || out_size == 0 || !base_dir || !filename)
        return 0;
    if (snprintf(out, out_size, "%s/%s", base_dir, filename) >= (int)out_size)
        return 0;
    return 1;
}

void app_path_dirname(const char* path, char* out, size_t out_size)
{
    const char* slash;
    size_t len;

    if (!out || out_size == 0)
        return;
    out[0] = '\0';
    if (!path || !path[0])
        return;

    slash = strrchr(path, '/');
    if (!slash)
    {
        snprintf(out, out_size, ".");
        return;
    }

    len = (size_t)(slash - path);
    if (len == 0)
        len = 1;
    if (len >= out_size)
        len = out_size - 1;
    memcpy(out, path, len);
    out[len] = '\0';
}

const char* app_path_basename(const char* path)
{
    const char* slash;

    if (!path)
        return "";
    slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}
