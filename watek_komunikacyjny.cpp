#include "main.h"
#include "watek_komunikacyjny.h"

bool compare(const idLamportPair& a, const idLamportPair& b) {
    if (a.lamport == b.lamport) {
        return a.id < b.id;
    }
    return a.lamport < b.lamport;
}

void addToResQueue(const packet_t &packet, int state_no) {
    pthread_mutex_lock(&resQueueMut);
    resQueue.push_back({packet.src, packet.ts});
    std::sort(resQueue.begin(), resQueue.end(), compare);

    debug("[R%d] REQRES [%d %d] - ResQueue wyglada tak: { ", state_no, packet.src, packet.ts);
    for (const auto& element : resQueue) {
        debugNoTag("[%d %d] ", element.id, element.lamport);
    } debugNoTag("}\n");
    pthread_mutex_unlock(&resQueueMut);
}

void addToGroupQueue(const packet_t &packet, int state_no) {
    pthread_mutex_lock(&groupQueueMut);
    groupQueue.push_back({packet.src, packet.ts});
    std::sort(groupQueue.begin(), groupQueue.end(), compare);
    pthread_mutex_unlock(&groupQueueMut);

    debug("[R%d] REQQUEUE [%d %d] - GroupQueue wyglada tak: { ", state_no, packet.src, packet.ts);
    for (const auto& element : groupQueue) {
        debugNoTag("[%d %d] ", element.id, element.lamport);
    } debugNoTag("}\n");
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
    pthread_mutex_unlock(&groupQueueMut);
    debug("[I%d] groupQueue wyglada tak: { ", state_no);
    for (const auto& element : groupQueue) {
        debugNoTag("[%d %d] ", element.id, element.lamport);
    } debugNoTag("}\n");
}

void delFromResQueue(const packet_t &packet, int state_no) {
    pthread_mutex_lock(&resQueueMut);

    auto it = std::find_if(resQueue.begin(), resQueue.end(), [&packet](const idLamportPair& s) { 
            return s.id == packet.src; 
    });
    if (it != resQueue.end()) {
        resQueue.erase(it);
    }
    
    pthread_mutex_unlock(&resQueueMut);
    debug("[I%d] resQueue wyglada tak: { ", state_no);
    for (const auto& element : resQueue) {
        debugNoTag("[%d %d] ", element.id, element.lamport);
    } debugNoTag("}\n");
}

