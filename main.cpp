#include "main.h"
#include "watek_glowny.h"
#include "watek_komunikacyjny.h"

int rank, size, lamport=0, lamportREQQUEUE=0;
int ackQueueCounter=0;
int groupSize, resNum; 

pthread_t threadKom;

void finalizuj() {
    pthread_mutex_destroy( &stateMut);
    /* Czekamy, aż wątek potomny się zakończy */
    println("czekam na wątek \"komunikacyjny\"\n" );
    pthread_join(threadKom,NULL);
    MPI_Type_free(&MPI_PAKIET_T);
    MPI_Finalize();
}

void check_thread_support(int provided) {
    debug("THREAD SUPPORT: chcemy %d. Co otrzymamy?", provided);
    switch (provided) {
        case MPI_THREAD_SINGLE: 
            debug("Brak wsparcia dla wątków, kończę");
            /* Nie ma co, trzeba wychodzić */
            fprintf(stderr, "Brak wystarczającego wsparcia dla wątków - wychodzę!");
            MPI_Finalize();
            exit(-1);
            break;
        case MPI_THREAD_FUNNELED: 
            debug("tylko te wątki, ktore wykonaly mpi_init_thread mogą wykonać wołania do biblioteki mpi");
	        break;
        case MPI_THREAD_SERIALIZED: 
            /* Potrzebne zamki wokół wywołań biblioteki MPI */
            debug("tylko jeden watek naraz może wykonać wołania do biblioteki MPI");
	        break;
        case MPI_THREAD_MULTIPLE: debug("Pełne wsparcie dla wątków"); /* tego chcemy. Wszystkie inne powodują problemy */
	        break;
        default: debug("Nikt nic nie wie\n");
    }
}


int main(int argc, char **argv) {
    if (argc >= 3) {
        resNum = atoi(argv[1]);
        groupSize = atoi(argv[2]);
    } else {
        printf("Nie podano argumentów!\n");
        return 0;
    }

    MPI_Status status;
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    // check_thread_support(provided);

    srand(rank);
    inicjuj_typ_pakietu();
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    pthread_mutex_lock(&groupQueueMut);
    pthread_create( &threadKom, NULL, startKomWatek , 0);
    mainLoop();
    finalizuj();
    return 0;
}

