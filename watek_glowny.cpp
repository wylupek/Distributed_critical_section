#include "main.h"
#include "watek_glowny.h"

void mainLoop() {
    srandom(rank);
    int tag;
	packet_t* pkt = nullptr;

    while (true) {
	switch (state) {
	    case WantGroup: 
			println("Szukam grupy")
			pkt = new packet_t;
			pkt->data = 0;

			pthread_mutex_lock(&lamportMut);
			lamport++;
			pkt->ts = lamport;
			lamportREQQUEUE = lamport;
			pthread_mutex_unlock(&lamportMut);

			sendPacketToAllNoInc(pkt, REQQUEUE);
			changeState( WaitingForQueue );
			free(pkt);
			debug("Wysłałem wszystkie prośby o dołączenie do kolejki");
			break;

	    case WaitingForQueue:
			println("Czekam na wejście do kolejki")
			pthread_mutex_lock(&groupQueueMut);
			changeState(InQueue);
			break;

		case InQueue:
			println("Czekam w kolejce na uformowanie grupy");
			changeState(InFinish);
			sendPacket(0, rank, END);
			break;

		case InFinish:
			println("Wychodzę z sekcji");
			sendPacket(0, rank, END);
			return;
		default:
			return;
		// case InSection:
		// 	// tutaj zapewne jakiś muteks albo zmienna warunkowa
		// 	println("Jestem w sekcji krytycznej")
		// 	sleep(5);
		// 	//if ( perc < 25 ) {
		// 	debug("Perc: %d", perc);
		// 	println("Wychodzę z sekcji krytycznej")
		// 	debug("Zmieniam stan na wysyłanie");
		// 	pkt = new packet_t;
		// 	pkt->data = perc;
		// 	for (int i=0;i<=size-1;i++)
		// 	if (i!=rank)
		// 		sendPacket( pkt, (rank+1)%size, RELEASE);
		// 	changeState( InRun );
		// 	free(pkt);
		// 	//}
		// 	break;
	}
    sleep(SEC_IN_STATE);
    }
}
