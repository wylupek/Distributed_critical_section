#include "main.h"
#include "watek_glowny.h"

void mainLoop() {
    srandom(rank);
    int tag;
	packet_t* pkt = nullptr;

    while (true) {
	switch (state) {
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
			if (leader == true) {
				pthread_mutex_lock(&lamportMut);
				for (int i = 0; i < GROUPSIZE; i++) {
					inGroup[i] = groupQueue[i].id;
				}
				pthread_mutex_unlock(&lamportMut);

				// Print group members
				print("**** Jestem liderem grupy, skład grupy to:");
				for (int i = 0; i < GROUPSIZE; i++) {
					printNoTag(" %d", inGroup[i]);
				} printNoTag("\n");

				for (int i = 0; i < GROUPSIZE; i++) {
					pkt->inGroup[i] = inGroup[i];
				}

				pthread_mutex_lock(&lamportMut);
				lamport++;
				pkt->ts = lamport;
				pthread_mutex_unlock(&lamportMut);

				sendPacketToAllNoInc(pkt, GROUPFORMED);
			}
			changeState(InFinish);
			sendPacket(0, rank, END);
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
