#include "utilities.h"
using namespace std;
/*
 * Zmienne
 */

MPI_Request mpiRequest;											//request
MPI_Status mpiStatus;											//status

ostringstream ss;												//strumień do konwertowania liczb na stringi

int rank, size;													//numer procesu i ogólna liczba procesów
int scalarClock;   												//zegar

int mode;														//tryb działania propgramu
int choosenChannel;												//wybrany kanal
int actualThinkingChanel;										//pozycja na liscie preferencji aktualnie przemyślanego kanału
int direction;													//jaki kierunke (1 - do walki, 0 - powrót)
bool isGoingToChannel;											//zmienna oznaczająca, czy chcemy wejsc do kanału, czy wyjsc z niego

int *stamps;													// lamport - znaczniki czasowe
bool *isAnswer;													// lamport - czy wszystkie kanały odpowiedziały

vector<Request> demand[NUMBERS_OF_CHANNELS];					//kolejki żądań do każdej z stref krytycznych
int listOfPreference[NUMBERS_OF_CHANNELS]; 						//tablica zawierająca preferencje kanałów
Channel channels[NUMBERS_OF_CHANNELS];							//tablica zawierająca dane wszyskich kanałów

/*
 * Dane dotyczące struktury przesyłanej
 */
const int msg_items = 4;
int msg_block_lengths[4] = { 1, 1, 1, 1 };
MPI_Datatype msg_types[4] = { MPI_INT, MPI_INT, MPI_INT, MPI_INT };
MPI_Datatype MPI_MSG_TYPE;
MPI_Aint msg_offsets[4];

/*
 * Dodaje żadanie do kolejki żądań
 */
void addDemand( Request aRequest ) {
	int channel = aRequest.channelNO;
	int pid = aRequest.pid;
	int clock = aRequest.clock;
	bool added = false;
	//Sprawdza miejsce na liscie
	for ( int i = 0; i < demand[channel].size(); i++ ) {
		Request r = demand[channel][i];
		if ( (r.clock < clock) || (r.clock == clock && r.pid < pid) ) {
			continue;
		}
		added = true;
		demand[channel].insert( demand[channel].begin() + i, { pid, clock, channel } );
		break;
	}
	if ( !added ) {

		demand[channel].push_back( { pid, clock, channel } );

	}
}
/*
 * Sprawdzanie wiadomosci typu request
 */
void checkRequest() {
	int ms = 50;
	usleep( ms * 1000 );
	Request channelRequest;
	Msg req;
	Msg ans;
	int test;
	MPI_Irecv( &req, 1, MPI_MSG_TYPE, MPI_ANY_SOURCE, REQUEST_TAG, MPI_COMM_WORLD, &mpiRequest );
	MPI_Test( &mpiRequest, &test, &mpiStatus );

	if ( test == 1 ) {
		scalarClock = max( scalarClock, req.clock ) + 1;

		ans.channelNO = req.channelNO;
		ans.clock = scalarClock;

		channelRequest.channelNO = req.channelNO;
		channelRequest.clock = req.clock;
		channelRequest.pid = mpiStatus.MPI_SOURCE;

		MPI_Send( &ans, 1, MPI_MSG_TYPE, mpiStatus.MPI_SOURCE, ANSWER_TAG, MPI_COMM_WORLD );

		addDemand( channelRequest );

		clearStringStream( ss );
		ss << mpiStatus.MPI_SOURCE;
		string log = "Prośba od " + ss.str() + " o dostęp do kanału ";
		clearStringStream( ss );
		ss << req.channelNO;
		log += ss.str();
		createLog( log, ss, rank, scalarClock );

	}
}

/*
 * Sprawdzanie wiadomosci typu dismis
 */
void checkDismis() {
	int ms = 50;
	usleep( ms * 1000 );
	Msg req;
	int test;
	MPI_Irecv( &req, 1, MPI_MSG_TYPE, MPI_ANY_SOURCE, DISMIS_TAG, MPI_COMM_WORLD, &mpiRequest );
	MPI_Test( &mpiRequest, &test, &mpiStatus );
	if ( test == 1 ) {
		scalarClock = max( scalarClock, req.clock ) + 1;

		int channel = req.channelNO;

		channels[channel].direction = req.direction;
		channels[channel].numberOfShips = req.numberOfShips;

		demand[channel].erase( demand[channel].begin() );
		clearStringStream( ss );
		ss << mpiStatus.MPI_SOURCE;
		string log = "Prośba od " + ss.str() + " o zwolnienie kanału ";
		clearStringStream( ss );
		ss << req.channelNO;
		log += ss.str();
		createLog( log, ss, rank, scalarClock );
	}
}

/*
 * Sprawdzanie wiadomosci typu answer
 */
