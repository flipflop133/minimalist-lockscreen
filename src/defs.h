#ifndef DEFS_H
#define DEFS_H
void *screen_timeout_loop(void *arg);
void *sleep_timeout_loop(void *arg);
long retrieve_idle_time();
void signal_handler(int signal);
int is_player_running();
#endif