/*
 * utilities.cpp
 *
 *  Created on: 30-05-2013
 *      Author: ksurdyk
 */
#include "utilities.h"

using namespace std;
/*
 * Czyszczenie strumienia
 */
void clearStringStream( ostringstream &aSS ) {
	aSS.str( "" );
	aSS.clear();
}

/*
 * Wypisanie logu
 */
void createLog( string aLog, ostringstream &aSS, int aRank, int aScalarClock ) {
	clearStringStream( aSS );
	aSS << aRank;
	string log = "PID: " + aSS.str();
	clearStringStream( aSS );
	aSS << aScalarClock;
	log += " time: " + aSS.str() + " msg: " + aLog + "\n";
	cout << log;
}

/*
 * Wypisanie statusu kanału
 */
void printChannelStatus( int aNO, ostringstream &aSS, Channel &aChannel, int aRank, int aScalarClock ) {
	clearStringStream( aSS );
	aSS << aNO;
	string log = "Kanał numer: " + aSS.str();
	clearStringStream( aSS );
	aSS << aChannel.numberOfShips;
	log += ". Liczba statków: " + aSS.str();
	clearStringStream( aSS );
	aSS << aChannel.maxNumberOfShips;
	log += ". Maksymalnie: " + aSS.str();
	clearStringStream( aSS );
	aSS << aChannel.direction;
	log += ". Kierunek: " + aSS.str();
	createLog( log, aSS, aRank, aScalarClock );
}

