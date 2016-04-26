// Utilisation Model.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <queue>
#include <random>
#include <vector>
#include <iostream>
#include <iomanip>

class ExperimentResult
{
	public:
		float Sev1SLAPercentage;
		float Sev2SLAPercentage;
		float Sev3SLAPercentage;
		float Utilisation;
		int StillInProgress;
};

class Ticket
{
	public:
		Ticket();
		bool CompletedWithinSLA();
		int HoursRequired;
		int HoursCompleted;
		int HourArrived;
		int HourCompleted;
		int Severity;
		int ID;

	private:
		static int nextTicketID;
};

int Ticket::nextTicketID = 0;

Ticket::Ticket()
{
	std::random_device RandomNumberGenerator;
	std::uniform_int_distribution<int> HoursRequiredDistribution(1,16);	// 1 - 16 hours
	HoursRequired = HoursRequiredDistribution(RandomNumberGenerator);
	HoursCompleted = 0;
	HourArrived = 0;
	HourCompleted = 0;
	
	std::uniform_int_distribution<int> SeverityDistribution(1,20);
	int tempSev = SeverityDistribution(RandomNumberGenerator);
	if ( tempSev >= 19 )
	{
		Severity = 1;	// 5% sev 1
	}
	else if ( tempSev >= 15 )
	{
		Severity = 2; // 25% sev 2
	}
	else
	{
		Severity = 3; // 70% sev 3
	}

	ID = nextTicketID;
	nextTicketID++;
}

bool Ticket::CompletedWithinSLA()
{

	int SLAHours = 0;

	switch ( Severity )
	{
		case 1:
			SLAHours = 16;
			break;
		case 2:
			SLAHours = 32;
			break;
		case 3:
			SLAHours = 64;
			break;
	}

	bool bRetVal = false;
	if ( ( HourCompleted - HourArrived ) <= SLAHours )
	{
		bRetVal = true;
	}

	return bRetVal;
}

class MultiTasker
{
	public:
		MultiTasker();
		void DoWork( int currentHour, bool (*GreaterThanFunction)(const Ticket&, const Ticket&) );
		int HoursBooked;
		int HoursWorked;
		std::vector<Ticket> ToDoList;
		std::vector<Ticket> * DoneList;
};


MultiTasker::MultiTasker()
{
	HoursBooked = 0;
	HoursWorked = 0;
}

void MultiTasker::DoWork( int currentHour, bool (*GreaterThanFunction)(const Ticket&, const Ticket&) )
{
	// Sort To Do list into priority order
	int currentTicketID = 0;
	if ( ToDoList.size() > 0 )
	{
		currentTicketID = ToDoList[0].ID;
		std::stable_sort(ToDoList.begin(), ToDoList.end(), GreaterThanFunction);
		if ( currentTicketID != ToDoList[0].ID )
		{
			// Have had to interupt work, add an hour on to required time for dropped ticket
			for ( int i = 0; i < ToDoList.size(); i++ )
			{
				if ( ToDoList[i].ID == currentTicketID )
				{
					ToDoList[i].HoursRequired += 4;
				}
			}
		}
	}
	
	// Do an hours work on the highest priority item
	if ( ToDoList.size() > 0 )
	{
		ToDoList[0].HoursCompleted++;
		HoursWorked++;

		// If it is completed, remove it and put it in the done pile!
		if ( ToDoList[0].HoursCompleted == ToDoList[0].HoursRequired )
		{
			ToDoList[0].HourCompleted = currentHour;
			DoneList->push_back(ToDoList[0]);
			ToDoList.erase(ToDoList.begin());
			
		}
	}

	HoursBooked++;

}

bool GreaterBySeverityAndAge(const Ticket& a, const Ticket& b)
{
	// Prioritise by severity first (1 beats 2) and then by age (oldest first)
	if(a.Severity < b.Severity)
	{
		return true;
	}
	else if ( a.Severity == b.Severity )
	{
		if ( a.HourArrived < b.HourArrived )
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}	
}

