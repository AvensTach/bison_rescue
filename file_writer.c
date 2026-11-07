#include "file_writer.h"

int save_observation_to_file(const char *filename, const Observation *obs) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("open file failed");
        return -1;
    }

    char buffer[512];
    const char *age_str[] = {"Young", "Adult", "Old"};

    struct tm *tm_info = localtime(&obs->observation_time);
    char time_str[26];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

    int len = snprintf(buffer, sizeof(buffer),
                       "[%s] Explorer: %s | Territory: %s | Count: %d | Age: %s\n",
                       time_str, obs->observing_personname, obs->territory_name,
                       obs->bison_number, age_str[obs->bison_age]);

    if (write(fd, buffer, len) == -1) {
        perror("write to file failed");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}