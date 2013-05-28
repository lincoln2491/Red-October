/*
 * structs.h
 *
 *	Plik zawiera struktury oraz stałe używane w programie
 *  Created on: 02-05-2013
 *      Author: ksurdyk
 */

#ifndef STRUCTS_H_
#define STRUCTS_H_

/*
 * Zmienne używane w projekcie
 */
const int numberOfChanels = 20; 								//liczba kanałów
const int maxDemandSize = 5; 									//maksymalna liczba procesów w kolejce żądań

/*
 * Tagi używane w komunikacji
 */

const int requestTag = numberOfChanels + 1; 					//wysyłanie
const int dismisTag = numberOfChanels + 2;
const int answerTag = numberOfChanels + 3;

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
	int pidSend;												// id procesu wysyłającego
	int pidRecv;												// id procesu wysyłającego
	int clock;													//zegar skalarny
	int channelNO;												//numer kanału który zwalniamy
	int numberOfShips; 											// ile jest aktualnie w kanale
	int direction;												// jaki kierunke (1 - do walki, 0 - spowrotem)
};

#endif /* STRUCTS_H_ */
