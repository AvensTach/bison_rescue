#include "common.h"
#include "file_writer.h"

volatile sig_atomic_t ready_count = 0;
volatile sig_atomic_t time_is_up = 0;

void handle_arrival(int sig) { ready_count++; }
void handle_alarm(int sig) { time_is_up = 1; }

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <number_of_expeditions> <observation_time_seconds>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int num_expeditions = atoi(argv[1]);
    int obs_time = atoi(argv[2]);

    if (num_expeditions <= 0 || num_expeditions > MAX_EXPEDITIONS || obs_time <= 0) {
        fprintf(stderr, "Invalid arguments. Max expeditions is %d.\n", MAX_EXPEDITIONS);
        exit(EXIT_FAILURE);
    }

    pid_t children[MAX_EXPEDITIONS];
    int pipes_fd[MAX_EXPEDITIONS][2];

    int msg_qid = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msg_qid == -1) { perror("msgget failed"); exit(EXIT_FAILURE); }

    signal(SIGUSR1, handle_arrival);
    signal(SIGALRM, handle_alarm);

    printf("Ministry: Starting %d expeditions for %d seconds.\n", num_expeditions, obs_time);

    for (int i = 0; i < num_expeditions; i++) {
        if (pipe(pipes_fd[i]) == -1) { perror("pipe failed"); exit(EXIT_FAILURE); }

        pid_t pid = fork();
        if (pid < 0) { perror("fork failed"); exit(EXIT_FAILURE); }
        else if (pid == 0) {

            close(pipes_fd[i][0]);

            srand(time(NULL) ^ getpid());

            kill(getppid(), SIGUSR1);

            char territory[50];
            ssize_t bytes_read = read(pipes_fd[i][0], territory, sizeof(territory) - 1);
            if (bytes_read > 0) {
                territory[bytes_read] = '\0';
            } else {
                strcpy(territory, "Unknown Territory");
            }
            close(pipes_fd[i][0]);

            printf("Expedition [PID %d]: Arrived at %s and starting observations!\n", getpid(), territory);
            char observer_name[50];
            snprintf(observer_name, sizeof(observer_name), "Researcher_%d", getpid());

            while (1) {
                sleep(2 + rand() % 4);

                MsgBuf msg;
                msg.mtype = 1;

                strncpy(msg.data.observing_personname, observer_name, sizeof(msg.data.observing_personname));
                strncpy(msg.data.territory_name, territory, sizeof(msg.data.territory_name));
                msg.data.bison_number = 1 + rand() % 15;
                msg.data.bison_age = (BisonAge)(rand() % 3);
                msg.data.observation_time = time(NULL);

                save_observation_to_file("observations.txt", &msg.data);

                if (msgsnd(msg_qid, &msg, sizeof(Observation), IPC_NOWAIT) == -1) {
                    perror("Expedition: msgsnd failed");
                } else {
                    printf("Expedition [PID %d]: Sent report (%d bisons at %s)\n", 
                           getpid(), msg.data.bison_number, msg.data.territory_name);
                }
            }

            exit(EXIT_SUCCESS);
        } else {
            children[i] = pid;
            close(pipes_fd[i][0]);
        }
    }

    while (ready_count < num_expeditions) { pause(); }
    printf("Ministry: All expeditions have arrived.\n");

    const char* territories[] = {"Carpathians", "Polissya", "Volyn", "Podillia", "Sivershchyna"};
    for (int i = 0; i < num_expeditions; i++) {
        const char* t_name = territories[i % 5];
        write(pipes_fd[i][1], t_name, strlen(t_name) + 1);
        close(pipes_fd[i][1]);
    }

    alarm(obs_time);
    MsgBuf msg;
    srand(time(NULL));

    while (!time_is_up) {
        if (msgrcv(msg_qid, &msg, sizeof(Observation), 0, IPC_NOWAIT) != -1) {
            printf("Ministry: Received observation from %s (Bisons: %d)\n", msg.data.territory_name, msg.data.bison_number);
        }
        if (rand() % 100 < 5) {
            int victim = rand() % num_expeditions;
            if (children[victim] > 0) {
                printf("Ministry: Recalling tired expedition PID %d\n", children[victim]);
                kill(children[victim], SIGTERM);
                children[victim] = -1;
            }
        }
        usleep(500000);
    }

    printf("Ministry: Time is up. Terminating remaining expeditions.\n");
    for (int i = 0; i < num_expeditions; i++) {
        if (children[i] > 0) kill(children[i], SIGTERM);
    }
    for (int i = 0; i < num_expeditions; i++) wait(NULL);

    msgctl(msg_qid, IPC_RMID, NULL);
    printf("Ministry: Project execution finished.\n");
    return 0;
}