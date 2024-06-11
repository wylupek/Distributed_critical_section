#include "main.h"
#include "watek_komunikacyjny.h"

bool compare(const idLamportPair& a, const idLamportPair& b) {
    if (a.lamport == b.lamport) {
        return a.id < b.id;
    }
    return a.lamport < b.lamport;
}

/* wątek komunikacyjny; zajmuje się odbiorem i reakcją na komunikaty */
void *startKomWatek(void *ptr)
{
    MPI_Status status;
    int is_message = FALSE;
    packet_t pakiet;
    
    while (state != InFinish) {
	    debug("Czekam na komunikat");
        MPI_Recv(&pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        pthread_mutex_lock(&lamportMut);
        lamport = std::max(lamport, pakiet.ts) + 1;
        pthread_mutex_unlock(&lamportMut);

        switch ( status.MPI_TAG ) {
        case REQQUEUE: 
            debug("Otrzymano REQQUEUE od [%d %d]", pakiet.src, pakiet.ts);
            groupQueue.push_back({pakiet.src, pakiet.ts});
            sendPacket(0, status.MPI_SOURCE, ACKQUEUE);
            break;

        case ACKQUEUE: 
            debug("Otrzymano ACKQUEUE od [%d %d]. Mam już %d", pakiet.src, pakiet.ts, ackQueueCounter + 1);
            ackQueueCounter++;
            if (ackQueueCounter == size - 1) {
                ackQueueCounter = 0;
                groupQueue.push_back({rank, lamportREQQUEUE});
                pthread_mutex_unlock(&groupQueueMut);
            }
            break;
            
        default:
            break;
        }
    }
    return ptr;
}
        // std::string result;
        // result += "(";
        // for (const auto& element : groupQueue) {
        //      result += "[" + std::to_string(element.id) + " " + std::to_string(element.lamport) + "] ";
        // } result += ")";
        // std::cout << " [" << rank << " " << lamport << "]" << " Moja kolejka: " << result << "\n";