#include "main.h"
#include "util.h"

MPI_Datatype MPI_PAKIET_T;
state_t state = WantGroup;

pthread_mutex_t stateMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lamportMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t groupQueueMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t waitingForQueueMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t waitingForGroupMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t resQueueMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t waitingForResMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t waitingForStartMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t leadersMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t waitingForRelease = PTHREAD_MUTEX_INITIALIZER;

std::vector<idLamportPair> groupQueue;
int groupMembers[GROUPSIZE];
std::vector<int> leaders;
std::vector<idLamportPair> resQueue;

struct tagNames_t {
    const char *name;
    int tag;
} tagNames[] = {
    {"REQQUEUE", REQQUEUE}, {"ACKQUEUE", ACKQUEUE}, {"GROUPFORMED", GROUPFORMED},
    {"REQRES", REQRES}, {"ACKRES", ACKRES}, {"START", START}, {"END", END}
};

const char *const tag2string(int tag) {
    for (int i = 0; i < sizeof(tagNames) / sizeof(struct tagNames_t); i++) {
	    if ( tagNames[i].tag == tag )  return tagNames[i].name;
    }
    return "<unknown>";
}

/* tworzy typ MPI_PAKIET_T */
void inicjuj_typ_pakietu() {
    int blocklengths[NITEMS] = {1,1,GROUPSIZE};
    MPI_Datatype typy[NITEMS] = {MPI_INT, MPI_INT, MPI_INT};
    MPI_Aint offsets[NITEMS]; 
    offsets[0] = offsetof(packet_t, ts);
    offsets[1] = offsetof(packet_t, src);
    offsets[2] = offsetof(packet_t, inGroup);

    MPI_Type_create_struct(NITEMS, blocklengths, offsets, typy, &MPI_PAKIET_T);
    MPI_Type_commit(&MPI_PAKIET_T);
}

void sendPacket(packet_t *pkt, int destination, int tag) {
    int freepkt = 0;
     if (pkt == nullptr) { 
        pkt = (packet_t*)malloc(sizeof(packet_t)); 
        freepkt = 1;
    }
    pkt->src = rank;
    pthread_mutex_lock( &lamportMut );
    lamport++;
    pkt->ts = lamport;
    pthread_mutex_unlock( &lamportMut );
    MPI_Send( pkt, 1, MPI_PAKIET_T, destination, tag, MPI_COMM_WORLD);
    debugln("[S] %s do %d", tag2string(tag), destination);
    if (freepkt) free(pkt);
}

void sendPacketNoInc(packet_t *pkt, int destination, int tag) {
    int freepkt = 0;
     if (pkt == nullptr) { 
        pkt = (packet_t*)malloc(sizeof(packet_t)); 
        freepkt = 1;
    }
    pkt->src = rank;
    MPI_Send( pkt, 1, MPI_PAKIET_T, destination, tag, MPI_COMM_WORLD);
    debugln("[S] %s do %d", tag2string(tag), destination);
    if (freepkt) free(pkt);
}

void sendPacketToAllNoInc(packet_t *pkt, int tag) {
    for (int i=0; i<= size - 1; i++)
        if (i!=rank) {
            int freepkt = 0;
            if (pkt == nullptr) { 
                pkt = (packet_t*)malloc(sizeof(packet_t)); 
                freepkt = 1;
            }
            pkt->src = rank;
            MPI_Send( pkt, 1, MPI_PAKIET_T, i, tag, MPI_COMM_WORLD);
            debugln("Wysłano %s do %d", tag2string(tag), i);
            if (freepkt) free(pkt);
        }
}

void sendPacketToAllWithMeNoInc(packet_t *pkt, int tag) {
    for (int i=0; i<= size - 1; i++) {
        pkt->src = rank;
        MPI_Send( pkt, 1, MPI_PAKIET_T, i, tag, MPI_COMM_WORLD);
        debugln("Wysłano %s do %d", tag2string(tag), i);
    }
}

void changeState( state_t newState ) {
    pthread_mutex_lock( &stateMut );
    state = newState;
    pthread_mutex_unlock( &stateMut );
}
