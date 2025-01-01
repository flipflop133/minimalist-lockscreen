#ifndef DEFS_H
#define DEFS_H
#include <pthread.h>
void *screensaver_loop(void *arg);
void *sleep_timeout_loop(void *arg);
void *update_xscreensaver_info_loop(void *arg);
void main_cleanup(int signal);
void lockscreen_handler(int signal);
int is_player_running();
extern pthread_mutex_t mutex;
#endif