bool GreaterBySeverityAndWorkload(const MultiTasker& a, const MultiTasker& b)
{
	/* Prioritise resource as follows:
		- Anyone who has nothing on first, then
		- Anyone working on a lower priority faults, then
		- Anyone with the fewest faults  */

	if( (a.ToDoList.size() == 0) && (b.ToDoList.size() == 0) )
	{
		// Both have nothing on so equal
		return false;
	}
	else
	{
		// Check they both are working on at least one fault
		if( (a.ToDoList.size() > 0) && (b.ToDoList.size() > 0) )
		{
			if ( a.ToDoList[0].Severity > b.ToDoList[0].Severity )
			{
				// a is working on a lower priority fault
				return true;
			}
			else
			{
				if ( a.ToDoList[0].Severity == b.ToDoList[0].Severity )
				{
					if ( a.ToDoList.size() < b.ToDoList.size() )
					{
						// a has less on
						return true;
					}
					else
					{
						// a has the same or more on
						return false;
					}
				}
				else
				{
					return false;
				}
			}
		}
		else
		{
			// One isn't working on anything
			if ( a.ToDoList.size() == 0 )
			{
				// a has nothing on
				return true;
			}
			else
			{
				return false;
			}
		}

	}	
}

ExperimentResult StandardRoundRobin(int teamSize)
{
	std::vector<Ticket> doneList;
	
	// Setup a team of great multi-taskers!
	std::vector<MultiTasker> team;
	for (int i = 0; i < teamSize; i++)
	{
		MultiTasker mt;
		mt.DoneList = &doneList;
		team.push_back(mt);
	}
	
	// Run experiemnet for 700, 8 hour days
	int runHours = 8 * 700;
	int iCurrentTeamMember = 0;
	for ( int iCurrentHour = 0; iCurrentHour < runHours; iCurrentHour++ )
	{
		// 30% chance that a ticket arrives
		std::random_device RandomNumberGenerator;
		std::uniform_int_distribution<int> Distribution(1,3);
		int rnd = Distribution(RandomNumberGenerator);
		if ( rnd == 3 )
		{
			// Create ticket
			Ticket ticket;
			ticket.HourArrived = iCurrentHour;
			
			// Assign using round-robin
			team[iCurrentTeamMember].ToDoList.push_back(ticket);
			iCurrentTeamMember++;
			if ( iCurrentTeamMember == team.size() )
			{
				iCurrentTeamMember = 0;
			}
		}

		// Now get all the team to do an hours work
		for ( int i = 0; i < team.size(); i++ )
		{
			team[i].DoWork(iCurrentHour, GreaterBySeverityAndAge);
		}
	}

	// Now work out percentage within SLA 
	int sevCount[3] = {0, 0, 0};
	int sevWithinSLA[3] = {0, 0, 0};
	
	for each ( Ticket ticket in doneList )
	{
		sevCount[ticket.Severity - 1]++;
		if ( ticket.CompletedWithinSLA() )
		{
			sevWithinSLA[ticket.Severity - 1]++;
		}
	}

	ExperimentResult er;
	
	er.Sev1SLAPercentage = ( float(sevWithinSLA[0]) / float(sevCount[0]) ) * 100;
	er.Sev2SLAPercentage = ( float(sevWithinSLA[1]) / float(sevCount[1]) ) * 100;
	er.Sev3SLAPercentage = ( float(sevWithinSLA[2]) / float(sevCount[2]) ) * 100;
	
	// Now work out percentage utilisation and tickets still in progress
	int totalBookedHours = 0;
	int totalWorkedHOurs = 0;
	er.StillInProgress = 0;
	for each ( MultiTasker mt in team )
	{
		totalBookedHours += mt.HoursBooked;
		totalWorkedHOurs += mt.HoursWorked;
		er.StillInProgress += mt.ToDoList.size();
	}

	er.Utilisation = ( float(totalWorkedHOurs) / float(totalBookedHours) ) * 100;

	return er;

}

