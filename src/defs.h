#ifndef DEFS_H
#define DEFS_H
#include <pthread.h>
void *screen_standby_loop(void *arg) ;
void *screen_suspend_loop(void *arg) ;
void *screen_off_loop(void *arg) ;
void *screensaver_loop(void *arg);
void *sleep_timeout_loop(void *arg);
void *update_xscreensaver_info_loop(void *arg);
void update_xscreensaver_info();
void main_cleanup(int signal);
void signal_handler(int signal);
void cafeine_handler(int signal);
int is_player_running();
extern pthread_mutex_t mutex;
#endif