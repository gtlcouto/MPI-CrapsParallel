/**	@file: CrapsParallel.cpp
	@author Gustavo Couto
	@author gcouto@fanshaweonline.ca
	@date 2013-12-03
	@version 0.0.4
	@note Developed for C# MPI
	@brief Craps game utilizing MPI Architecture.
	*/



#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <map>
#include <mpi.h>
using namespace std;

long long gamesPlayed = 0;
map<unsigned,unsigned> resultFreq;
const int MSG_SIZE = 1;
const int TAG_DATA = 0, TAG_QUIT = 1;

//Function crapsGame
//Basic crap game functionality.
bool crapsGame()
{
	int mark;
	int mark2 = 0;

	
		while(true)
		{
			int dice1 = (rand() % 6) + 1;
			int dice2 = (rand() % 6) + 1;
			mark = (dice1 + dice2);
			if (mark == 7 || mark == 11) 
			{
				gamesPlayed++;
				return true;
			
			} else if (mark == 2 || mark == 12 || mark == 3) 
			{
				gamesPlayed++;
				return false;
			} else
			{
				while ((mark2 != mark) || (mark2 != 7)) {
				dice1 = (rand() % 6) + 1;
				dice2 = (rand() % 6) + 1;
				mark2 = (dice1 + dice2);
				if (mark == mark2) {
					gamesPlayed++;
					return true;
				} else if (mark2 == 7) {
					gamesPlayed++;
					return false;
					}
				} 
			}

			break;
		}
		
}

//Function playTillStreak
//play the game until the winstreak is reached or another process reaches it first
void playTillStreak(int max_streak)
{
	int winstreak = 0;
	do {		
		if(crapsGame())
		{
			winstreak++;
			++resultFreq[winstreak];
		} else {
			winstreak = 0;
			++resultFreq[winstreak];
		}
	} while ( winstreak < max_streak);


}

//slave process
void processSlave()
{
	// Message passing variables
	int msgBuff[MSG_SIZE];
	MPI_Status status;

	srand(static_cast<unsigned>(time(NULL)));

	// (main loop)
	do
	{
		// Receive a message from the master 
		MPI_Recv( msgBuff, MSG_SIZE, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

		// IF it's a DATA message
		if( status.MPI_TAG == TAG_DATA )
		{
			int max_streak = msgBuff[0];
			playTillStreak(max_streak);
		}
		// ELSE
		else
			//Sends a DONE message to the master 
			// You can try using either a blocking or non-blocking send here to see what 
			// impact it has on performance.
			MPI_Send(&msgBuff, 1, MPI_INT, 0, TAG_QUIT, MPI_COMM_WORLD);

	} while( status.MPI_TAG != TAG_QUIT );
}

//master process
void processMaster(int numProcs, int max_streak)
{
	static MPI_Request request;
	static int recvFlag;
	MPI_Status status;
	int msgBuff[MSG_SIZE];
	msgBuff[0] = max_streak;
	// Display a starting message
	cout << "Simulation of Shooter Winning Streaks in 'Craps'\n";
	cout << "==================================================\n\n";

	//gives the process a random seed
	srand(static_cast<unsigned>(time(NULL)) + ( numProcs + max_streak * 10));

	// Start the timer
	double elapsedTime = MPI_Wtime();


	

	//send data to slaves
	MPI_Isend(msgBuff, MSG_SIZE, MPI_INT, 0, TAG_DATA, MPI_COMM_WORLD, &request);

	playTillStreak(max_streak);

	if( request )
	{
		// Already listening for a message

		// Test to see if message has been received
		MPI_Test( &request, &recvFlag, &status );

		if( recvFlag )
		{
			// Message received
			if( status.MPI_TAG == TAG_DATA )
				// streak found
				cout << "Found streak" << msgBuff << endl;

			// Reset the request handle
			request = 0;
		}	
	}




	// Final report
	elapsedTime = MPI_Wtime() - elapsedTime;
	int totalPlayed = 0;
	float impliedFreq;
	cout.precision(10);
	cout<<fixed;
	cout << "Win-Streak" << "\t\t" << "Frequency" << "\t\t" << "Implied Probability" << "\n" ;
	for( map<unsigned,unsigned>::iterator iter = resultFreq.begin(); iter != resultFreq.end(); ++iter )
	{
		totalPlayed += (*iter).second;
	}
	for( map<unsigned,unsigned>::iterator iter = resultFreq.begin(); iter != resultFreq.end(); ++iter )
	{
		cout << (*iter).first << "\t\t\t" << (*iter).second << "\t\t\t\t";
		impliedFreq = (float)(*iter).second / (float)totalPlayed;
		cout << impliedFreq << "\n";
	}
	cout << "\nTotal winning streaks generated: " << totalPlayed;
	cout << endl << "Completed in " << elapsedTime << " seconds using " << numProcs << " processors." << endl;
}


int main(int argc, char *argv[])
{
	if( MPI_Init(&argc, &argv) == MPI_SUCCESS )
	{
		int numProcs, rank;
		MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
		MPI_Comm_rank(MPI_COMM_WORLD, &rank);

		int max_win_streak = atoi(argv[1]);
		if (numProcs > 0)
		{
			if( rank == 0 )
				processMaster( numProcs, max_win_streak );
			else
				processSlave();
		}

		MPI_Finalize();
	}
}
	
