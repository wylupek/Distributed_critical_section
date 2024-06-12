#include "main.h"
#include "watek_glowny.h"

void mainLoop() {
    srandom(rank);
    int tag;
	packet_t* pkt = nullptr;

    while (true) {
	switch (state) {
		case Test:
			break;

	    case WantGroup:
			// 1.1 Proces wysyła do wszystich REQQUEUE i przechodzi do stanu WaitingForQueue
			println("Szukam grupy - wysyłam REQQUEUE")
			pkt = new packet_t;

			pthread_mutex_lock(&lamportMut);
			lamport++;
			pkt->ts = lamport;
			lamportREQQUEUE = lamport;
			pthread_mutex_unlock(&lamportMut);
			sendPacketToAllNoInc(pkt, REQQUEUE);

			free(pkt);
			println("Wysłałem wszystkie REQQUEUE");

			pthread_mutex_lock(&waitingForQueueMut);
			changeState(WaitingForGroup);
			break;

	    case WaitingForGroup:
			println("Czekam na uformowanie grupy")
			
			// 1.4 i 1.7 Proces sprawdza czy nie jest liderem
			println("Sprawdzam czy jestem liderem")
			if (groupQueue.size() >= GROUPSIZE && groupQueue[0].id == rank) {
				// 1.5 Proces jest liderem
				// 1.5.1 Wypełnia inGroup
				pthread_mutex_lock(&groupQueueMut);
				for (int i = 0; i < GROUPSIZE; i++) {
					groupMembers[i] = groupQueue[i].id;
				}
				pthread_mutex_unlock(&groupQueueMut);

				print("* Jestem liderem grupy, skład grupy to:");
				for (int i = 0; i < GROUPSIZE; i++) {
					printNoTag(" %d", groupMembers[i]);
				} printNoTag("\n");

				// 1.5.2 Proces wysyła do wszystkich GROUPFORMED z listą groupMembers
				for (int i = 0; i < GROUPSIZE; i++) {
					pkt->inGroup[i] = groupMembers[i];
				}
				pthread_mutex_lock(&lamportMut);
				lamport++;
				pkt->ts = lamport;
				pthread_mutex_unlock(&lamportMut);
				sendPacketToAllNoInc(pkt, GROUPFORMED);

				// 1.5.3 Proces usuwa z groupQueueMut procesy z list groupMembers
				pthread_mutex_lock(&groupQueueMut);
				for (int i = 0; i < GROUPSIZE; i++) {
					auto it = std::find_if(groupQueue.begin(), groupQueue.end(), [i](const idLamportPair& s) { return s.id == groupMembers[i]; });
					if (it != groupQueue.end()) {
						groupQueue.erase(it);
					}
				}
				pthread_mutex_unlock(&groupQueueMut);

				// 1.5.4 Proces dodaje się do listy leaders
				leaders.push_back(rank);
				changeState(Leader);
				break;
			}
			pthread_mutex_lock(&waitingForGroupMut);
			break;

		case Leader:
			println("Zostałem liderem");
			return;

		case Member:
			print("* Jestem członkiem grupy, skład grupy to:");
			for (int i = 0; i < GROUPSIZE; i++) {
				printNoTag(" %d", groupMembers[i]);
			} printNoTag("\n");
			return;

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


			// if (rank != 1) return;
			// packet_t tmp_packet;
			// groupQueue = {{1, 2}, {2, 2}, {3, 2}, {4, 2}, {5, 2}};
			// tmp_packet.inGroup[0] = 2;
			// tmp_packet.inGroup[1] = 3;
			// for (int i = 0; i < GROUPSIZE; i++) {
			// 	// auto it = std::find_if(groupQueue.begin(), groupQueue.end(), [i](const idLamportPair& s) { return s.id == tmp_packet.inGroup[i]; });
			// 	auto it = std::find_if(groupQueue.begin(), groupQueue.end(), [i, &tmp_packet](const idLamportPair& s) { 
            // 		return s.id == tmp_packet.inGroup[i]; 
        	// 	});
			// 	if (it != groupQueue.end()) {
			// 		groupQueue.erase(it);
			// 	}
			// }

			// for (const auto& element : groupQueue) {
			// 	std::cout << element.id << " ";
			// }
			// std::cout << groupMembers[2];

			// changeState(InFinish);