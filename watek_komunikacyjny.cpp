#include "main.h"
#include "watek_komunikacyjny.h"

bool compare(const idLamportPair& a, const idLamportPair& b) {
    if (a.lamport == b.lamport) {
        return a.id < b.id;
    }
    return a.lamport < b.lamport;
}

/* wątek komunikacyjny; zajmuje się odbiorem i reakcją na komunikaty */
void *startKomWatek(void *ptr) {
    MPI_Status status;
    int firstId;
    packet_t pakiet;
    std::vector<int> vec(GROUPSIZE);

    while (state != InFinish) {
	    debugln("Czekam na komunikat");
        MPI_Recv(&pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        
        pthread_mutex_lock(&lamportMut);
        lamport = std::max(lamport, pakiet.ts) + 1;
        pthread_mutex_unlock(&lamportMut);
        
        switch (status.MPI_TAG) {
        case REQQUEUE:
            // 1.3
            debugln("Otrzymano REQQUEUE od [%d %d]", pakiet.src, pakiet.ts);

            pthread_mutex_lock(&groupQueueMut);
            groupQueue.push_back({pakiet.src, pakiet.ts});
            std::sort(groupQueue.begin(), groupQueue.end(), compare);
            firstId = groupQueue[0].id;
            pthread_mutex_unlock(&groupQueueMut);

            // 1.4
            if (state == WaitingForGroup) {
                if (groupQueue.size() >= GROUPSIZE) {
                    if (firstId == rank) {
                        leader = true;
                        pthread_mutex_unlock(&waitingForGroupMut);
                    }
                }
            }

            sendPacket(0, status.MPI_SOURCE, ACKQUEUE);
            break;

        case ACKQUEUE:
            // 1.2
            debugln("Otrzymano ACKQUEUE od [%d %d]. Mam już %d", pakiet.src, pakiet.ts, ackQueueCounter + 1);
            ackQueueCounter++;
            if (ackQueueCounter == size - 1) {
                ackQueueCounter = 0;

                pthread_mutex_lock(&groupQueueMut);
                groupQueue.push_back({rank, lamportREQQUEUE});
                std::sort(groupQueue.begin(), groupQueue.end(), compare);
                firstId = groupQueue[0].id;
                pthread_mutex_unlock(&groupQueueMut);

                pthread_mutex_unlock(&waitingForQueueMut);

                // 1.4
                if (groupQueue.size() >= GROUPSIZE) {
                    if (firstId == rank) {
                        leader = true;
                        pthread_mutex_unlock(&waitingForGroupMut);
                    }
                }
            }
            break;
        
        case GROUPFORMED:
            debug("Otrzymano GROUPFORMED od [%d %d]. Utworzona grupa to:", pakiet.src, pakiet.ts);
            for (int i = 0; i < GROUPSIZE; i++) {
                debugNoTag(" %d", pakiet.inGroup[i]);
            } debugNoTag("\n");
            break;

        default:
            break;
        }
    }
    return ptr;
}