ExperimentResult PushBasedOnCurrentWorkload(int teamSize)
{
	std::vector<Ticket> doneList;
	
	// Setup a team of great multi-taskers!
	std::vector<MultiTasker> team;
	for (int i = 0; i < teamSize; i++)
	{
		MultiTasker mt;
		mt.DoneList = &doneList;
		team.push_back(mt);
	}
	
	// Run experiemnet for 700, 8 hour days
	int runHours = 8 * 700;
	for ( int iCurrentHour = 0; iCurrentHour < runHours; iCurrentHour++ )
	{
		// 30% chance that a ticket arrives
		std::random_device RandomNumberGenerator;
		std::uniform_int_distribution<int> Distribution(1,3);
		int rnd = Distribution(RandomNumberGenerator);
		if ( rnd == 3 )
		{
			// Create ticket
			Ticket ticket;
			ticket.HourArrived = iCurrentHour;
			
			// Sort the team so that the best option is always in position 0
			std::stable_sort(team.begin(), team.end(), GreaterBySeverityAndWorkload);

			team[0].ToDoList.push_back(ticket);
		}

		// Now get all the team to do an hours work
		for ( int i = 0; i < team.size(); i++ )
		{
			team[i].DoWork(iCurrentHour, GreaterBySeverityAndAge);
		}
	}

	// Now work out percentage within SLA 
	int sevCount[3] = {0, 0, 0};
	int sevWithinSLA[3] = {0, 0, 0};
	
	for each ( Ticket ticket in doneList )
	{
		sevCount[ticket.Severity - 1]++;
		if ( ticket.CompletedWithinSLA() )
		{
			sevWithinSLA[ticket.Severity - 1]++;
		}
	}

	ExperimentResult er;
	
	er.Sev1SLAPercentage = ( float(sevWithinSLA[0]) / float(sevCount[0]) ) * 100;
	er.Sev2SLAPercentage = ( float(sevWithinSLA[1]) / float(sevCount[1]) ) * 100;
	er.Sev3SLAPercentage = ( float(sevWithinSLA[2]) / float(sevCount[2]) ) * 100;
	
	// Now work out percentage utilisation and tickets still in progress
	int totalBookedHours = 0;
	int totalWorkedHOurs = 0;
	er.StillInProgress = 0;
	for each ( MultiTasker mt in team )
	{
		totalBookedHours += mt.HoursBooked;
		totalWorkedHOurs += mt.HoursWorked;
		er.StillInProgress += mt.ToDoList.size();
	}

	er.Utilisation = ( float(totalWorkedHOurs) / float(totalBookedHours) ) * 100;

	return er;

}

ExperimentResult PurePull(int teamSize)
{
	std::vector<Ticket> ticketQueue;
	std::vector<Ticket> doneList;
	
	// Setup a team of great multi-taskers!
	std::vector<MultiTasker> team;
	for (int i = 0; i < teamSize; i++)
	{
		MultiTasker mt;
		mt.DoneList = &doneList;
		team.push_back(mt);
	}
	
	// Run experiemnet for 700, 8 hour days
	int runHours = 8 * 700;
	int iCurrentTeamMember = 0;
	for ( int iCurrentHour = 0; iCurrentHour < runHours; iCurrentHour++ )
	{
		// 30% chance that a ticket arrives
		std::random_device RandomNumberGenerator;
		std::uniform_int_distribution<int> Distribution(1,3);
		int rnd = Distribution(RandomNumberGenerator);
		if ( rnd == 3 )
		{
			// Create ticket
			Ticket ticket;
			ticket.HourArrived = iCurrentHour;
			
			// Add to our ticket queue
			ticketQueue.push_back(ticket);

			// Sort our ticket queue, highest priority then oldest first
			std::stable_sort(ticketQueue.begin(), ticketQueue.end(), GreaterBySeverityAndAge);

		}

		// Assign tickets to any team members that are available to pull work
		for ( int i = 0; i < team.size(); i++ )
		{
			if ( team[i].ToDoList.size() == 0 )
			{
				if ( ticketQueue.size() != 0 )
				{
					team[i].ToDoList.push_back(ticketQueue[0]);
					ticketQueue.erase(ticketQueue.begin());
				}
			}
		}

		// Now get all the team to do an hours work
		for ( int i = 0; i < team.size(); i++ )
		{
			team[i].DoWork(iCurrentHour, GreaterBySeverityAndAge);
		}
	}

	// Now work out percentage within SLA 
	int sevCount[3] = {0, 0, 0};
	int sevWithinSLA[3] = {0, 0, 0};
	
	for each ( Ticket ticket in doneList )
	{
		sevCount[ticket.Severity - 1]++;
		if ( ticket.CompletedWithinSLA() )
		{
			sevWithinSLA[ticket.Severity - 1]++;
		}
	}

	ExperimentResult er;
	
	er.Sev1SLAPercentage = ( float(sevWithinSLA[0]) / float(sevCount[0]) ) * 100;
	er.Sev2SLAPercentage = ( float(sevWithinSLA[1]) / float(sevCount[1]) ) * 100;
	er.Sev3SLAPercentage = ( float(sevWithinSLA[2]) / float(sevCount[2]) ) * 100;
	
	// Now work out percentage utilisation and tickets still in progress
	int totalBookedHours = 0;
	int totalWorkedHOurs = 0;
	er.StillInProgress = 0;
	for each ( MultiTasker mt in team )
	{
		totalBookedHours += mt.HoursBooked;
		totalWorkedHOurs += mt.HoursWorked;
		er.StillInProgress += mt.ToDoList.size();
	}

	er.StillInProgress += ticketQueue.size();

	er.Utilisation = ( float(totalWorkedHOurs) / float(totalBookedHours) ) * 100;

	return er;

}

