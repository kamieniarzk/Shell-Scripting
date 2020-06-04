#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>

#define BARBERS_FEMALE 2
#define BARBERS_MALE 2
#define BARBERS_UNI 1
#define BARBERS_COUNT BARBERS_FEMALE + BARBERS_MALE + BARBERS_UNI
#define CHAIRS 4
#define CLIENTS 30

#define KEY_BARBER 0x0000
#define KEY_CLIENT 0x0001
#define KEY_ROOM 0x0002
#define KEY_MEM 0x0003

enum type
{
    FEMALE,
    MALE,
    UNIVERSAL
};

// functions
void clean_up();
void barber(int id, int type);
void client(int id, int type);
void room_down();
void room_up();
void barber_down(int type);
void barber_up(int type);
void client_down(int type);
void client_up(int type);

// global variables
int children_count;
pid_t children[BARBERS_COUNT];
struct sharedMem // shared variables structure - waiting room
{
    int waiting_females;
    int waiting_males;
    int chairs;
} * mem;

union semun {              // for passing to semctl
    int val;               /* Value for SETVAL/GETVAL */
    struct semid_ds *buf;  /* Buffer for IPC_STAT, IPC_SET */
    unsigned short *array; /* Array for GETALL, SETALL */
    struct seminfo *__buf; /* Buffer for IPC_INFO
                                        (Linux-specific) */
} semun;

int main()
{
    // allocating shared memory segment
    int memid = shmget(KEY_MEM, sizeof(struct sharedMem), 0660 | IPC_CREAT);
    if (memid < 0)
        perror("shmget error");

    mem = shmat(memid, NULL, 0);

    if (mem == (void *)-1)
        perror("shmat error");

    // initializing shared memory attributes to initial state (no clients in the waiting room)
    mem->chairs = CHAIRS;
    mem->waiting_females = 0;
    mem->waiting_males = 0;

    // creating semaphores
    int sem_room = semget(KEY_ROOM, 1, 0660 | IPC_CREAT);      // semaphore for waiting room
    int sem_barbers = semget(KEY_BARBER, 3, 0660 | IPC_CREAT); // set of semaphores, one for each type of barber
    int sem_clients = semget(KEY_CLIENT, 2, 0660 | IPC_CREAT); // set of semaphores, one for each type of client

    if (sem_clients < 0 || sem_barbers < 0 || sem_room < 0)
        perror("Error while creating the semaphores");

    // setting the semaphores to initial values

    // setting waiting room mutex to unlocked
    semun.val = 1;
    if (semctl(sem_room, 0, SETVAL, semun) < 0)
        perror("error while initializing sem_room");

    // setting barbers semaphores
    //FEMALE
    semun.val = 0;
    if (semctl(sem_barbers, FEMALE, SETVAL, semun) < 0)
        perror("error while initializing BARBERS_FEMALE semaphore");
    //MALE
    semun.val = 0;
    if (semctl(sem_barbers, MALE, SETVAL, semun) < 0)
        perror("error while initializing BARBERS_MALE semaphore");
    //UNIVERSAL
    semun.val = 0;
    if (semctl(sem_barbers, UNIVERSAL, SETVAL, semun) < 0)
        perror("error while initializing BARBERS_UNI semaphore");

    // setting clients semaphores
    //FEMALE
    semun.val = 0;
    if (semctl(sem_clients, FEMALE, SETVAL, semun) < 0)
        perror("error while initializing CLIENTS_FEMALE semaphore");
    //MALE
    semun.val = 0;
    if (semctl(sem_clients, MALE, SETVAL, semun) < 0)
        perror("error while initializing CLIENTS_MALE semaphore");

    // barbers loop
    pid_t tempID;
    for (int i = 0; i < BARBERS_COUNT; i++)
    {
        tempID = fork();
        // process fork fail
        if (tempID < 0)
        {
            clean_up();
            exit(1);
        }
        // inside parent
        if (tempID > 0)
        {
            children[children_count] = tempID;
            children_count++;
        }
        // inside child
        if (tempID == 0)
        {
            if (i < BARBERS_FEMALE)
                barber(i, FEMALE);
            else if (i >= BARBERS_FEMALE && i < BARBERS_FEMALE + BARBERS_MALE)
                barber(i - BARBERS_FEMALE, MALE);
            else
                barber(i - BARBERS_FEMALE - BARBERS_MALE, UNIVERSAL);
            return 0;
        }
    }

    // clients loop
    for (int i = 0; i < CLIENTS; i++)
    {
        tempID = fork();
        // process fork fail
        if (tempID < 0)
        {
            clean_up();
            exit(1);
        }
        // inside parent
        if (tempID > 0)
        {
            children[children_count] = tempID;
            children_count++;
        }
        // inside child
        if (tempID == 0)
        {
            srand(time(NULL));
            while (1)
            {
                client(i, rand() % 2); // random gender
                sleep(rand() % 3);
            }
            return 0;
        }
    }
    sleep(90);
    clean_up();
}

