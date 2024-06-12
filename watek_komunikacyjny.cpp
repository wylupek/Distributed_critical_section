#include "main.h"
#include "watek_komunikacyjny.h"

bool compare(const idLamportPair& a, const idLamportPair& b) {
    if (a.lamport == b.lamport) {
        return a.id < b.id;
    }
    return a.lamport < b.lamport;
}

void *startKomWatek(void *ptr) {
    MPI_Status status;
    packet_t packet;

    while (state != InFinish) {
	    // debugln("Czekam na komunikat");
        MPI_Recv(&packet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        
        pthread_mutex_lock(&lamportMut);
        lamport = std::max(lamport, packet.ts) + 1;
        pthread_mutex_unlock(&lamportMut);
        
        switch (state) {
        case WantGroup:
            switch (status.MPI_TAG) {
            case REQQUEUE:
                // 1.3 Proces zapisuje otrzymane REQQUEUE oraz odsyła ACKQUEUE
                pthread_mutex_lock(&groupQueueMut);
                groupQueue.push_back({packet.src, packet.ts});
                std::sort(groupQueue.begin(), groupQueue.end(), compare);
                pthread_mutex_unlock(&groupQueueMut);

                debug("[R wantGroup] REQQUEUE [%d %d] - GroupQueue wyglada tak: { ", packet.src, packet.ts)
                for (const auto& element : groupQueue) {
                    debugNoTag("[%d %d] ", element.id, element.lamport);
                } debugNoTag("}\n");

                sendPacket(0, status.MPI_SOURCE, ACKQUEUE);
                break;

            case ACKQUEUE:
                // 1.2 Proces zlicza ACKQUEUE i jeżeli zliczy size - 1 to przechodzi do stanu WaitingForQueue
                debugln("[R] ACKQUEUE [%d %d]", packet.src, packet.ts);
                ackQueueCounter++;
                if (ackQueueCounter == size - 1) {
                    ackQueueCounter = 0;
                    pthread_mutex_lock(&groupQueueMut);
                    groupQueue.push_back({rank, lamportREQQUEUE});
                    std::sort(groupQueue.begin(), groupQueue.end(), compare);
                    pthread_mutex_unlock(&groupQueueMut);

                    debug("[I] Otrzymałem wszystkie ACKQUEUE, GroupQueue wyglada tak: { ")
                    for (const auto& element : groupQueue) {
                        debugNoTag("[%d %d] ", element.id, element.lamport);
                    } debugNoTag("}\n");
                    
                    // Odblokowanie przejście do stanu WaitingForGroup
                    pthread_mutex_unlock(&waitingForQueueMut);
                }
                break;

            case GROUPFORMED:
                debug("[R] GROUPFORMED [%d %d] - Utworzona grupa to:", packet.src, packet.ts);
                for (int i = 0; i < GROUPSIZE; i++) {
                    debugNoTag(" %d", packet.inGroup[i]);
                } debugNoTag("\n");
                break;

            default:
                break;
            }
            break;

        case WaitingForGroup:
            switch (status.MPI_TAG) {
            case REQQUEUE:
                // 1.3 Proces zapisuje otrzymane REQQUEUE oraz odsyła ACKQUEUE
                pthread_mutex_lock(&groupQueueMut);
                groupQueue.push_back({packet.src, packet.ts});
                std::sort(groupQueue.begin(), groupQueue.end(), compare);
                pthread_mutex_unlock(&groupQueueMut);

                debug("[R] REQQUEUE [%d %d] - GroupQueue wyglada tak: { ", packet.src, packet.ts)
                for (const auto& element : groupQueue) {
                    debugNoTag("[%d %d] ", element.id, element.lamport);
                } debugNoTag("}\n");

                sendPacket(0, status.MPI_SOURCE, ACKQUEUE);
                pthread_mutex_unlock(&waitingForGroupMut);
                break;

            case ACKQUEUE:
                // Nie dostane ACKQUEUE, bo jestem w tym stanie gdy dostane już wszystkie
                break;

            case GROUPFORMED:   // Można zrobic ze lider wysyla do siebie groupFormed i tutaj sa usuwane wszystkie rzeczy. W sumie to moze byc nawet mądrzejsze xd
                debug("[R] GROUPFORMED [%d %d] - Utworzona grupa to:", packet.src, packet.ts);
                for (int i = 0; i < GROUPSIZE; i++) {
                    debugNoTag(" %d", packet.inGroup[i]);
                } debugNoTag("\n");

                for (int i = 0; i < GROUPSIZE; i++) {
                    if (packet.inGroup[i] == rank) {
                        // 1.6.2 Wypełnia inGroup
                        for (int j = 0; j < GROUPSIZE; j++) {
                            groupMembers[j] = packet.inGroup[j];
                        }
                        changeState(Member);
                    }
                }
                
                // 1.6.3 Proces dodaje nadawcę do listy leaders
                leaders.push_back(packet.src); // Tutaj moze mutex

                // 1.6.4 Proces usuwa członków grupy z groupQueue
                pthread_mutex_lock(&groupQueueMut);
                for (int i = 0; i < GROUPSIZE; i++) {
                    auto it = std::find_if(groupQueue.begin(), groupQueue.end(), [i, &packet](const idLamportPair& s) { 
                        return s.id == packet.inGroup[i]; 
                    });
                    if (it != groupQueue.end()) {
                        groupQueue.erase(it);
                    }
                }
                pthread_mutex_unlock(&groupQueueMut);

                // Odblokowanie przejście do stanu Member
                pthread_mutex_unlock(&waitingForGroupMut);
                break;

            default:
                break;
            }
            break;
        
        default:
            break;
        }
    }
    return ptr;
}
