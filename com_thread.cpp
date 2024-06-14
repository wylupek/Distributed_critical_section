#include "main.h"
#include "com_thread.h"

bool compare(const idLamportPair& a, const idLamportPair& b) {
    if (a.lamport == b.lamport) {
        return a.id < b.id;
    }
    return a.lamport < b.lamport;
}

void addToGroupQueue(const packet_t &packet, int state_no) {
    pthread_mutex_lock(&groupQueueMut);
    groupQueue.push_back({packet.src, packet.ts});
    std::sort(groupQueue.begin(), groupQueue.end(), compare);
    debug("[R%d] REQQUEUE [%d %d] - GroupQueue: { ", state_no, packet.src, packet.ts);
    for (const auto& element : groupQueue) {
        debugNoTag("[%d %d] ", element.id, element.lamport);
    } debugNoTag("}\n");
    pthread_mutex_unlock(&groupQueueMut);
}

void delFromGroupQueue(const packet_t &packet, int state_no) {
    pthread_mutex_lock(&groupQueueMut);
    for (int i = 0; i < GROUPSIZE; i++) {
        auto it = std::find_if(groupQueue.begin(), groupQueue.end(), [i, &packet](const idLamportPair& s) { 
            return s.id == packet.inGroup[i]; 
        });
        if (it != groupQueue.end()) {
            groupQueue.erase(it);
        }
    }
    debug("[I%d] GroupQueue: { ", state_no);
    for (const auto& element : groupQueue) {
        debugNoTag("[%d %d] ", element.id, element.lamport);
    } debugNoTag("}\n");
    pthread_mutex_unlock(&groupQueueMut);
}

void addToResQueue(const packet_t &packet, int state_no) {
    pthread_mutex_lock(&resQueueMut);
    resQueue.push_back({packet.src, packet.ts});
    std::sort(resQueue.begin(), resQueue.end(), compare);
    debug("[R%d] REQRES [%d %d] - ResQueue: { ", state_no, packet.src, packet.ts);
    for (const auto& element : resQueue) {
        debugNoTag("[%d %d] ", element.id, element.lamport);
    } debugNoTag("}\n");
    pthread_mutex_unlock(&resQueueMut);
}

void delFromResQueue(const packet_t &packet, int state_no) {
    pthread_mutex_lock(&resQueueMut);
    auto it = std::find_if(resQueue.begin(), resQueue.end(), [&packet](const idLamportPair& s) { 
            return s.id == packet.src; 
    });
    if (it != resQueue.end()) {
        resQueue.erase(it);
    }
    debug("[I%d] ResQueue: { ", state_no);
    for (const auto& element : resQueue) {
        debugNoTag("[%d %d] ", element.id, element.lamport);
    } debugNoTag("}\n");
    pthread_mutex_unlock(&resQueueMut);
}