ExperimentResult Random(int teamSize)
{
	std::vector<Ticket> doneList;
	
	// Setup a team of great multi-taskers!
	std::vector<MultiTasker> team;
	for (int i = 0; i < teamSize; i++)
	{
		MultiTasker mt;
		mt.DoneList = &doneList;
		team.push_back(mt);
	}
	
	// Run experiemnet for 700, 8 hour days
	int runHours = 8 * 700;
	for ( int iCurrentHour = 0; iCurrentHour < runHours; iCurrentHour++ )
	{
		// 30% chance that a ticket arrives
		std::random_device RandomNumberGenerator;
		std::uniform_int_distribution<int> Distribution(1,3);
		int rnd = Distribution(RandomNumberGenerator);
		if ( rnd == 3 )
		{
			// Create ticket
			Ticket ticket;
			ticket.HourArrived = iCurrentHour;
			
			// Assign randomly
			std::uniform_int_distribution<int> TeamDistribution(0, team.size() - 1);
			int teamMember = TeamDistribution(RandomNumberGenerator);
			
			team[teamMember].ToDoList.push_back(ticket);
		}

		// Now get all the team to do an hours work
		for ( int i = 0; i < team.size(); i++ )
		{
			team[i].DoWork(iCurrentHour, GreaterBySeverityAndAge);
		}
	}

	// Now work out percentage within SLA 
	int sevCount[3] = {0, 0, 0};
	int sevWithinSLA[3] = {0, 0, 0};
	
	for each ( Ticket ticket in doneList )
	{
		sevCount[ticket.Severity - 1]++;
		if ( ticket.CompletedWithinSLA() )
		{
			sevWithinSLA[ticket.Severity - 1]++;
		}
	}

	ExperimentResult er;
	
	er.Sev1SLAPercentage = ( float(sevWithinSLA[0]) / float(sevCount[0]) ) * 100;
	er.Sev2SLAPercentage = ( float(sevWithinSLA[1]) / float(sevCount[1]) ) * 100;
	er.Sev3SLAPercentage = ( float(sevWithinSLA[2]) / float(sevCount[2]) ) * 100;
	
	// Now work out percentage utilisation and tickets still in progress
	int totalBookedHours = 0;
	int totalWorkedHOurs = 0;
	er.StillInProgress = 0;
	for each ( MultiTasker mt in team )
	{
		totalBookedHours += mt.HoursBooked;
		totalWorkedHOurs += mt.HoursWorked;
		er.StillInProgress += mt.ToDoList.size();
	}

	er.Utilisation = ( float(totalWorkedHOurs) / float(totalBookedHours) ) * 100;

	return er;

}

