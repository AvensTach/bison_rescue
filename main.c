#include "common.h"
#include "file_writer.h"
#include "ipc_sync.h"

volatile sig_atomic_t time_is_up = 0;
/* New counter for the second synchronization phase (ready to work) */
volatile sig_atomic_t expeditions_started_count = 0;

void handle_alarm(int sig) { time_is_up = 1; }
/* New signal handler for SIGUSR2 */
void handle_start(int sig) { expeditions_started_count++; }

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

    int msg_qid = create_msg_queue();

    /* Setup handlers */
    setup_signal_handlers(); // Sets up SIGUSR1

    /* Setup handler for SIGUSR2 (Task requirement 'b' completion) */
    struct sigaction sa;
    sa.sa_handler = handle_start;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGRTMIN + 1, &sa, NULL) == -1) {
        perror("sigaction SIGRTMIN+1 failed");
        exit(EXIT_FAILURE);
    }

    signal(SIGALRM, handle_alarm);

    create_pipes(num_expeditions, pipes_fd);

    printf("Ministry: Starting %d expeditions for %d seconds.\n", num_expeditions, obs_time);

    for (int i = 0; i < num_expeditions; i++) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork failed"); exit(EXIT_FAILURE); }
        else if (pid == 0) {
            /* ---------------- CHILD PROCESS ---------------- */
            close(pipes_fd[i][1]);
            srand(time(NULL) ^ getpid());

            /* Task 'a': Signal arrival */
            kill(getppid(), SIGRTMIN);

            /* Task 'b': Receive territory */
            char territory[50];
            receive_territory_name(pipes_fd[i][0], territory, sizeof(territory));
            close(pipes_fd[i][0]);

            /* Task 'b': Print identification text and signal ready to work */
            printf("Expedition [PID %d]: Arrived at %s and ready to work!\n", getpid(), territory);
            kill(getppid(), SIGRTMIN + 1);

            char observer_name[50];
            snprintf(observer_name, sizeof(observer_name), "Researcher_%d", getpid());

            /* Task 'c': Observation loop */
            while (1) {
                sleep(2 + rand() % 4);

                Observation obs;
                strncpy(obs.observing_personname, observer_name, sizeof(obs.observing_personname));
                strncpy(obs.territory_name, territory, sizeof(obs.territory_name));
                obs.bison_number = 1 + rand() % 15;
                obs.bison_age = (BisonAge)(rand() % 3);
                obs.observation_time = time(NULL);

                if (send_observation(msg_qid, &obs) == -1) {
                    perror("Expedition: msgsnd failed");
                } else {
                    printf("Expedition [PID %d]: Sent report (%d bisons at %s)\n",
                           getpid(), obs.bison_number, obs.territory_name);
                }
            }
            exit(EXIT_SUCCESS);
        } else {
            /* ---------------- PARENT PROCESS ---------------- */
            children[i] = pid;
            close(pipes_fd[i][0]);
        }
    }

    /* Wait for children to arrive */
    while (expeditions_ready_count < num_expeditions) { pause(); }
    printf("Ministry: All expeditions have arrived.\n");

    const char *territories[] = {"Carpathians", "Polissya", "Volyn", "Podillia", "Sivershchyna"};
    for (int i = 0; i < num_expeditions; i++) {
        const char *t_name = territories[i % 5];
        send_territory_name(pipes_fd[i][1], t_name);
        close(pipes_fd[i][1]);
    }

    /* Wait for children to acknowledge territories */
    while (expeditions_started_count < num_expeditions) { pause(); }
    printf("Ministry: All expeditions confirmed their territories and started working.\n");

    alarm(obs_time);
    srand(time(NULL));

    /* Main parent loop */
    while (!time_is_up) {
        Observation obs;

        while (receive_observation(msg_qid, &obs, IPC_NOWAIT) != -1) {
            printf("Ministry: Received observation from %s (Bisons: %d)\n",
                   obs.territory_name, obs.bison_number);

            save_observation_to_file("observations.txt", &obs);
        }

        /* Task 'd': Randomly recall tired expeditions */
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

    remove_msg_queue(msg_qid);
    printf("Ministry: Project execution finished.\n");
    return 0;
}