void *startComThread(void *ptr) {
    MPI_Status status;
    packet_t packet;

    while (true) {
    debugln("Waiting for message");
    MPI_Recv(&packet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    
    pthread_mutex_lock(&lamportMut);
    lamport = std::max(lamport, packet.ts) + 1;
    pthread_mutex_unlock(&lamportMut);
    
    switch (state) {
    case WantGroup:
        switch (status.MPI_TAG) {
        case REQQUEUE:
            addToGroupQueue(packet, 1);
            sendPacket(0, status.MPI_SOURCE, ACKQUEUE);
            break;

        case ACKQUEUE:
            debugln("[R1] ACKQUEUE [%d %d]", packet.src, packet.ts);
            ackQueueCounter++;

            /* If I received all ACKQUEUE */
            if (ackQueueCounter == size - 1) {
                ackQueueCounter = 0;
                pthread_mutex_lock(&groupQueueMut);
                groupQueue.push_back({rank, lamportREQQUEUE});
                std::sort(groupQueue.begin(), groupQueue.end(), compare);
                pthread_mutex_unlock(&groupQueueMut);

                debug("[I1] Received all ACKQUEUE, GroupQueue: { ");
                for (const auto& element : groupQueue) {
                    debugNoTag("[%d %d] ", element.id, element.lamport);
                } debugNoTag("}\n");
                
                /* Unlocks WaitingForGroup state */
                pthread_mutex_unlock(&waitingForQueueMut);
            }
            break;

        case GROUPFORMED:
            debug("[R1] GROUPFORMED [%d %d] - members:", packet.src, packet.ts);
            for (int i = 0; i < GROUPSIZE; i++) {
                debugNoTag(" %d", packet.inGroup[i]);
            } debugNoTag("\n");

            /* If im in the message I found a group and can fill group members */
            for (int i = 0; i < GROUPSIZE; i++) {
                if (packet.inGroup[i] == rank) {
                    for (int j = 0; j < GROUPSIZE; j++) {
                        groupMembers[j] = packet.inGroup[j];
                    }
                    ackQueueCounter = 0;
                    changeState(Member);
                }
            }

            /* Remove group members from the queue of processes waiting for the group */
            delFromGroupQueue(packet, 1);
            break;

        case REQRES:
            addToResQueue(packet, 1);
            sendPacket(0, status.MPI_SOURCE, ACKRES);
            break;

        case ACKRES:
            break;

        case END:
            debugln("[R1] END [%d, %d]", packet.src, packet.ts);
            delFromResQueue(packet, 1);
            break;

        default:
            break;
        }
        break;

    case WaitingForGroup:
        switch (status.MPI_TAG) {
        case REQQUEUE:
            addToGroupQueue(packet, 2);
            sendPacket(0, status.MPI_SOURCE, ACKQUEUE);

            /* Unlocks checking if I become a new leader */
            pthread_mutex_unlock(&waitingForGroupMut);
            break;

        case GROUPFORMED:
            debug("[R2] GROUPFORMED [%d %d] - members:", packet.src, packet.ts);
            for (int i = 0; i < GROUPSIZE; i++) {
                debugNoTag(" %d", packet.inGroup[i]);
            } debugNoTag("\n");

            /* If im in the message I found a group and can fill group members */
            for (int i = 0; i < GROUPSIZE; i++) {
                if (packet.inGroup[i] == rank) {
                    for (int j = 0; j < GROUPSIZE; j++) {
                        groupMembers[j] = packet.inGroup[j];
                    }
                    changeState(Member);
                }
            }
            
            /* Remove group members from the queue of processes waiting for the group */
            delFromGroupQueue(packet, 2);

            /* Unlocks checking if I become a new leader */
            pthread_mutex_unlock(&waitingForGroupMut);
            break;

        case REQRES:
            addToResQueue(packet, 2);
            sendPacket(0, status.MPI_SOURCE, ACKRES);
            break;

        case END:
            debugln("[R2] END [%d, %d]", packet.src, packet.ts);
            delFromResQueue(packet, 2);
            break;

        default:
            break;
        }
        break;
    
    case Leader:
        switch (status.MPI_TAG) {
        case REQQUEUE:
            addToGroupQueue(packet, 3);
            sendPacket(0, status.MPI_SOURCE, ACKQUEUE);
            break;

        case GROUPFORMED:
            debug("[R3] GROUPFORMED [%d %d] - members:", packet.src, packet.ts);
            for (int i = 0; i < GROUPSIZE; i++) {
                debugNoTag(" %d", packet.inGroup[i]);
            } debugNoTag("\n");
            
            /* Remove group members from the queue of processes waiting for the group */
            delFromGroupQueue(packet, 3);
            break;

        case REQRES:
            addToResQueue(packet, 3);
            sendPacket(0, status.MPI_SOURCE, ACKRES);
            break;

        case ACKRES:
            debugln("[R3] ACKQRES [%d %d]", packet.src, packet.ts);
            ackResCounter++;

            /* If I received all ACKRES */
            if (ackResCounter == size - 1) {
                ackResCounter = 0;
                pthread_mutex_lock(&resQueueMut);
                resQueue.push_back({rank, lamportREQQUEUE});
                std::sort(resQueue.begin(), resQueue.end(), compare);
                pthread_mutex_unlock(&resQueueMut);

                debug("[I3] Received all ACKRES, GroupQueue: { ");
                for (const auto& element : resQueue) {
                    debugNoTag("[%d %d] ", element.id, element.lamport);
                } debugNoTag("}\n");
                
                /* Unlocks WaitingForRes state */
                pthread_mutex_unlock(&waitingForResMut);
            }
            break;

        case END:
            debugln("[R3] END [%d, %d]", packet.src, packet.ts);
            delFromResQueue(packet, 3);
            break;

        default:
            break;
        }
        break;

    case Member:
        switch (status.MPI_TAG) {
        case REQQUEUE:
            addToGroupQueue(packet, 4);
            sendPacket(0, status.MPI_SOURCE, ACKQUEUE);
            break;

        case GROUPFORMED:
            debug("[R4] GROUPFORMED [%d %d] - members:", packet.src, packet.ts);
            for (int i = 0; i < GROUPSIZE; i++) {
                debugNoTag(" %d", packet.inGroup[i]);
            } debugNoTag("\n");

            /* Remove group members from the queue of processes waiting for the group */
            delFromGroupQueue(packet, 4);
            break;

        case REQRES:
            addToResQueue(packet, 4);
            sendPacket(0, status.MPI_SOURCE, ACKRES);
            break;

        case START:
            debugln("[R4] START [%d %d]", packet.src, packet.ts);

            /* Unlocks InSection state*/
            pthread_mutex_unlock(&waitingForStartMut);
            break;

        case END:
            debugln("[R4] END [%d, %d]", packet.src, packet.ts);
            delFromResQueue(packet, 4);
            break;

        default:
            break;
        }
        break;

    case WaitingForRes:
        switch (status.MPI_TAG) {
        case REQQUEUE:
            addToGroupQueue(packet, 5);
            sendPacket(0, status.MPI_SOURCE, ACKQUEUE);
            break;

        case GROUPFORMED:
            debug("[R5] GROUPFORMED [%d %d] - members:", packet.src, packet.ts);
            for (int i = 0; i < GROUPSIZE; i++) {
                debugNoTag(" %d", packet.inGroup[i]);
            } debugNoTag("\n");

            /* If im in the message I found a group and can fill group members */
            delFromGroupQueue(packet, 5);
            break;

        case REQRES:
            addToResQueue(packet, 5);
            sendPacket(0, status.MPI_SOURCE, ACKRES);
            break;

        case END:
            debugln("[R5] END [%d, %d]", packet.src, packet.ts);
            delFromResQueue(packet, 5);
            
            /* Unlocks checking if I can enter critical section*/
            pthread_mutex_unlock(&waitingForRelease);
            break;

        default:
            break;
        }
        break;

    case Break:
    case InSection:
        switch (status.MPI_TAG) {
        case REQQUEUE:
            addToGroupQueue(packet, 5);
            sendPacket(0, status.MPI_SOURCE, ACKQUEUE);
            break;

        case GROUPFORMED:
            debug("[R5] GROUPFORMED [%d %d] - members:", packet.src, packet.ts);
            for (int i = 0; i < GROUPSIZE; i++) {
                debugNoTag(" %d", packet.inGroup[i]);
            } debugNoTag("\n");
            
            /* If im in the message I found a group and can fill group members */
            delFromGroupQueue(packet, 5);
            break;

        case REQRES:
            addToResQueue(packet, 5);
            sendPacket(0, status.MPI_SOURCE, ACKRES);
            break;

        case END:
            debugln("[R6] END [%d, %d]", packet.src, packet.ts);
            delFromResQueue(packet, 6);
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
