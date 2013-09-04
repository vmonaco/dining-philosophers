//============================================================================
// name        : diningphilosophers.cpp
// author      :
// version     :
// copyright   :
// description : the dining philosophers problem
//============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/date_time.hpp>
#include <boost/timer.hpp>

using namespace std;
using namespace boost;

#define NUM_PHILOSOPHERS 3
#define NUM_CYCLES 500

// milliseconds
#define MIN_TIME_TO_THINK 100
#define MAX_TIME_TO_THINK 150
#define MIN_TIME_TO_EAT 100
#define MAX_TIME_TO_EAT 150

//#define TIME_TO_THINK rand()%(MAX_TIME_TO_THINK - MIN_TIME_TO_THINK) + MIN_TIME_TO_THINK
//#define TIME_TO_EAT rand()%(MAX_TIME_TO_EAT - MIN_TIME_TO_EAT) + MIN_TIME_TO_EAT

#define TIME_TO_THINK 0
#define TIME_TO_EAT 0

#define OBSERVER_INTERVAL 100

// philospher states
#define WAITING 0
#define THINKING 1
#define EATING 2

struct MillisecondTimer {

	void Reset() {
		gettimeofday(&start, NULL);
	}

	long TimeElapsed() {
		gettimeofday(&end, NULL);
		seconds = end.tv_sec - start.tv_sec;
		useconds = end.tv_usec - start.tv_usec;
		return ((seconds) * 1000 + useconds / 1000.0) + 0.5;
	}

private:
	struct timeval start, end;
	long seconds, useconds;
};

struct Observer {

	Observer(int waitingState, long timeBetweenQueries) :
		mWaitingState(waitingState),
		mTimeBetweenQueries(timeBetweenQueries),
		mDeadlockReached(false) {
	}

	void UpdateState(int id, int state) {
		mMutex.lock();
		mStates[id] = state;
		mMutex.unlock();
	}

	void Run() {
		while (!mDeadlockReached) {
			mMutex.lock();

			mDeadlockReached = true;
			map<int, int>::iterator it = mStates.begin();
			while (it != mStates.end()) {
				if (it->second != mWaitingState) {
					mDeadlockReached = false;
					break;
				}
			}

			mMutex.unlock();

			// sleep
			this_thread::sleep(mTimeBetweenQueries);
		}

		cout << endl << "********** Deadlock reached **********" << endl;
	}

private:
	mutex mMutex;
	map<int, int> mStates;
	bool mDeadlockReached;
	int mWaitingState;
	posix_time::milliseconds mTimeBetweenQueries;
};

struct Chopstick {

	Chopstick(int id) :
			mID(id), mTimeWaiting(0), mTimeWaitingToEat(0), mTimeEating(0) {
		mTimer.Reset();
	}

	void PickUp() {
		mMutex.lock();
		mTimeWaiting += mTimer.TimeElapsed();
		mTimer.Reset();
	}

	void UseToEat() {
		mTimeWaitingToEat += mTimer.TimeElapsed();
		mTimer.Reset();
	}

	void PutDown() {
		mTimeEating += mTimer.TimeElapsed();
		mTimer.Reset();
		mMutex.unlock();
	}

	int GetID() {
		return mID;
	}

	void PrintStats(ostream& out) {
		out << "Chopstick " << mID
				<< " stats. Time spent (milliseconds) on table: "
				<< mTimeWaiting << ", waiting to eat: " << mTimeWaitingToEat
				<< ", eating: " << mTimeEating << ", in hand: "
				<< (mTimeWaitingToEat + mTimeEating) << endl;
	}

protected:
	mutex mMutex;
	int mID;
	long mTimeWaiting, mTimeWaitingToEat, mTimeEating;
	MillisecondTimer mTimer;
};

struct Philosopher {

public:

	Philosopher(int id, Chopstick* leftChopstick, Chopstick* rightChopstick, Observer* observer) :
			mID(id), mTimeWaiting(0), mTimeEating(0), mTimeThinking(0),
			mLeftChopstick(leftChopstick), mRightChopstick(rightChopstick),  mObserver(observer) {

	}

