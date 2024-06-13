#ifndef UTILH
#define UTILH
#include "main.h"

#define RESNUM 2
#define GROUPSIZE 2
#define BREAKTIME 5
#define BREAKPROB 25 // %
#define TIMEINSECTION 5

#define REQQUEUE    1
#define ACKQUEUE    2
#define GROUPFORMED 3
#define REQRES      4
#define ACKRES      5
#define START       6
#define END         7

/* typ pakietu */
typedef struct {
    int ts;
    int src;  
    int inGroup[GROUPSIZE];
} packet_t;
#define NITEMS 3

typedef enum {
    WantGroup, WaitingForGroup, Leader, Member, WaitingForRes, InSection
} state_t;

typedef struct {
    int id;
    int lamport;
} idLamportPair;

extern MPI_Datatype MPI_PAKIET_T;
extern state_t state;

extern pthread_mutex_t stateMut;
extern pthread_mutex_t lamportMut;
extern pthread_mutex_t groupQueueMut;
extern pthread_mutex_t waitingForQueueMut;
extern pthread_mutex_t waitingForGroupMut;
extern pthread_mutex_t resQueueMut;
extern pthread_mutex_t waitingForResMut;
extern pthread_mutex_t waitingForStartMut;
extern pthread_mutex_t waitingForRelease;

extern std::vector<idLamportPair> groupQueue;
extern int groupMembers[GROUPSIZE];
extern std::vector<idLamportPair> resQueue;

void inicjuj_typ_pakietu();

/* wysyłanie pakietu, skrót: wskaźnik do pakietu (0 oznacza stwórz pusty pakiet), do kogo, z jakim typem */
void sendPacket(packet_t *pkt, int destination, int tag);
void sendPacketToAllNoInc(packet_t *pkt, int tag);
void sendPacketNoInc(packet_t *pkt, int destination, int tag);
void sendPacketToAllWithMeNoInc(packet_t *pkt, int tag);
void changeState(state_t);
#endif