void checkAnswer() {
	int ms = 50;
	usleep( ms * 1000 );
	Msg req;
	int test;
	MPI_Irecv( &req, 1, MPI_MSG_TYPE, MPI_ANY_SOURCE, ANSWER_TAG, MPI_COMM_WORLD, &mpiRequest );
	MPI_Test( &mpiRequest, &test, &mpiStatus );
	if ( test == 1 ) {
		scalarClock = max( scalarClock, req.clock ) + 1;

		clearStringStream( ss );
		ss << mpiStatus.MPI_SOURCE;
		string log = "Dostanie informacji od procesu " + ss.str() + " na temat kanału ";
		clearStringStream( ss );
		ss << req.channelNO;
		log += ss.str();
		createLog( log, ss, rank, scalarClock );

		stamps[mpiStatus.MPI_SOURCE] = req.clock;
		isAnswer[mpiStatus.MPI_SOURCE] = true;
	}
}
/*
 * przesylanie do innych procesow mojego stanu
 */
void broadcastRequest( Request aRequest ) {
	int tag = REQUEST_TAG;

	clearStringStream( ss );
	ss << aRequest.channelNO;
	string log = "Wysyłanie żądania o dostęp do kanału " + ss.str();
	createLog( log, ss, rank, scalarClock );

	Msg msg;
	msg.channelNO = aRequest.channelNO;
	msg.clock = aRequest.clock;							//aRequest->clock;
	msg.direction = 0;
	msg.numberOfShips = 0;
	for ( int i = 0; i < size; i++ ) {
		if ( i != rank ) {
			MPI_Send( &msg, 1, MPI_MSG_TYPE, i, tag, MPI_COMM_WORLD );
		}
	}
}

/*
 * Funkcja ustawiająca wszelkie zmienne
 */
void configure() {

	string log;

	scalarClock = 0;

	choosenChannel = 0;
	actualThinkingChanel = 0;
	direction = 1; 												// 0 w strone walki, 1 do bazy
	isGoingToChannel = true;

	isAnswer = new bool[size];
	stamps = new int[size];

	for ( int i = 0; i < NUMBERS_OF_CHANNELS; i++ ) {
		listOfPreference[i] = i;
	}

	srand( rank + 1 );
	for ( int i = 0; i < NUMBERS_OF_CHANNELS; i++ ) {
		swap( listOfPreference[i], listOfPreference[rand() % NUMBERS_OF_CHANNELS] );
		channels[i].maxNumberOfShips = (10 * (i + 1)) % 30 + 5;
		channels[i].direction = 0;
		channels[i].numberOfShips = 0;
	}

	log = "Lista preferencji: ";
	for ( int i = 0; i < NUMBERS_OF_CHANNELS; i++ ) {
		clearStringStream( ss );
		ss << listOfPreference[i];
		log = log + ss.str() + " - ";
	}
	createLog( log, ss, rank, scalarClock );

	mode = SEARCHING_CANAL;

	msg_offsets[0] = offsetof(Msg, channelNO);
	msg_offsets[1] = offsetof(Msg, clock);
	msg_offsets[2] = offsetof(Msg, numberOfShips);;
	msg_offsets[3] = offsetof(Msg, direction);

	MPI_Type_create_struct( msg_items, msg_block_lengths, msg_offsets, msg_types, &MPI_MSG_TYPE );
	MPI_Type_commit( &MPI_MSG_TYPE );

}

void goToCriticalSection() {
	Request r;
	r.channelNO = choosenChannel;
	r.clock = scalarClock;
	r.pid = rank;
	addDemand( r );

	broadcastRequest( r );

	for ( int i = 0; i < size; i++ ) {
		if ( i == rank ) {
			stamps[i] = scalarClock;
			isAnswer[i] = true;
		} else {
			stamps[i] = 0;
			isAnswer[i] = false;
		}
	}

	mode = WAITING_FOR_ANSWERS;
}