	void PickupChopsticks() {
		mTimer.Reset();

		// aquire the left and right chopsticks to eat
		mLeftChopstick->PickUp();
//				cout << endl << "Philosopher " << mID << " picked up chopstick " << mLeftChopstick->GetID() << endl;
		mRightChopstick->PickUp();
//				cout << endl << "Philosopher " << mID << " picked up chopstick " << mRightChopstick->GetID() << endl;

		// accumulate the time this philosopher waits for the chopsticks
		mTimeWaiting += mTimer.TimeElapsed();
	}

	void PutdownChopsticks() {
		// release the chopsticks
		mLeftChopstick->PutDown();
//				cout << endl << "Philosopher " << mID << " put down chopstick " << mLeftChopstick->GetID() << endl;
		mRightChopstick->PutDown();
//				cout << endl << "Philosopher " << mID << " put down chopstick " << mRightChopstick->GetID() << endl;
	}

	void Think(int timeToThink) {
		cout << endl << "Philosopher " << mID << " thinking." << endl;
		mTimeThinking += timeToThink;

		posix_time::milliseconds thinkTime(timeToThink);
		this_thread::sleep(thinkTime);
	}

	void Eat(int timeToEat) {
		cout << endl << "Philosopher " << mID << " eating." << endl;
		mTimeEating += timeToEat;

		mLeftChopstick->UseToEat();
		mRightChopstick->UseToEat();
		posix_time::milliseconds eatTime(timeToEat);
		this_thread::sleep(eatTime);
	}

	/**
	 * Alternatively think and aquire both chopsticks to eat
	 */
	void ThinkAndEat(int cycles) {
		while (cycles--) {
			mObserver->UpdateState(mID, THINKING);
			Think(TIME_TO_THINK);
			mObserver->UpdateState(mID, WAITING);
			PickupChopsticks();
			mObserver->UpdateState(mID, EATING);
			Eat(TIME_TO_EAT);
			PutdownChopsticks();
		}
	}

	void PrintStats(ostream& out) {
		out << "Philosopher " << mID
				<< " done. Time spent (milliseconds) eating: " << mTimeEating
				<< ", thinking: " << mTimeThinking << ", waiting: "
				<< mTimeWaiting << endl;
	}

protected:
	Chopstick* mLeftChopstick;
	Chopstick* mRightChopstick;
	int mID;
	long mTimeWaiting, mTimeThinking, mTimeEating;
	MillisecondTimer mTimer;
	Observer* mObserver;
};


void CreatePhilosphers(int numPhilosphers, int numCycles) {
	cout << "Creating " << numPhilosphers << " philosophers and chopsticks"
			<< endl;

	vector<Chopstick*> chopsticks;
	for (int i = 0; i < numPhilosphers; i++) {
		chopsticks.push_back(new Chopstick(i));
	}

	Observer* observer = new Observer(WAITING, OBSERVER_INTERVAL);
	vector<Philosopher*> philosophers;
	for (int i = 0; i < numPhilosphers; i++) {
		philosophers.push_back(
				new Philosopher(i, chopsticks[i],
						chopsticks[(i + 1) % numPhilosphers], observer));
	}

	thread_group threadGroup;
	MillisecondTimer mainTimer;

	threadGroup.add_thread(new thread(&Observer::Run, observer));
	mainTimer.Reset();
	for (int i = 0; i < numPhilosphers; i++) {
		threadGroup.add_thread(
				new thread(&Philosopher::ThinkAndEat, philosophers[i],
						numCycles));
	}
	threadGroup.join_all();
	long totalTime = mainTimer.TimeElapsed();

	cout << "Total time (ms) elapsed for " << numPhilosphers
			<< " philosophers to eat/think " << numCycles << " times: "
			<< totalTime << endl;

	for (int i = 0; i < numPhilosphers; i++) {
		philosophers[i]->PrintStats(cout);
	}

	for (int i = 0; i < numPhilosphers; i++) {
		chopsticks[i]->PrintStats(cout);
	}

	for (int i = 0; i < numPhilosphers; i++) {
		delete philosophers[i];
		delete chopsticks[i];
	}
}

int main(int argc, char* argv[]) {

	srand(std::time(NULL));

	CreatePhilosphers(NUM_PHILOSOPHERS, NUM_CYCLES);

	return 0;
}
