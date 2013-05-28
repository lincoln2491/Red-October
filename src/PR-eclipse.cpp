#include <iostream>
#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <vector>
#include <stddef.h>
#include <algorithm>
#include <sstream>

#include "structsAndProperites.h"

using namespace std;

//TODO synchronizacja wątków

/*
 * Zmienne używane w programie
 */

//dane dotyczące struktury przesyłanej
const int msg_items = 6;
int msg_block_lengths[6] = { 1, 1, 1, 1, 1, 1 };
MPI_Datatype msg_types[6] = { MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT };
MPI_Datatype mpi_msg_type;
MPI_Aint msg_offsets[6];

int listOfPreference[numberOfChanels]; 							//tablica zawierająca preferencje kanałów
Channel *channels;												//tablica zawierająca dane wszyskich kanałów
int rank, size;													//numer procesu i ogólna liczba procesów
int scalarClock = 0;   											//zegar
vector<vector<Request*> > demand;								//kolejki żądań do każdej z stref krytycznych

int *stamps;													// lamport
bool *isAnswer;													// lamport

pthread_t requestThread;

//mutexy

pthread_mutex_t clockMutex;
pthread_mutex_t *demandMutex;
pthread_mutex_t *stampsMutex;
pthread_mutex_t *channelsMutex;
pthread_mutex_t checkAnswerMutex;
pthread_mutex_t checkDemandMutex;

void clearStringStream( ostringstream &aSS ) {
	aSS.str( "" );
	aSS.clear();
}

void createLog( string aLog ) {
	ostringstream ss;
	clearStringStream( ss );
	ss << rank;
	string log = "PID: " + ss.str();
	pthread_mutex_lock( &clockMutex );
	clearStringStream( ss );
	ss << scalarClock;
	pthread_mutex_unlock( &clockMutex );
	log += " time: " + ss.str() + " msg: " + aLog + "\n";
	cout << log;
}

void incrementClock( int aValue ) {
	pthread_mutex_lock( &clockMutex );
	scalarClock = max( scalarClock, aValue ) + 1;
	pthread_mutex_unlock( &clockMutex );
}

void printChannelStatus( int aNO ) {
	pthread_mutex_lock( &channelsMutex[aNO] );
	ostringstream ss;
	clearStringStream( ss );
	ss << aNO;
	string log = "Kanał numer: " + ss.str();
	clearStringStream( ss );
	ss << channels[aNO].numberOfShips;
	log += ". Liczba statków: " + ss.str();
	clearStringStream( ss );
	ss << channels[aNO].maxNumberOfShips;
	log += ". Maksymalnie: " + ss.str();
	clearStringStream( ss );
	ss << channels[aNO].direction;
	log += ". Kierunek: " + ss.str();
	incrementClock( 0 );
	createLog( log );
}

/*
 * Funkcja odpowiedzialna za odpowiadanie na żądania
 */
void* requestMonitor( void* arg ) {
	int myId;
	//createLog( "Początek słuchania żądań" );
	MPI_Comm_rank( MPI_COMM_WORLD, &myId );
	Request *request;
	Msg req;
	Msg ans;
	MPI_Status status;
	ostringstream ss;

	while ( true ) {
		MPI_Recv( &req, 1, mpi_msg_type, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status );

		incrementClock( req.clock );

		if ( status.MPI_TAG == requestTag ) {
			ans.channelNO = req.channelNO;

			pthread_mutex_lock( &clockMutex );
			ans.clock = scalarClock;
			pthread_mutex_unlock( &clockMutex );
			ans.pidSend = rank;
			ans.pidRecv = req.pidSend;

			request = new Request();
			request->channelNO = req.channelNO;
			request->clock = req.clock;
			request->pid = req.pidSend;

			MPI_Send( &ans, 1, mpi_msg_type, req.pidSend, answerTag, MPI_COMM_WORLD );

			clearStringStream( ss );
			ss << req.pidSend;
			string log = "Prośba od " + ss.str() + " o dostęp do kanału ";
			clearStringStream( ss );
			ss << req.channelNO;
			log += ss.str();
			createLog( log );
			pthread_mutex_lock( &demandMutex[request->channelNO] );
			demand[request->channelNO].push_back( request );
			pthread_mutex_unlock( &demandMutex[request->channelNO] );

		} else if ( status.MPI_TAG == dismisTag ) {
			clearStringStream( ss );
			ss << req.pidSend;
			string log = "Prośba od " + ss.str() + " o zwolnienie kanału ";
			clearStringStream( ss );
			ss << req.channelNO;
			log += ss.str();
			createLog( log );

			int channel = req.channelNO;
			pthread_mutex_lock( &channelsMutex[channel] );
			channels[channel].direction = req.direction;
			channels[channel].numberOfShips = req.numberOfShips;
			pthread_mutex_unlock( &channelsMutex[channel] );

			pthread_mutex_lock( &demandMutex[channel] );
			for ( int i = 0; i < demand[channel].size(); i++ ) {
				if ( demand[channel][i]->pid == req.pidSend ) {
					Request *r;
					r = demand[channel][i];
					demand[channel].erase( demand[channel].begin() + i - 1 );
					delete r;
					break;
				}
			}
			pthread_mutex_unlock( &demandMutex[channel] );
			pthread_mutex_unlock( &checkDemandMutex );
		} else {
			clearStringStream( ss );
			ss << req.pidSend;
			string log = "Dostanie informacji od procesu " + ss.str() + " na temat kanału ";
			clearStringStream( ss );
			ss << req.channelNO;
			log += ss.str();
			createLog( log );

			pthread_mutex_lock( &stampsMutex[req.pidSend] );
			stamps[req.pidSend] = req.clock;
			isAnswer[req.pidSend] = true;
			pthread_mutex_unlock( &stampsMutex[req.pidSend] );
			pthread_mutex_unlock( &checkAnswerMutex );
		}
	}
}

