/*
 * utilities.h
 *
 *  Created on: 30-05-2013
 *      Author: ksurdyk
 */

#ifndef UTILITIES_H_
#define UTILITIES_H_
#include <sstream>
#include "structsAndProperites.h"
using namespace std;
void clearStringStream( ostringstream &aSS );
void createLog( string aLog, ostringstream &aSS, int aRank, int aScalarClock );
void printChannelStatus( int aNO, ostringstream &aSS, Channel &aChannel, int aRank, int aScalarClock );
void swiming( int aChannelNumber, ostringstream &aSS, int &aScalarClock, int aRank );
void fighting( ostringstream &aSS, int &aScalarClock, int aRank );
void leafChannel( int aChannelNumber, ostringstream &aSS, int &aScalarClock, int aRank );
void goToChannel( int id, int direction, ostringstream &aSS, int &aScalarClock, int aRank );
#endif /* UTILITIES_H_ */
