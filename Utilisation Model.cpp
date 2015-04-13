// Utilisation Model.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <queue>
#include <random>
#include <vector>
#include <iostream>
#include <iomanip>

class WorkItem
{
	public:
		WorkItem();
		int HoursRequired;
		int HoursCompleted;
		int HourAddedToQueue;
		int HourTakenFromQueue;
		int HourCompleted;
		bool Expedite;
};


WorkItem::WorkItem()
{
	std::random_device RandomNumberGenerator;
	std::uniform_int_distribution<int> WorkDistribution(1,16);	// 1 - 16 hours
	HoursRequired = WorkDistribution(RandomNumberGenerator);
	HoursCompleted = 0;
	HourAddedToQueue = 0;
	HourTakenFromQueue = 0;
	HourCompleted = 0;
	Expedite = false;
}

class Worker
{
	public:
		Worker();
		int HoursBooked;
		int HoursWorked;
		WorkItem CurrentItem;
		bool NoWork;
};

Worker::Worker()
{
	HoursBooked = 0;
	HoursWorked = 0;
	NoWork = true;
}




void RunExperiment(int frequencyToAddWork)
{
	std::queue<WorkItem> WorkerToDoList;
	std::vector<WorkItem> CompletedItems;
	
	Worker worker;
	
	int experimentRunDays = 700;
	int experimentRunHours = experimentRunDays * 8;

	int workAddCount = 0;
	
	for (int i = 0; i < experimentRunHours; i++)
	{
		// Check if we should add work
		if ( workAddCount == frequencyToAddWork )
		{
			workAddCount = 0;
		}
		if ( workAddCount == 0 )
		{
			WorkItem wi;
			wi.HourAddedToQueue = i;
			WorkerToDoList.push(wi);
		}
		workAddCount++;

		// If this is the first hour worker must always take from the queue
		if ( i == 0 )
		{
			worker.CurrentItem = WorkerToDoList.front();
			WorkerToDoList.pop();
			worker.CurrentItem.HourTakenFromQueue = i;
			worker.NoWork = false;
		}
		else
		{
			// Check if current item complete
			if ( worker.CurrentItem.HoursCompleted == worker.CurrentItem.HoursRequired )
			{
				worker.CurrentItem.HourCompleted = i;
				CompletedItems.push_back(worker.CurrentItem);
				worker.NoWork = true;
			}

			if ( worker.NoWork )
			{
				if ( WorkerToDoList.size() > 0 )
				{
					worker.CurrentItem = WorkerToDoList.front();
					WorkerToDoList.pop();
					worker.CurrentItem.HourTakenFromQueue = i;
					worker.NoWork = false;
				}
			}
		}

		// Update counts
		worker.CurrentItem.HoursCompleted++;
		worker.HoursBooked++;
		if ( !worker.NoWork )
		{
			worker.HoursWorked++;
		}
	}

	std::cout << "Work item added every " << frequencyToAddWork << " hour(s)";
	std::cout << "Worker utilisation: " << std::setprecision(4) << ( float(worker.HoursWorked) / float(worker.HoursBooked) ) * 100 << " percent\n";
	//std::cout << std::setprecision(4) << ( float(worker.HoursWorked) / float(worker.HoursBooked) ) * 100 << ",";

	// Calculate average cycle time, lead time and queue time
	long totalCycleTime = 0;
	long totalLeadTime = 0;
	long totalQueueTime = 0;

	for each ( WorkItem wi in CompletedItems )
	{
		totalCycleTime += wi.HoursRequired; // Because resoure only single tasks
		totalLeadTime += ( wi.HourCompleted - wi.HourAddedToQueue);
		totalQueueTime += ( wi.HourTakenFromQueue - wi.HourAddedToQueue );
	}

	std::cout << "Average cycle time: " << totalCycleTime / CompletedItems.size() << "\n";
	std::cout << "Average lead time: " << totalLeadTime / CompletedItems.size() << "\n";
	std::cout << "Average queue time: " << totalQueueTime / CompletedItems.size() << "\n";
	//std::cout << totalQueueTime / CompletedItems.size() << ",";
	std::cout << "Work items completed: " << CompletedItems.size() << "\n";
	//std::cout << CompletedItems.size() << "\n";
}

int _tmain(int argc, _TCHAR* argv[])
{
	
	for ( int i = 1; i<=20; i++ )
	{
		RunExperiment(i);
	}
	
	return 0;
}