/*
 * przesylanie do innych procesow mojego stanu
 */
void broadcastRequest( Request *aRequest ) {
	MPI_Status status;
	ostringstream ss;

	int tag = requestTag;

	clearStringStream( ss );
	ss << aRequest->channelNO;
	string log = "Wysyłanie żądania o dostęp do kanału " + ss.str();
	createLog( log );

	Msg msg;
	msg.channelNO = aRequest->channelNO;
	msg.clock = aRequest->clock;
	msg.pidSend = rank;
	for ( int i = 0; i < size; i++ ) {
		if ( i != rank ) {
			msg.pidRecv = i;
			MPI_Send( &msg, 1, mpi_msg_type, i, tag, MPI_COMM_WORLD );
		}
	}
}

/*
 *	Funkcja pozorująca płynięcie kanałem
 */
void swiming( int aChannelNumber ) {
	int time = rand() % 1000;
	ostringstream ss;
	clearStringStream( ss );
	ss << aChannelNumber;
	string log = "Początek płynięcia kanałem  " + ss.str() + ". Będzie to trwało ";
	clearStringStream( ss );
	ss << time;
	log += ss.str();
	incrementClock( 0 );
	createLog( log );

	usleep( time * 1000 );

	clearStringStream( ss );
	ss << aChannelNumber;
	log = "Koniec płynięcia kanałem " + ss.str() + ". Trwało to ";
	clearStringStream( ss );
	ss << time;
	log += ss.str();
	incrementClock( 0 );
	createLog( log );
}

/*
 * Funkcja pozorująca walkę
 */
void fighting() {
	int time = rand() % 1000;
	ostringstream ss;
	clearStringStream( ss );
	ss << time;
	string log = "Początek walki która będzie trwala " + ss.str();
	incrementClock( 0 );
	createLog( log );

	usleep( time * 1000 );

	clearStringStream( ss );
	ss << time;
	log = "Koniec walki trwającej " + ss.str();
	incrementClock( 0 );
	createLog( log );
}

/*
 *  Funkcja opuszczenia kanału
 */
void leafChannel( int aChannelNumber ) {
	ostringstream ss;
	clearStringStream( ss );
	ss << aChannelNumber;
	string log = "Opuszczam kanał " + ss.str();
	incrementClock( 0 );
	createLog( log );
}

/*
 * Funkcja sprawdzająca, czy można wpłynąć do kanłu
 */