int main( int argc, char **argv ) {
	MPI_Init( &argc, &argv );
	MPI_Comm_size( MPI_COMM_WORLD, &size );
	MPI_Comm_rank( MPI_COMM_WORLD, &rank );

	configure();

	while ( true ) {

		if ( mode == SEARCHING_CANAL ) {
			scalarClock++;
			if ( demand[listOfPreference[actualThinkingChanel]].size() < MAX_DEMAND_SIZE ) {
				choosenChannel = listOfPreference[actualThinkingChanel];
				goToCriticalSection();
			}
			actualThinkingChanel++;
			if ( actualThinkingChanel == NUMBERS_OF_CHANNELS ) {
				actualThinkingChanel = 0;
			}
		}

		else if ( mode == WAITING_FOR_ANSWERS ) {
			string log;
			clearStringStream( ss );
			ss << choosenChannel;
			log = "Sprawdzam odpowiedzi dotyczące kanału " + ss.str();
			createLog( log, ss, rank, scalarClock );

			scalarClock++;
			bool isAllAnswer = true;
			for ( int i = 0; i < size; i++ ) {
				if ( i == rank ) {
					continue;
				}

				if ( isAnswer[i] == false || stamps[i] <= stamps[rank] ) {
					isAllAnswer = false;
					break;
				}
			}
			if ( isAllAnswer ) {
				clearStringStream( ss );
				ss << choosenChannel;
				log = "Dostałem wszystkie odpowiedzi dotyczące kanału " + ss.str();
				createLog( log, ss, rank, scalarClock );
				mode = WAITING_TO_BE_FIRST;
			}
		}

		else if ( mode == WAITING_TO_BE_FIRST ) {
			string log;
			clearStringStream( ss );
			log = "Sprawdzam, czy jestem pierwszy w kolejce do kanału " + ss.str();
			createLog( log, ss, rank, scalarClock );
			scalarClock++;
			Request r = demand[choosenChannel].at( 0 );
			if ( r.pid == rank ) {
				clearStringStream( ss );
				log = "Jestem pierwszy w kolejce do kanału " + ss.str();
				createLog( log, ss, rank, scalarClock );
				mode = IN_CRITICAL_SECTION;
			}
		}

		else if ( mode == IN_CRITICAL_SECTION ) {
			scalarClock++;
			bool isSucses = false;
			if ( !isGoingToChannel ) {													//wersja jak opusczamy kanał
				channels[choosenChannel].numberOfShips--;
				isSucses = true;
			} else if ( ((channels[choosenChannel].direction == direction) 				//wersja jak idziemy do kanału
			&& (channels[choosenChannel].numberOfShips < channels[choosenChannel].maxNumberOfShips))
					|| (channels[choosenChannel].numberOfShips == 0) ) {
				channels[choosenChannel].direction = direction;
				channels[choosenChannel].numberOfShips++;
				isSucses = true;
			}
			printChannelStatus( choosenChannel, ss, channels[choosenChannel], rank, scalarClock );

			Msg dismis;
			dismis.channelNO = choosenChannel;
			dismis.direction = channels[choosenChannel].direction;
			dismis.numberOfShips = channels[choosenChannel].numberOfShips;
			dismis.clock = scalarClock;

			for ( int i = 0; i < size; i++ ) {
				if ( i != rank ) {
					MPI_Send( &dismis, 1, MPI_MSG_TYPE, i, DISMIS_TAG, MPI_COMM_WORLD );
				}
			}

			demand[choosenChannel].erase( demand[choosenChannel].begin() );

			if ( isSucses ) {
				if ( isGoingToChannel ) {
					string log;
					clearStringStream( ss );
					ss << choosenChannel;
					log = "Wchodzę do kanału " + ss.str();
					createLog( log, ss, rank, scalarClock );
					isGoingToChannel = false;
					goToCriticalSection();
				} else {
					string log;
					clearStringStream( ss );
					ss << choosenChannel;
					log = "Wyszedłem z kanału " + ss.str();
					createLog( log, ss, rank, scalarClock );
					mode = SEARCHING_CANAL;
					direction = 1 - direction;
					isGoingToChannel = true;
				}
			} else {
				if ( isGoingToChannel ) {
					string log;
					clearStringStream( ss );
					ss << choosenChannel;
					log = "Nie udało mi się wejść do kanału " + ss.str();
					createLog( log, ss, rank, scalarClock );
					mode = SEARCHING_CANAL;
				} else {
					string log;
					clearStringStream( ss );
					ss << choosenChannel;
					log = "Nie udało się wyjść z kanału " + ss.str();
					createLog( log, ss, rank, scalarClock );
				}
			}
			printChannelStatus( choosenChannel, ss, channels[choosenChannel], rank, scalarClock );
		}

		createLog( "Zaczynam czytać wiadomosci", ss, rank, scalarClock );
		int i = 0;
		while ( true ) {
			i++;
			createLog( "Sprawdzam wiadomośc", ss, rank, scalarClock );
			int ms = 50;
			usleep( ms * 1000 );
			MPI::Status stat;
			MPI::COMM_WORLD.Iprobe( MPI::ANY_SOURCE, MPI::ANY_TAG, stat );
			int tag = stat.Get_tag();
			if ( tag == ANSWER_TAG ) {
				checkAnswer();
			} else if ( tag == REQUEST_TAG ) {
				checkRequest();
			} else if ( tag == DISMIS_TAG ) {
				checkDismis();
			} else {
				clearStringStream( ss );
				ss << i;
				string log = "Nie ma już dla mnie wiadomosci, przeczytałem: " + ss.str();
				createLog( log, ss, rank, scalarClock );
				break;
			}
		}
	}

	MPI_Type_free( &MPI_MSG_TYPE );
	MPI_Finalize();
	return 0;
}

