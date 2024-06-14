#include "main.h"
#include "state_thread.h"

void mainLoop() {
    srandom(rank);
	packet_t* packet = nullptr;

    while (true) {
	switch (state) {
	case WantGroup:
		/* Sending REQQUEUE to all */
		println("Looking for group - sending REQQUEUE");
		packet = new packet_t;
		pthread_mutex_lock(&lamportMut);
		lamport++;
		packet->ts = lamport;
		lamportREQQUEUE = lamport;
		pthread_mutex_unlock(&lamportMut);
		sendPacketToAllNoInc(packet, REQQUEUE);
		free(packet);
		println("REQQUEUE sent");

		/* Waiting for all ACKQUEUE */
		pthread_mutex_lock(&waitingForQueueMut);
		changeState(WaitingForGroup);
		println("Waiting for the group to form");
		break;

	case WaitingForGroup:
		println("Checking if im not the leader");
		/* If leader */
		if (groupQueue.size() >= GROUPSIZE && groupQueue[0].id == rank) {
			/* Filling groupMembers */
			pthread_mutex_lock(&groupQueueMut);
			for (int i = 0; i < GROUPSIZE; i++) {
				groupMembers[i] = groupQueue[i].id;
			}
			pthread_mutex_unlock(&groupQueueMut);

			print("* Im the leader, group members are:");
			for (int i = 0; i < GROUPSIZE; i++) {
				printNoTag(" %d", groupMembers[i]);
			} printNoTag(" *\n");
			
			/* Sending GROUPFORMED to all*/
			for (int i = 0; i < GROUPSIZE; i++) {
				packet->inGroup[i] = groupMembers[i];
			}
			pthread_mutex_lock(&lamportMut);
			lamport++;
			packet->ts = lamport;
			pthread_mutex_unlock(&lamportMut);
			sendPacketToAllNoInc(packet, GROUPFORMED);
			free(packet);
			println("GROUPFORMED sent");

			/* Remove group members from the queue of processes waiting for the group */
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
		/* Unlocks when com_thread receives GROUPFORMED or REQQUEUE */
		pthread_mutex_lock(&waitingForGroupMut);
		break;

	case Leader:
		/* Sending REQRES to all */
		println("Sending REQRES");
		packet = new packet_t;
		pthread_mutex_lock(&lamportMut);
		lamport++;
		packet->ts = lamport;
		lamportREQQUEUE = lamport;
		pthread_mutex_unlock(&lamportMut);
		sendPacketToAllNoInc(packet, REQRES);
		free(packet);
		println("REQRES sent");

		/* Unlocks after receiving all ACKRES */
		pthread_mutex_lock(&waitingForResMut);
		changeState(WaitingForRes);
		break;

	case WaitingForRes:
		print("Checking if I can use the resource, leaders waiting for the resource are: { ");
		for (const auto& element : resQueue) {
			printNoTag("[%d %d] ", element.id, element.lamport);
		}printNoTag("}\n");

		pthread_mutex_lock(&resQueueMut);
		for (int i = 0; i < RESNUM; i++) {
			/* If I'm below RESNUM in queue, i can start*/
			if (resQueue[i].id == rank) {
				/* Sending START to group members */
				println("Group can enter critical section, sending START to group members");
				packet = new packet_t;
				pthread_mutex_lock(&lamportMut);
				lamport++;
				packet->ts = lamport;
				pthread_mutex_unlock(&lamportMut);
				for (int j = 0; j < GROUPSIZE; j ++) {
					sendPacket(packet, groupMembers[j], START);
				}
				free(packet);
				println("START sent, I can enter critical section now");
				changeState(InSection);
				pthread_mutex_unlock(&waitingForRelease);
				break;
			}
		}
		pthread_mutex_unlock(&resQueueMut);

		/* Unlocks after receiving END */
		pthread_mutex_lock(&waitingForRelease);
		break;

	case Member:
		print("* I found a group and I'm waiting for start, the group members are:");
		for (int i = 0; i < GROUPSIZE; i++) {
			printNoTag(" %d", groupMembers[i]);
		} printNoTag(" *\n");

		/* Unlocks after receiving START */
		pthread_mutex_lock(&waitingForStartMut);
		println("I can enter critical section now");
		changeState(InSection);
		break;

	case InSection:
		print(" ** Im in critical section with:");
		for (int i = 0; i < GROUPSIZE; i++) {
			printNoTag(" %d", groupMembers[i]);
		} printNoTag(" **\n")

		/* Perform some action */
		sleep(TIMEINSECTION);

		/* If I'm a leader */
		if (groupMembers[0] == rank) {
			/* Send END to all */
			println("Sending END");
			packet = new packet_t;
			pthread_mutex_lock(&lamportMut);
			lamport++;
			packet->ts = lamport;
			pthread_mutex_unlock(&lamportMut);
			sendPacketToAllWithMeNoInc(packet, END);
			free(packet);
		}

		println(" ** Leaving critical section ** ");
		/* If a break has been imposed */
		if (random() % 100 < BREAKPROB) {
			changeState(Break);
			break;
		}
		/* Else */
		changeState(WantGroup);
		break;


	case Break:
		println("Break has been imposed");
		sleep(BREAKTIME);
		println("The break has been waited");
		changeState(WantGroup);

	default:
		break;
	}

	/* Sleep for better debuging */
    sleep(SEC_IN_STATE);
    }
}