bool goToCriticalSection( int aChannelNumber, int aDirection, bool aIsSwimmingToCanal ) {
	bool ret = false;
	MPI_Status status;

	bool isAllAnswer = false;

//gdy kolejka żądań jest większa niż maksymalna ilość
	pthread_mutex_lock( &demandMutex[aChannelNumber] );
	if ( demand[aChannelNumber].size() > maxDemandSize && aIsSwimmingToCanal ) {
		pthread_mutex_unlock( &demandMutex[aChannelNumber] );
		return false;
	}
	pthread_mutex_unlock( &demandMutex[aChannelNumber] );

	incrementClock( 0 );

	for ( int i = 0; i < size; i++ ) {
		if ( i == rank ) {
			pthread_mutex_lock( &clockMutex );
			pthread_mutex_lock( &stampsMutex[i] );
			stamps[i] = scalarClock;
			isAnswer[i] = true;
			pthread_mutex_unlock( &clockMutex );
			pthread_mutex_unlock( &stampsMutex[i] );
		} else {
			pthread_mutex_lock( &stampsMutex[i] );
			stamps[i] = 0;
			isAnswer[i] = false;
			pthread_mutex_unlock( &stampsMutex[i] );
		}
	}
	Request *request = new Request();
	request->channelNO = aChannelNumber;
	request->pid = rank;

	pthread_mutex_lock( &clockMutex );
	request->clock = scalarClock;
	pthread_mutex_unlock( &clockMutex );

	pthread_mutex_lock( &demandMutex[aChannelNumber] );
	demand[aChannelNumber].push_back( request );
	pthread_mutex_unlock( &demandMutex[aChannelNumber] );

	broadcastRequest( request );

//TODO to trzeba przed każdym z while jakiś mutex

	while ( !isAllAnswer ) {
		pthread_mutex_lock( &checkAnswerMutex );
		isAllAnswer = true;
		for ( int i = 0; i < size; i++ ) {
			if ( i == rank ) {
				continue;
			}

			pthread_mutex_lock( &stampsMutex[i] );
			if ( isAnswer[i] == false || stamps[i] <= stamps[rank] ) {
				isAllAnswer = false;
				pthread_mutex_unlock( &stampsMutex[i] );
				break;
			}
			pthread_mutex_unlock( &stampsMutex[i] );
		}
	}

	incrementClock( 0 );
	createLog( "Dostałem wszystkie odpowiedzi" );

//TODO to trzeba przed każdym z while jakiś mutex
	while ( true ) {
		pthread_mutex_lock( &checkDemandMutex );
		pthread_mutex_lock( &demandMutex[aChannelNumber] );
		if ( demand[aChannelNumber][0]->pid == rank ) {
			pthread_mutex_unlock( &demandMutex[aChannelNumber] );
			break;
		}
		pthread_mutex_unlock( &demandMutex[aChannelNumber] );
	}

	incrementClock( 0 );
	createLog( "Jestem pierwszy w kolejce żądań" );

//wchodze do sekcji krytycznej
	pthread_mutex_lock( &channelsMutex[aChannelNumber] );
	if ( !aIsSwimmingToCanal ) {													//wersja jak opusczamy kanał
		ret = true;
		channels[aChannelNumber].numberOfShips--;
	} else if ( ((channels[aChannelNumber].direction == aDirection) 			//wersja jak idziemy do kanału
	&& (channels[aChannelNumber].numberOfShips < channels[aChannelNumber].maxNumberOfShips)) || (channels[aChannelNumber].numberOfShips == 0) ) {
		channels[aChannelNumber].direction == aDirection;
		channels[aChannelNumber].numberOfShips++;
		ret = true;
	}

	incrementClock( 0 );
	createLog( "Sprawdziłem kanał" );

	Msg dismis;
	dismis.channelNO = aChannelNumber;
	dismis.direction = channels[aChannelNumber].direction;
	dismis.numberOfShips = channels[aChannelNumber].numberOfShips;

	pthread_mutex_unlock( &channelsMutex[aChannelNumber] );

	pthread_mutex_lock( &clockMutex );
	dismis.clock = scalarClock;
	pthread_mutex_unlock( &clockMutex );
	dismis.pidSend = rank;
	for ( int i = 0; i < size; i++ ) {
		if ( i != rank ) {
			dismis.pidRecv = i;
			MPI_Send( &dismis, 1, mpi_msg_type, i, dismisTag, MPI_COMM_WORLD );
		}
	}
	pthread_mutex_lock( &demandMutex[aChannelNumber] );
	Request *r = demand[aChannelNumber][0];
	demand[aChannelNumber].erase( demand[aChannelNumber].begin() );
	pthread_mutex_unlock( &demandMutex[aChannelNumber] );

	delete r;

	incrementClock( 0 );
	createLog( "Opuściłem sekcję krytyczną" );
//wychodze z sekcji krytycznej
	return ret;
}

/*
 * Funkcja wejscia do kanału, jak już został wybrany
 */
void goToChannel( int id, int direction ) {
	ostringstream ss;
	clearStringStream( ss );
	ss << id;
	string log = "Wpływam do kanału " + ss.str();
	clearStringStream( ss );
	ss << direction;
	log += " w kierunku " + ss.str();
	incrementClock( 0 );
	createLog( log );
}

/*
 * Funkcja ustawiająca wszelkie zmienne
 */
