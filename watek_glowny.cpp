#include "main.h"
#include "watek_glowny.h"

void mainLoop() {
    srandom(rank);
    int tag;
	int perc;
	packet_t* pkt = nullptr;

    while (true) {
	switch (state) {
	case WantGroup:
		// 1.1 Proces wysyła do wszystich REQQUEUE i przechodzi do stanu WaitingForQueue
		println("Szukam grupy - wysyłam REQQUEUE");
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
		println("Czekam na uformowanie grupy");
		
		// 1.4 i 1.7 Proces sprawdza czy nie jest liderem
		println("Sprawdzam czy jestem liderem");
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
			free(pkt);
			println("Wysłałem wszystkim GROUPFORMED");

			// 1.5.3 Proces usuwa z groupQueueMut procesy z list groupMembers
			pthread_mutex_lock(&groupQueueMut);
			for (int i = 0; i < GROUPSIZE; i++) {
				auto it = std::find_if(groupQueue.begin(), groupQueue.end(), [i](const idLamportPair& s) { return s.id == groupMembers[i]; });
				if (it != groupQueue.end()) {
					groupQueue.erase(it);
				}
			}
			pthread_mutex_unlock(&groupQueueMut);

			changeState(Leader);
			break;
		}
		pthread_mutex_lock(&waitingForGroupMut);
		break;

	case Leader:
		// 2.1
		// 2.1.1 Proces wysyła do wszystich REQQUEUE i przechodzi do stanu WaitingForQueue
		println("Wysyłam REQRES");
		
		pkt = new packet_t;

		pthread_mutex_lock(&lamportMut);
		lamport++;
		pkt->ts = lamport;
		lamportREQQUEUE = lamport;
		pthread_mutex_unlock(&lamportMut);
		sendPacketToAllNoInc(pkt, REQRES);
		free(pkt);
		println("Wysłałem wszystkie REQRES");
		pthread_mutex_lock(&waitingForResMut);
		changeState(WaitingForRes);
		break;

	case WaitingForRes:
		print("Sprawdzam czy mogę korzystać z zasobu, procesy w ResQueue: { ");
		for (const auto& element : resQueue) {
			printNoTag("[%d %d] ", element.id, element.lamport);
		}printNoTag("}\n");

		pthread_mutex_lock(&resQueueMut);
		for (int i = 0; i < RESNUM; i++) {
			if (resQueue[i].id == rank) {
				// Wysłanie START
				pkt = new packet_t;
				pthread_mutex_lock(&lamportMut);
				lamport++;
				pkt->ts = lamport;
				pthread_mutex_unlock(&lamportMut);
				println("Grupa może korzystać z zasobu, wysyłam start");
				for (int j = 0; j < GROUPSIZE; j ++) {
					sendPacket(pkt, groupMembers[j], START);
				}
				free(pkt);

				println("Wchodzę do sekcji");
				changeState(InSection);
				pthread_mutex_unlock(&waitingForRelease);
				break;
			}
		}
		pthread_mutex_unlock(&resQueueMut);
		pthread_mutex_lock(&waitingForRelease);
		break;

	case Member:
		print("* Jestem członkiem grupy i czekam na start, skład grupy to:");
		for (int i = 0; i < GROUPSIZE; i++) {
			printNoTag(" %d", groupMembers[i]);
		} printNoTag("\n");

		pthread_mutex_lock(&waitingForStartMut);
		println("Wchodzę do sekcji");
		changeState(InSection);
		break;

	case InSection:
		print(" ** Jestem w sekcji z ");
		for (int i = 0; i < GROUPSIZE; i++) {
			printNoTag(" %d", groupMembers[i]);
		} printNoTag(" **\n")
		sleep(TIMEINSECTION);

		// 3.1 Jeżeli jestem liderem
		if (groupMembers[0] == rank) {
			println("Wysyłam END do wszystkich procesów");
			pkt = new packet_t;
			pthread_mutex_lock(&lamportMut);
			lamport++;
			pkt->ts = lamport;
			pthread_mutex_unlock(&lamportMut);
			sendPacketToAllWithMeNoInc(pkt, END);
			free(pkt);
		}

		// 3.3
		perc = random()%100;
		if (perc < BREAKPROB) {
			println(" ** Wychodzę z sekcji ** ");
			changeState(Break);
			break;
		}
		
		// 3.4
		println(" ** Wychodzę z sekcji ** ");
		changeState(WantGroup);
		break;


	case Break:
		println("Nałożono przerwę");
		sleep(BREAKTIME);
		println("Odczekano przerwę");
		changeState(WantGroup);

	default:
		break;
	}
    sleep(SEC_IN_STATE);
    }
}
