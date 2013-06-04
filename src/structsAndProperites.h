/*
 * structs.h
 *
 *	Plik zawiera struktury oraz stałe używane w programie
 *  Created on: 02-05-2013
 *      Author: ksurdyk
 */

#ifndef STRUCTS_H_
#define STRUCTS_H_

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

using namespace std;
/*
 * Struktura opisująca kanał
 */
struct Channel {
	int numberOfShips; 											// ile jest aktualnie w kanale
	int direction; 												// jaki kierunke (1 - do walki, 0 - powrót)
	int maxNumberOfShips; 										// ile może być maksymalnie statków w kanałe
};

/*
 * Struktura opisująca żadanie przydziału dostępu do strefy krytycznej (kanału)
 */
struct Request {
	int pid;													//identyfikator procesu który chce uzyskać dostęp
	int clock; 													//zegar skalarny
	int channelNO;												//numer kanału do którego chcemy uzyskać dostęp

};

/*
 * Struktura opisująca wiadomość
 */
struct Msg {
	int channelNO;												//numer kanału który zwalniamy
	int clock;													//zegar skalarny
	int numberOfShips; 											// ile jest aktualnie w kanale
	int direction;												// jaki kierunke (1 - do walki, 0 - spowrotem)
};

/*
 * Stałe używane w projekcie
 */
const int NUMBERS_OF_CHANNELS = 10; 								//liczba kanałów
const int MAX_DEMAND_SIZE = 5; 										//maksymalna liczba procesów w kolejce żądań

/*
 * Tryby pracy
 */
const int SEARCHING_CANAL = 0;										//szukanie kanału
const int WAITING_FOR_ANSWERS = 1;									//czekanie na wszystkie odpowiedzi
const int WAITING_TO_BE_FIRST = 2;									//czekanie na bycie pierwszym w kolejce żądań
const int IN_CRITICAL_SECTION = 3;									//w sekcji krytycznej

/*
 * Tagi używane w komunikacji
 */

const int REQUEST_TAG = NUMBERS_OF_CHANNELS + 1; 					//wysyłanie żądania sekcji
const int DISMIS_TAG = NUMBERS_OF_CHANNELS + 2;						//wysyłanie zwolnienia sekcji
const int ANSWER_TAG = NUMBERS_OF_CHANNELS + 3;						//wysyłanie odpowiedzi

#endif /* STRUCTS_H_ */