void *startKomWatek(void *ptr) {
    MPI_Status status;
    packet_t packet;

    while (true) {
    debugln("Czekam na komunikat");
    MPI_Recv(&packet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    
    pthread_mutex_lock(&lamportMut);
    lamport = std::max(lamport, packet.ts) + 1;
    pthread_mutex_unlock(&lamportMut);
    
    switch (state) {
    case WantGroup:
        switch (status.MPI_TAG) {
        // Zapisuje w groupQueue
        case REQQUEUE:
            // 1.3 Proces zapisuje otrzymane REQQUEUE oraz odsyła ACKQUEUE niezależnie od stanu
            addToGroupQueue(packet, 1);
            sendPacket(0, status.MPI_SOURCE, ACKQUEUE);
            break;

        // Zliczam ACKQUEUE
        case ACKQUEUE:
            // 1.2 Proces zlicza ACKQUEUE i jeżeli zliczy size - 1 to przechodzi do stanu WaitingForQueue
            debugln("[R1] ACKQUEUE [%d %d]", packet.src, packet.ts);
            ackQueueCounter++;
            if (ackQueueCounter == size - 1) {
                ackQueueCounter = 0;
                pthread_mutex_lock(&groupQueueMut);
                groupQueue.push_back({rank, lamportREQQUEUE});
                std::sort(groupQueue.begin(), groupQueue.end(), compare);
                pthread_mutex_unlock(&groupQueueMut);

                debug("[I1] Otrzymałem wszystkie ACKQUEUE, GroupQueue wyglada tak: { ");
                for (const auto& element : groupQueue) {
                    debugNoTag("[%d %d] ", element.id, element.lamport);
                } debugNoTag("}\n");
                
                // Odblokowanie przejście do stanu WaitingForGroup + sprawdzenie czy nie jest liderem
                pthread_mutex_unlock(&waitingForQueueMut);
            }
            break;

        // Usuwam procesy z groupQueue EDGE CASE jeżeli nie dostałem wszystckich ack a ktos wysle mi zapro
        // To idę i ustawiam ackCoutner = 0 i nie zliczam.
        case GROUPFORMED:
            debug("[R1] GROUPFORMED [%d %d] - Utworzona grupa to:", packet.src, packet.ts);
            for (int i = 0; i < GROUPSIZE; i++) {
                debugNoTag(" %d", packet.inGroup[i]);
            } debugNoTag("\n");

            // 1.6.2 Jeżeli proces znajduje się w przesłanej liscie, to znaczy ze znalazł grupę i wypełnia groupMembers.
            for (int i = 0; i < GROUPSIZE; i++) {
                if (packet.inGroup[i] == rank) {
                    for (int j = 0; j < GROUPSIZE; j++) {
                        groupMembers[j] = packet.inGroup[j];
                    }
                    ackQueueCounter = 0;
                    changeState(Member);
                }
            }

            // 1.6.4 Proces usuwa członków grupy z groupQueue
            delFromGroupQueue(packet, 1);
            break;

        // Zapisuje proces do resQueue
        case REQRES:
            addToResQueue(packet, 1);
            sendPacket(0, status.MPI_SOURCE, ACKRES);
            break;

        // Nie otrzymam, bo jeszcze nie wysłałem, albo jeżeli to kolejna iteracja, 
        // to żeby się tu znaleźć musiałem otrzymać wszystkie ACKRES
        case ACKRES:
            break;

        case END:
            // Proces czekający na sekcje jeszcze raz sprawdza czy może do niej wejść
            debugln("[R1] END [%d, %d]", packet.src, packet.ts);
            delFromResQueue(packet, 1);
            break;

        default:
            break;
        }
        break;

    case WaitingForGroup:
        switch (status.MPI_TAG) {
        // Zapisuje do groupQueue i sprawdzam czy nie jestem liderem
        case REQQUEUE:
            // 1.3 Proces zapisuje otrzymane REQQUEUE oraz odsyła ACKQUEUE niezależnie od stanu
            addToGroupQueue(packet, 2);
            sendPacket(0, status.MPI_SOURCE, ACKQUEUE);

            // Po każdym otrzymanym REQQUEUE proces moze zostać liderem
            pthread_mutex_unlock(&waitingForGroupMut);
            break;

        // Nie dostane ACKQUEUE, bo jestem w tym stanie gdy dostane już wszystkie
        case ACKQUEUE:
            break;

        // Sprawdzam czy nie znajduję się w grupie
        case GROUPFORMED:   // Można zrobic ze lider wysyla do siebie groupFormed i tutaj sa usuwane wszystkie rzeczy. W sumie to moze byc nawet mądrzejsze xd
            debug("[R2] GROUPFORMED [%d %d] - Utworzona grupa to:", packet.src, packet.ts);
            for (int i = 0; i < GROUPSIZE; i++) {
                debugNoTag(" %d", packet.inGroup[i]);
            } debugNoTag("\n");

            // 1.6.2 Jeżeli proces znajduje się w przesłanej liscie, to znaczy ze znalazł grupę i wypełnia groupMembers.
            for (int i = 0; i < GROUPSIZE; i++) {
                if (packet.inGroup[i] == rank) {
                    for (int j = 0; j < GROUPSIZE; j++) {
                        groupMembers[j] = packet.inGroup[j];
                    }
                    changeState(Member);
                }
            }
            
            // 1.6.4 Proces usuwa członków grupy z groupQueue
            delFromGroupQueue(packet, 2);

            // Odblokowanie przejście do stanu Member lub sprawdzenie czy proces nie zostanie nowym liderem
            pthread_mutex_unlock(&waitingForGroupMut);
            break;

        // Dodatnie do resQueue
        case REQRES:
            // 2.1.3 Dodaje do resQueue
            addToResQueue(packet, 2);
            sendPacket(0, status.MPI_SOURCE, ACKRES);
            break;

        // Nie otrzymam, bo jeszcze nie wysłałem, albo jeżeli to kolejna iteracja, 
        // to żeby się tu znaleźć musiałem otrzymać wszystkie ACKRES
        case ACKRES:
            break;

        case END:
            // Proces czekający na sekcje jeszcze raz sprawdza czy może do niej wejść
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
            // 1.3 Proces zapisuje otrzymane REQQUEUE oraz odsyła ACKQUEUE niezależnie od stanu
            addToGroupQueue(packet, 3);
            sendPacket(0, status.MPI_SOURCE, ACKQUEUE);
            break;

        // Nie dostane ACKQUEUE, bo jestem w tym stanie gdy dostane już wszystkie
        case ACKQUEUE:
            break;

        case GROUPFORMED:
            debug("[R3] GROUPFORMED [%d %d] - Utworzona grupa to:", packet.src, packet.ts);
            for (int i = 0; i < GROUPSIZE; i++) {
                debugNoTag(" %d", packet.inGroup[i]);
            } debugNoTag("\n");
            
            // 1.6.4 Proces usuwa członków grupy z groupQueue
            delFromGroupQueue(packet, 3);

            break;

        case REQRES:
            // 2.1.3
            addToResQueue(packet, 3);
            sendPacket(0, status.MPI_SOURCE, ACKRES);
            break;

        case ACKRES:
            // 2.1.2
            debugln("[R3] ACKQRES [%d %d]", packet.src, packet.ts);
            ackResCounter++;
            if (ackResCounter == size - 1) {
                ackResCounter = 0;
                pthread_mutex_lock(&resQueueMut);
                resQueue.push_back({rank, lamportREQQUEUE});
                std::sort(resQueue.begin(), resQueue.end(), compare);
                pthread_mutex_unlock(&resQueueMut);

                debug("[I3] Otrzymałem wystarczającą ilość ACKRES, ResQueue wyglada tak: { ");
                for (const auto& element : resQueue) {
                    debugNoTag("[%d %d] ", element.id, element.lamport);
                } debugNoTag("}\n");
                
                pthread_mutex_unlock(&waitingForResMut);
            }
            break;

        case END:
            // Proces czekający na sekcje jeszcze raz sprawdza czy może do niej wejść
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
            // 1.3 Proces zapisuje otrzymane REQQUEUE oraz odsyła ACKQUEUE
            addToGroupQueue(packet, 4);
            sendPacket(0, status.MPI_SOURCE, ACKQUEUE);
            break;

        // Nie dostane ACKQUEUE, bo jestem w tym stanie gdy dostane już wszystkie
        case ACKQUEUE:
            break;

        case GROUPFORMED:   // Można zrobic ze lider wysyla do siebie groupFormed i tutaj sa usuwane wszystkie rzeczy. W sumie to moze byc nawet mądrzejsze xd
            debug("[R4] GROUPFORMED [%d %d] - Utworzona grupa to:", packet.src, packet.ts);
            for (int i = 0; i < GROUPSIZE; i++) {
                debugNoTag(" %d", packet.inGroup[i]);
            } debugNoTag("\n");

            // 1.6.4 Proces usuwa członków grupy z groupQueue
            delFromGroupQueue(packet, 4);

            break;

        case REQRES:
            // 2.1.3
            addToResQueue(packet, 4);
            sendPacket(0, status.MPI_SOURCE, ACKRES);
            break;

        // Nie dostane ACKRES, bo nie wysyłałem ACKREQ
        case ACKRES:
            break;

        // Po START wchodze do sekcji
        case START:
            // 2.2
            println("[R4] START [%d %d]", packet.src, packet.ts);
            // Pozwala na zmiane stanu na InSection
            pthread_mutex_unlock(&waitingForStartMut);
            break;

        case END:
            // Proces czekający na sekcje jeszcze raz sprawdza czy może do niej wejść
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
            // 1.3 Proces zapisuje otrzymane REQQUEUE oraz odsyła ACKQUEUE niezależnie od stanu
            addToGroupQueue(packet, 5);
            sendPacket(0, status.MPI_SOURCE, ACKQUEUE);
            break;

        // Nie dostane ACKQUEUE, bo jestem w tym stanie gdy dostane już wszystkie
        case ACKQUEUE:
            break;

        case GROUPFORMED:   // Można zrobic ze lider wysyla do siebie groupFormed i tutaj sa usuwane wszystkie rzeczy. W sumie to moze byc nawet mądrzejsze xd
            debug("[R5] GROUPFORMED [%d %d] - Utworzona grupa to:", packet.src, packet.ts);
            for (int i = 0; i < GROUPSIZE; i++) {
                debugNoTag(" %d", packet.inGroup[i]);
            } debugNoTag("\n");

            // 1.6.4 Proces usuwa członków grupy z groupQueue
            delFromGroupQueue(packet, 5);
            break;

        case REQRES:
            addToResQueue(packet, 5);
            sendPacket(0, status.MPI_SOURCE, ACKRES);
            break;

        // Nie dostane ACKQUEUE, bo jestem w tym stanie gdy dostane już wszystkie
        case ACKRES:
            break;

        case END:
            debugln("[R5] END [%d, %d]", packet.src, packet.ts);
            delFromResQueue(packet, 5);
            // Proces czekający na sekcje jeszcze raz sprawdza czy może do niej wejść
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
            // 1.3 Proces zapisuje otrzymane REQQUEUE oraz odsyła ACKQUEUE niezależnie od stanu
            addToGroupQueue(packet, 5);
            sendPacket(0, status.MPI_SOURCE, ACKQUEUE);
            break;

        // Nie dostane ACKQUEUE, bo jestem w tym stanie gdy dostane już wszystkie
        case ACKQUEUE:
            break;

        case GROUPFORMED:   // Można zrobic ze lider wysyla do siebie groupFormed i tutaj sa usuwane wszystkie rzeczy. W sumie to moze byc nawet mądrzejsze xd
            debug("[R5] GROUPFORMED [%d %d] - Utworzona grupa to:", packet.src, packet.ts);
            for (int i = 0; i < GROUPSIZE; i++) {
                debugNoTag(" %d", packet.inGroup[i]);
            } debugNoTag("\n");
            
            // 1.6.4 Proces usuwa członków grupy z groupQueue
            delFromGroupQueue(packet, 5);
            break;

        case REQRES:
            addToResQueue(packet, 5);
            sendPacket(0, status.MPI_SOURCE, ACKRES);
            break;

        // Nie dostane ACKQUEUE, bo jestem w tym stanie gdy dostane już wszystkie
        case ACKRES:
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
