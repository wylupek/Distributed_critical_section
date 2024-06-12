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
    int groupQueueSize;
    packet_t packet;
    std::vector<int> vec(GROUPSIZE);

    while (state != InFinish) {
	    // debugln("Czekam na komunikat");
        MPI_Recv(&packet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        
        pthread_mutex_lock(&lamportMut);
        lamport = std::max(lamport, packet.ts) + 1;
        pthread_mutex_unlock(&lamportMut);
        
        switch (status.MPI_TAG) {
        case REQQUEUE:
            // 1.3
            // debugln("Otrzymano REQQUEUE od [%d %d]", packet.src, packet.ts);

            pthread_mutex_lock(&groupQueueMut);
            groupQueue.push_back({packet.src, packet.ts});
            std::sort(groupQueue.begin(), groupQueue.end(), compare);
            firstId = groupQueue[0].id;
            groupQueueSize = groupQueue.size();

            debug("GroupQueue wyglada tak: { ")
            for (const auto& element : groupQueue) {
                debugNoTag("[%d %d] ", element.id, element.lamport);
            } debugNoTag("}\n");
            pthread_mutex_unlock(&groupQueueMut);

            // 1.4
            if (state == WaitingForGroup) {
                if (groupQueueSize >= GROUPSIZE) {
                    if (firstId == rank) {
                        leader = 1;
                        pthread_mutex_unlock(&waitingForGroupMut);
                    }
                }
            }

            sendPacket(0, status.MPI_SOURCE, ACKQUEUE);
            break;

        case ACKQUEUE:
            // 1.2
            // debugln("Otrzymano ACKQUEUE od [%d %d]. Mam już %d", packet.src, packet.ts, ackQueueCounter + 1);
            ackQueueCounter++;
            if (ackQueueCounter == size - 1) {
                ackQueueCounter = 0;
                pthread_mutex_lock(&groupQueueMut);
                groupQueue.push_back({rank, lamportREQQUEUE});
                std::sort(groupQueue.begin(), groupQueue.end(), compare);
                firstId = groupQueue[0].id;
                pthread_mutex_unlock(&groupQueueMut);

                debug("Otrzymałem wszystkie ACK, GroupQueue wyglada tak: { ")
                for (const auto& element : groupQueue) {
                    debugNoTag("[%d %d] ", element.id, element.lamport);
                } debugNoTag("}\n");

                pthread_mutex_unlock(&waitingForQueueMut);

                // 1.4
                if (groupQueue.size() >= GROUPSIZE) {
                    if (firstId == rank) {
                        leader = 1;
                        pthread_mutex_unlock(&waitingForGroupMut);
                    }
                }
            }
            break;
        
        case GROUPFORMED:
            debug("Otrzymano GROUPFORMED od [%d %d]. Utworzona grupa to:", packet.src, packet.ts);
            for (int i = 0; i < GROUPSIZE; i++) {
                debugNoTag(" %d", packet.inGroup[i]);
            } debugNoTag("\n");

            // 1.6.1
            for (int i = 0; i < GROUPSIZE; i++) {
                if (packet.inGroup[i] == rank) {
                    debugln("Jestem w grupie z liderem [%d %d]", packet.src, packet.ts)
                    // 1.6.2
                    for (int j = 0; j < GROUPSIZE; j++) {
                        groupMembers[j] = packet.inGroup[j];
                    }

                    // 1.6.3
                    leaders.push_back(packet.src);
                    
                    leader = 2;
                    pthread_mutex_unlock(&waitingForGroupMut);
                    break;
                }
            }
            if (leader == 2) break;

            
            // 1.7
            pthread_mutex_lock(&groupQueueMut);
            for (int i = 0; i < GROUPSIZE; i++) {
                auto it = std::find_if(groupQueue.begin(), groupQueue.end(), [i, &packet](const idLamportPair& s) { 
            		return s.id == packet.inGroup[i]; 
        		});
                if (it != groupQueue.end()) {
                    groupQueue.erase(it);
                }
            }

            firstId = groupQueue[0].id;
            groupQueueSize = groupQueue.size();
            pthread_mutex_unlock(&groupQueueMut);

            if (state == WaitingForGroup && leader == 3) {
                debugln("Nie mam grupy, ale może teraz ja zostane liderem %d.", firstId);
                if (groupQueueSize >= GROUPSIZE) {
                    if (firstId == rank) {
                        leader = 1;
                        pthread_mutex_unlock(&waitingForGroupMut);
                    }
                }
            }

            break;

        default:
            break;
        }
    }
    return ptr;
}