void barber(int id, int type)
{

    if (type == 0)
        printf("FEMALE");
    else if (type == 1)
        printf("MALE");
    else
        printf("UNIVERSAL");
    printf(" barber[%d] created\n", id);
    int client_type = type;
    srand(time(NULL));
    if (type == UNIVERSAL)
    {
        room_down();
        if (mem->waiting_females == 0) // if there are men waiting, set client to be served to man
            client_type = 1;
        else if (mem->waiting_females == 0) // if there are women waiting, set client to be served to woman
            client_type = 0;
        else // if there are both male and female clients waiting, randomize choice
            client_type = rand() % 2;
        room_up();
    }

    while (1)
    {

        // decrements the semaphore for client of given type,
        // if semaphore is negative it means there are no clients
        // of this type in the waiting room and barber goes to sleep
        client_down(client_type);

        // barber awake, locking the waiting room semaphore
        room_down();

        // accessing shared variables - decrementing number of waiting
        // clients of given type
        mem->chairs++;
        if (client_type == 0)
            mem->waiting_females--;
        else
            mem->waiting_males--;

        printf("WAITING ROOM: %d FEMALES, %d MALES\n", mem->waiting_females, mem->waiting_males);

        // setting barber semaphore up
        barber_up(type);

        // unlocking the waiting room semaphore
        room_up();

        if (type == 0)
            printf("FEMALE");
        else if (type == 1)
            printf("MALE");
        else
            printf("UNIVERSAL");
        printf(" barber[%d] is cutting hair\n", id);
        srand(time(NULL));
        sleep(rand() % 3); // random haircut time between 0 and 2 seconds
    }
}

void client(int id, int type)
{
    int barber_type = type;
    int sem_barber = semget(KEY_BARBER, 3, 0660);
    type == 0 ? printf("FEMALE") : printf("MALE");
    printf(" client[%d] came into the barber saloon.\n", id);

    // locking the waiting room semaphore
    room_down();

    if (mem->chairs > 0) // if there are chairs available in the waiting room
    {
        // accessing shared variables - incrementing number of waiting
        // clients of given type
        mem->chairs--;
        if (type == 0)
            mem->waiting_females++;
        else
            mem->waiting_males++;
        if (semctl(sem_barber, type, GETVAL) < semctl(sem_barber, UNIVERSAL, GETVAL))
            barber_type = UNIVERSAL;
        printf("WAITING ROOM: %d FEMALES, %d MALES\n", mem->waiting_females, mem->waiting_males);

        // setting client semaphore up
        client_up(type);

        // unlocking the waiting room semaphore
        room_up();

        // setting barber semaphore down
        barber_down(barber_type);

        type == 0 ? printf("FEMALE") : printf("MALE");
        printf(" client[%d] is being served\n", id);
    }
    else // waiting room is full
    {
        // unlocking the waiting room semaphore
        room_up();
        printf("No free chairs in the waiting room\n");
        type == 0 ? printf("FEMALE") : printf("MALE");
        printf(" client[%d] leaving the barber saloon.\n", id);
    }
}

void room_down()
{
    struct sembuf arg = {0, -1, SEM_UNDO};
    int sem_room = semget(KEY_ROOM, 1, 0660);
    if (sem_room < 0)
        perror("error in semget in room_down()");
    if (semop(sem_room, &arg, 1) < 0)
        perror("error in semop in room_down()");
    return;
}

void room_up()
{
    struct sembuf arg = {0, 1, SEM_UNDO};
    int sem_room = semget(KEY_ROOM, 1, 0660);
    if (sem_room < 0)
        perror("error in semget in room_up()");
    if (semop(sem_room, &arg, 1) < 0)
        perror("error in semop in room_down()");
    return;
}

void barber_down(int type)
{

    struct sembuf arg = {type, -1, SEM_UNDO};
    int sem = semget(KEY_BARBER, 3, 0660);
    if (sem < 0)
        perror("error in semget in barber_down()");
    if (semop(sem, &arg, 1) < 0)
        perror("error in semop in barber_down()");

    return;
}

void barber_up(int type)
{
    struct sembuf arg = {type, 1, SEM_UNDO};
    int sem = semget(KEY_BARBER, 3, 0660);
    if (sem < 0)
        perror("error in semget in barber_up()");
    if (semop(sem, &arg, 1) < 0)
        perror("error in semop in barber_up()");

    return;
}

void client_down(int type)
{
    struct sembuf arg = {type, -1, SEM_UNDO};
    int sem = semget(KEY_CLIENT, 2, 0660);
    if (sem < 0)
        perror("error in semget in client_down()");
    if (semop(sem, &arg, 1) < 0)
        perror("error in semop in client_down()");
    return;
}

void client_up(int type)
{
    struct sembuf arg = {type, 1, SEM_UNDO};
    int sem = semget(KEY_CLIENT, 2, 0660);
    if (sem < 0)
        perror("error in semget in client_up()");
    if (semop(sem, &arg, 1) < 0)
        perror("error in semgop in client_up()");
    return;
}

void clean_up()
{
    pid_t temp;
    while (1) // wait for all the processes
    {
        temp = wait(NULL);
        if (temp == -1)
            break;
    }
    // terminating all the processes
    for (int i = 0; i < children_count; i++)
    {
        kill(children[i], SIGTERM);
    }
}