ExperimentResult PullWithExpedite(int teamSize)
{
	std::vector<Ticket> ticketQueue;
	std::vector<Ticket> doneList;
	
	// Setup a team of great multi-taskers!
	std::vector<MultiTasker> team;
	for (int i = 0; i < teamSize; i++)
	{
		MultiTasker mt;
		mt.DoneList = &doneList;
		team.push_back(mt);
	}
	
	// Run experiemnet for 700, 8 hour days
	int runHours = 8 * 700;
	int iCurrentTeamMember = 0;
	for ( int iCurrentHour = 0; iCurrentHour < runHours; iCurrentHour++ )
	{
		// 30% chance that a ticket arrives
		std::random_device RandomNumberGenerator;
		std::uniform_int_distribution<int> Distribution(1,3);
		int rnd = Distribution(RandomNumberGenerator);
		if ( rnd == 3 )
		{
			// Create ticket
			Ticket ticket;
			ticket.HourArrived = iCurrentHour;
			
			// Add to our ticket queue
			ticketQueue.push_back(ticket);

			// Sort our ticket queue, highest priority then oldest first
			std::stable_sort(ticketQueue.begin(), ticketQueue.end(), GreaterBySeverityAndAge);

		}

		// Push any sev 1 tickets
		for ( int i = 0; i < ticketQueue.size(); i++ )
		{
			if ( ticketQueue[i].Severity == 1 )
			{
				// Sort the team so that the best option is always in position 0
				std::stable_sort(team.begin(), team.end(), GreaterBySeverityAndWorkload);
				team[0].ToDoList.push_back(ticketQueue[0]);
				ticketQueue.erase(ticketQueue.begin());
			}
		}

		// Assign tickets to any team members that are available to pull work
		for ( int i = 0; i < team.size(); i++ )
		{
			if ( team[i].ToDoList.size() == 0 )
			{
				if ( ticketQueue.size() != 0 )
				{
					team[i].ToDoList.push_back(ticketQueue[0]);
					ticketQueue.erase(ticketQueue.begin());
				}
			}
		}

		// Now get all the team to do an hours work
		for ( int i = 0; i < team.size(); i++ )
		{
			team[i].DoWork(iCurrentHour, GreaterBySeverityAndAge);
		}
	}

	// Now work out percentage within SLA 
	int sevCount[3] = {0, 0, 0};
	int sevWithinSLA[3] = {0, 0, 0};
	
	for each ( Ticket ticket in doneList )
	{
		sevCount[ticket.Severity - 1]++;
		if ( ticket.CompletedWithinSLA() )
		{
			sevWithinSLA[ticket.Severity - 1]++;
		}
	}

	ExperimentResult er;
	
	er.Sev1SLAPercentage = ( float(sevWithinSLA[0]) / float(sevCount[0]) ) * 100;
	er.Sev2SLAPercentage = ( float(sevWithinSLA[1]) / float(sevCount[1]) ) * 100;
	er.Sev3SLAPercentage = ( float(sevWithinSLA[2]) / float(sevCount[2]) ) * 100;
	
	// Now work out percentage utilisation and tickets still in progress
	int totalBookedHours = 0;
	int totalWorkedHOurs = 0;
	er.StillInProgress = 0;
	for each ( MultiTasker mt in team )
	{
		totalBookedHours += mt.HoursBooked;
		totalWorkedHOurs += mt.HoursWorked;
		er.StillInProgress += mt.ToDoList.size();
	}

	er.StillInProgress += ticketQueue.size();

	er.Utilisation = ( float(totalWorkedHOurs) / float(totalBookedHours) ) * 100;

	return er;

}

int _tmain(int argc, _TCHAR* argv[])
{

	ExperimentResult overallResult;
	overallResult.Sev1SLAPercentage = 0;
	overallResult.Sev2SLAPercentage = 0;
	overallResult.Sev3SLAPercentage = 0;
	overallResult.StillInProgress = 0;
	overallResult.Utilisation = 0;

	int timesToRepeatExperiment = 1000;

	for ( int i = 1; i <= 8; i++ )
	{
		ExperimentResult overallResult;
		overallResult.Sev1SLAPercentage = 0;
		overallResult.Sev2SLAPercentage = 0;
		overallResult.Sev3SLAPercentage = 0;
		overallResult.StillInProgress = 0;
		overallResult.Utilisation = 0;
		
		for ( int j = 0; j < timesToRepeatExperiment; j++ )
		{	
			ExperimentResult er = Random(i);
			overallResult.Sev1SLAPercentage += er.Sev1SLAPercentage;
			overallResult.Sev2SLAPercentage += er.Sev2SLAPercentage;
			overallResult.Sev3SLAPercentage += er.Sev3SLAPercentage;
			overallResult.Utilisation += er.Utilisation;
			overallResult.StillInProgress += er.StillInProgress;
		}

		overallResult.Sev1SLAPercentage /= timesToRepeatExperiment;
		overallResult.Sev2SLAPercentage /= timesToRepeatExperiment;
		overallResult.Sev3SLAPercentage /= timesToRepeatExperiment;
		overallResult.Utilisation /= timesToRepeatExperiment;
		overallResult.StillInProgress /= timesToRepeatExperiment;
		
		std::cout << "Team size: " << i << "\n";
		std::cout << "Utilisation: " << std::setprecision(4) << overallResult.Utilisation << "%\n";
		std::cout << "Sev 1 SLA Performance " << overallResult.Sev1SLAPercentage << "%\n";
		std::cout << "Sev 2 SLA Performance " << overallResult.Sev2SLAPercentage << "%\n";
		std::cout << "Sev 3 SLA Performance " << overallResult.Sev3SLAPercentage << "%\n";
		std::cout << "Tickets still in progress " << overallResult.StillInProgress << "\n";

	}

	int i;
	std::cin >> i; 

	return 0;
}