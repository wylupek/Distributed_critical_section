#include "main.h"
#include "state_thread.h"
#include "com_thread.h"

int rank, size, lamport=0, lamportREQQUEUE=0;
int ackQueueCounter=0;
int ackResCounter=0;

pthread_t comThread;

void finalize() {
    pthread_mutex_destroy( &stateMut);
    println("czekam na wątek \"komunikacyjny\"\n" );
    pthread_join(comThread,NULL);
    MPI_Type_free(&MPI_PAKIET_T);
    MPI_Finalize();
}


int main(int argc, char **argv) {
    MPI_Status status;
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    srand(rank);
    init_packet();
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);


    pthread_mutex_lock(&waitingForQueueMut);
    pthread_mutex_lock(&waitingForGroupMut);
    pthread_mutex_lock(&waitingForResMut);
    pthread_mutex_lock(&waitingForStartMut);
    pthread_mutex_lock(&waitingForRelease);

    pthread_create(&comThread, NULL, startComThread , 0);
    mainLoop();
    finalize();
    return 0;
}

