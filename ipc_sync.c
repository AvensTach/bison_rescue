#include "ipc_sync.h"

volatile sig_atomic_t expeditions_ready_count = 0;

void handle_sigusr1(int sig) {
    expeditions_ready_count++;
}

void setup_signal_handlers(void) {
    struct sigaction sa;
    sa.sa_handler = handle_sigusr1;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction SIGUSR1 failed");
        exit(EXIT_FAILURE);
    }
}

int create_msg_queue(void) {
    int msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("msgget failed");
        exit(EXIT_FAILURE);
    }
    return msgid;
}

void remove_msg_queue(int msgid) {
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("msgctl IPC_RMID failed");
    }
}

#define MSG_TYPE_OBSERVATION 1

int send_observation(int msgid, Observation* data) {
    MsgBuf msg;
    msg.mtype = MSG_TYPE_OBSERVATION;
    memcpy(&msg.data, data, sizeof(Observation));

    return msgsnd(msgid, &msg, sizeof(Observation), 0);
}

int receive_observation(int msgid, Observation* data, int msgflg) {
    MsgBuf msg;
    int res = msgrcv(msgid, &msg, sizeof(Observation), MSG_TYPE_OBSERVATION, msgflg);
    if (res != -1) {
        memcpy(data, &msg.data, sizeof(Observation));
    }
    return res;
}

void create_pipes(int num_expeditions, int fd_array[][2]) {
    for (int i = 0; i < num_expeditions; i++) {
        if (pipe(fd_array[i]) == -1) {
            perror("pipe creation failed");
            exit(EXIT_FAILURE);
        }
    }
}

void send_territory_name(int write_fd, const char* territory) {
    if (write(write_fd, territory, strlen(territory) + 1) == -1) {
        perror("write to pipe failed");
    }
}

void receive_territory_name(int read_fd, char* buffer, size_t max_len) {
    if (read(read_fd, buffer, max_len) == -1) {
        perror("read from pipe failed");
    }
}