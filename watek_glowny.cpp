#include "main.h"
#include "watek_glowny.h"

void mainLoop() {
    srandom(rank);
    int tag;
	packet_t* pkt = nullptr;

    while (true) {
	switch (state) {
		case Test:
			if (rank != 1) return;
			packet_t tmp_packet;
			groupQueue = {{1, 2}, {2, 2}, {3, 2}, {4, 2}, {5, 2}};
			tmp_packet.inGroup[0] = 2;
			tmp_packet.inGroup[1] = 3;
			for (int i = 0; i < GROUPSIZE; i++) {
				// auto it = std::find_if(groupQueue.begin(), groupQueue.end(), [i](const idLamportPair& s) { return s.id == tmp_packet.inGroup[i]; });
				auto it = std::find_if(groupQueue.begin(), groupQueue.end(), [i, &tmp_packet](const idLamportPair& s) { 
            		return s.id == tmp_packet.inGroup[i]; 
        		});
				if (it != groupQueue.end()) {
					groupQueue.erase(it);
				}
			}

			for (const auto& element : groupQueue) {
				std::cout << element.id << " ";
			}
			std::cout << groupMembers[2];

			changeState(InFinish);
			break;

	    case WantGroup:
			// 1.1
			println("Szukam grupy")
			pkt = new packet_t;

			pthread_mutex_lock(&lamportMut);
			lamport++;
			pkt->ts = lamport;
			lamportREQQUEUE = lamport;
			pthread_mutex_unlock(&lamportMut);

			sendPacketToAllNoInc(pkt, REQQUEUE);
			changeState( WaitingForQueue );
			free(pkt);
			println("Wysłałem wszystkie prośby o dołączenie do kolejki");
			break;

	    case WaitingForQueue:
			println("Czekam na wejście do kolejki")
			pthread_mutex_lock(&waitingForQueueMut);
			changeState(WaitingForGroup);
			break;

		case WaitingForGroup:
			println("Czekam w kolejce na uformowanie grupy");
			pthread_mutex_lock(&waitingForGroupMut);
			// 1.5
			if (leader == 1) {
				// 1.5.1
				pthread_mutex_lock(&groupQueueMut);
				for (int i = 0; i < GROUPSIZE; i++) {
					groupMembers[i] = groupQueue[i].id;
				}
				pthread_mutex_unlock(&groupQueueMut);

				print("* Jestem liderem grupy, skład grupy to:");
				for (int i = 0; i < GROUPSIZE; i++) {
					printNoTag(" %d", groupMembers[i]);
				} printNoTag("\n");

				for (int i = 0; i < GROUPSIZE; i++) {
					pkt->inGroup[i] = groupMembers[i];
				}

				pthread_mutex_lock(&lamportMut);
				lamport++;
				pkt->ts = lamport;
				pthread_mutex_unlock(&lamportMut);

				// 1.5.2
				sendPacketToAllNoInc(pkt, GROUPFORMED);

				// 1.5.3
				pthread_mutex_lock(&groupQueueMut);
				for (int i = 0; i < GROUPSIZE; i++) {
					auto it = std::find_if(groupQueue.begin(), groupQueue.end(), [i](const idLamportPair& s) { return s.id == groupMembers[i]; });
					if (it != groupQueue.end()) {
						groupQueue.erase(it);
					}
				}
				pthread_mutex_unlock(&groupQueueMut);

				// 1.5.4
				leaders.push_back(rank);
			}

			// 1.6
			else if (leader == 2) {
				print("* Jestem w grupie skład grupy to: ")
				for (int i = 0; i < GROUPSIZE; i++) {
					printNoTag(" %d", groupMembers[i]);
				} printNoTag("\n");

				// 1.6.4
				pthread_mutex_lock(&groupQueueMut);
				for (int i = 0; i < GROUPSIZE; i++) {
					auto it = std::find_if(groupQueue.begin(), groupQueue.end(), [i](const idLamportPair& s) { return s.id == groupMembers[i]; });
					if (it != groupQueue.end()) {
						groupQueue.erase(it);
					}
				}
				pthread_mutex_unlock(&groupQueueMut);
			}

			sleep(3);
			leader = 3;
			changeState(WantGroup);
			break;

		case InFinish:
			println("Wychodzę z sekcji");
			sendPacket(0, rank, END);
			return;
		default:
			return;
	}
    sleep(SEC_IN_STATE);
    }
}
