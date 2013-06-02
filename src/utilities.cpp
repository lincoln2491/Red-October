/*
 * utilities.cpp
 *
 *  Created on: 30-05-2013
 *      Author: ksurdyk
 */
#include "utilities.h"

using namespace std;

void clearStringStream( ostringstream &aSS ) {
	aSS.str( "" );
	aSS.clear();
}

void createLog( string aLog, ostringstream &aSS, int aRank, int aScalarClock ) {
	clearStringStream( aSS );
	aSS << aRank;
	string log = "PID: " + aSS.str();
	clearStringStream( aSS );
	aSS << aScalarClock;
	log += " time: " + aSS.str() + " msg: " + aLog + "\n";
	cout << log;
}

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

/*
 *	Funkcja pozorująca płynięcie kanałem
 */
void swiming( int aChannelNumber, ostringstream &aSS, int &aScalarClock, int aRank ) {
	int time = rand() % 1000;
	clearStringStream( aSS );
	aSS << aChannelNumber;
	string log = "Początek płynięcia kanałem  " + aSS.str() + ". Będzie to trwało ";
	clearStringStream( aSS );
	aSS << time;
	log += aSS.str();
	aScalarClock++;
	createLog( log, aSS, aRank, aScalarClock );

	usleep( time * 1000 );

	clearStringStream( aSS );
	aSS << aChannelNumber;
	log = "Koniec płynięcia kanałem " + aSS.str() + ". Trwało to ";
	clearStringStream( aSS );
	aSS << time;
	log += aSS.str();
	aScalarClock++;
	createLog( log, aSS, aRank, aScalarClock );
}

/*
 * Funkcja pozorująca walkę
 */
void fighting( ostringstream &aSS, int &aScalarClock, int aRank ) {
	int time = rand() % 1000;
	clearStringStream( aSS );
	aSS << time;
	string log = "Początek walki która będzie trwala " + aSS.str();
	aScalarClock++;
	createLog( log, aSS, aRank, aScalarClock );

	usleep( time * 1000 );

	clearStringStream( aSS );
	aSS << time;
	log = "Koniec walki trwającej " + aSS.str();
	aScalarClock++;
	createLog( log, aSS, aRank, aScalarClock );
}

/*
 *  Funkcja opuszczenia kanału
 */
void leafChannel( int aChannelNumber, ostringstream &aSS, int &aScalarClock, int aRank ) {
	clearStringStream( aSS );
	aSS << aChannelNumber;
	string log = "Opuszczam kanał " + aSS.str();
	aScalarClock++;
	createLog( log, aSS, aRank, aScalarClock );
}

/*
 * Funkcja wejscia do kanału, jak już został wybrany
 */
void goToChannel( int id, int direction, ostringstream &aSS, int &aScalarClock, int aRank ) {
	clearStringStream( aSS );
	aSS << id;
	string log = "Wpływam do kanału " + aSS.str();
	clearStringStream( aSS );
	aSS << direction;
	log += " w kierunku " + aSS.str();
	aScalarClock++;
	createLog( log, aSS, aRank, aScalarClock );
}

