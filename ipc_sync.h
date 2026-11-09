#ifndef IPC_SYNC_H
#define IPC_SYNC_H

#include "common.h"

extern volatile sig_atomic_t expeditions_ready_count;

void setup_signal_handlers(void);
int create_msg_queue(void);
void remove_msg_queue(int msgid);
int send_observation(int msgid, Observation* data);
int receive_observation(int msgid, Observation* data, int msgflg);
void create_pipes(int num_expeditions, int fd_array[][2]);
void send_territory_name(int write_fd, const char* territory);
void receive_territory_name(int read_fd, char* buffer, size_t max_len);

#endif /* IPC_SYNC_H */