void configure() {
	channels = new Channel[numberOfChanels];

	isAnswer = new bool[size];
	stamps = new int[size];

	demand.resize( numberOfChanels );

//ustalanie preferencji
	srand( rank );
	for ( int i = 0; i < numberOfChanels; i++ ) {
		swap( listOfPreference[i], listOfPreference[rand() % numberOfChanels] );
		channels[i].maxNumberOfShips = (10 * i) % 30;
		channels[i].direction = 0;
		channels[i].numberOfShips = 0;
	}

	msg_offsets[0] = offsetof(Msg, pidSend);
	msg_offsets[1] = offsetof(Msg, pidRecv);
	msg_offsets[2] = offsetof(Msg, clock);
	msg_offsets[3] = offsetof(Msg, channelNO);
	msg_offsets[4] = offsetof(Msg, numberOfShips);;
	msg_offsets[5] = offsetof(Msg, direction);

	MPI_Type_create_struct( msg_items, msg_block_lengths, msg_offsets, msg_types, &mpi_msg_type );
	MPI_Type_commit( &mpi_msg_type );

	demandMutex = new pthread_mutex_t[numberOfChanels];
	stampsMutex = new pthread_mutex_t[size];
	channelsMutex = new pthread_mutex_t[numberOfChanels];

	pthread_mutex_init( &clockMutex, NULL );
	pthread_mutex_init( &checkAnswerMutex, NULL );
	pthread_mutex_init( &checkDemandMutex, NULL );
	for ( int i = 0; i < numberOfChanels; i++ ) {
		pthread_mutex_init( &demandMutex[i], NULL );
		pthread_mutex_init( &channelsMutex[i], NULL );
	}
	for ( int i = 0; i < size; i++ ) {
		pthread_mutex_init( &stampsMutex[i], NULL );
	}

	pthread_create( &requestThread, NULL, requestMonitor, NULL );

}

int main( int argc, char **argv ) {
	/*
	 * inicjalizacja srodowiska
	 */

	MPI_Init( &argc, &argv );
	MPI_Comm_size( MPI_COMM_WORLD, &size );
	MPI_Comm_rank( MPI_COMM_WORLD, &rank );

	int l;
	MPI_Query_thread( &l );
	if ( l == MPI_THREAD_SINGLE ) {
		cout << 1 << endl;
	}
	if ( l == MPI_THREAD_FUNNELED ) {
		cout << 2 << endl;
	}
	if ( l == MPI_THREAD_SERIALIZED ) {
		cout << 3 << endl;
	}
	if ( l == MPI_THREAD_MULTIPLE ) {
		cout << 4 << endl;
	}

	//	configure();

	//do testowania
//	if ( rank == 0 ) {
//
//		goToCriticalSection( 1, 1, true );
//		printChannelStatus( 1 );
//		createLog( "trzeba kurwa wrocic" );
//		//goToCriticalSection( 1, 1, false );
//		printChannelStatus( 1 );
//	}
//
//	while ( true ) {
//
//	}

	//główna pętla programu
//	while ( true ) {
//		int channelToSwim;
//		bool isChoosen = false;
//
//		//tu trzeba wybrać jakim kanałem płynąć
//
//		while ( !isChoosen ) {
//			for ( int i = 0; i < numberOfChanels; ++i ) {
//				if ( goToCriticalSection( listOfPreference[i], 1, true ) ) {
//					channelToSwim = i;
//					isChoosen = true;
//					break;
//				}
//			}
//		}
//		incrementClock(0);
//		goToChannel( channelToSwim, 1 );
//		swiming( channelToSwim );
//
//		goToCriticalSection( channelToSwim, 1, false );
//
//		incrementClock(0);
//		leafChannel( channelToSwim );
//
//		incrementClock(0);
//		fighting();
//
//		isChoosen = false;
//		while ( !isChoosen ) {
//			for ( int i = 0; i < numberOfChanels; ++i ) {
//				if ( goToCriticalSection( listOfPreference[i], 0, true ) ) {
//					channelToSwim = i;
//					isChoosen = true;
//					break;
//				}
//			}
//		}
//
//		incrementClock(0);
//		goToChannel( channelToSwim, 0 );
//		swiming( channelToSwim );
//
//		goToCriticalSection( channelToSwim, 1, true );
//
//		incrementClock(0);
//		leafChannel( channelToSwim );
//
//		int time = rand() % 1000;
//		usleep( time * 1000 );
//
//	}

	MPI_Type_free( &mpi_msg_type );
	MPI_Finalize();
	return 0;
}
