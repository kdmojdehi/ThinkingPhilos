/*
Multicore Programming PA2
Version 1 of problem 2: this version only simulates the dinning philosophers problem
author: karan daei-mojdehi (k.mojdehi@knights.ucf.edu), Fall 2016
*/

#include <pthread.h>
#include <mutex>
#include <iostream>
#include <cstdio>
#include <time.h>
#include <unistd.h>
#include <ncurses.h>

#define kPhilos 5

using namespace std;

// pthread_mutex_t chop_stick[kPhilos];
mutex chop_stick[kPhilos]; //using std mutex
mutex print_mutex, exit_mutex; // if exit mutex is unlcoked, threads start quiting

// function used for printing from threads, locks what is printed to screen.
void lock_and_print(char const *s)
{
  lock_guard<mutex> lock(print_mutex);
  cout <<"\r" << s;
  return;
}

bool pick_stick(int stick_num)
{
  if (stick_num<0) stick_num = kPhilos-1; // the case where philo at seat 0 picks left stick
  // pthread_mutex_lock(&chop_stick[stick_num]);
  return chop_stick[stick_num].try_lock();
}

void leave_stick(int stick_num)
{
  if (stick_num<0) stick_num = kPhilos-1; // the case where philo at seat 0 leaves left stick
  // pthread_mutex_unlock(&chop_stick[stick_num]);
  chop_stick[stick_num].unlock();
}

class Philosopher {
  int seat_= 0;
  char *buffer_ = new char[50];
public:
  Philosopher(int id)
  {
    seat_ = id;
  }

  int get_seat() {return seat_;};

  void start_eating(int duration)
  {
    bool has_left_stick = false, has_right_stick = false;
    while ( !(has_left_stick & has_right_stick))
    {
      if (!has_left_stick)
        has_left_stick = pick_stick(seat_ - 1); // left stick
      if (!has_right_stick)
        has_right_stick = pick_stick(seat_); //right stick
    }
    sprintf(buffer_, "%d is now eating.\n\r", seat_);
    lock_and_print(buffer_);
    // sleep(duration);

  };

  void thinking(int duration)
  {
    sprintf(buffer_, "%d is now thinking.\n", seat_);
    lock_and_print(buffer_);
    leave_stick(seat_); //leave right stick
    leave_stick(seat_ - 1); //leave left stick
    // sleep(duration);
    sprintf(buffer_, "%d is now hungry.\n", seat_);
    lock_and_print(buffer_);
  };
};


void *ThreadLoop(void *data)
{
  int *id = (int *)data;
  char *buffer = new char[200];
  int duration=5;
  Philosopher Philo = Philosopher(*id);
  while (!exit_mutex.try_lock())
  {
    // generate a random duration (2-5secs) for thinking
    duration = rand()%4 + 2;
    Philo.start_eating(duration);
    // generate a random number (2-5 secs) for eating:
    duration = rand()%4 + 2;
    Philo.thinking(duration);
  }
  sprintf(buffer, "Thread %d stopped spinning\n", *id);
  lock_and_print(buffer);
  exit_mutex.unlock();
  pthread_exit(NULL);
}

int main()
{
  initscr();
  cbreak();
  noecho();
  refresh();
  pthread_t threads[kPhilos];
  int ids[kPhilos];
  int rc;
  void *status;
  char *buffer = new char[200];
  srand(time(NULL));
  exit_mutex.lock(); // unlocking it will cause all threads to stop spinning
  for(int i=0; i < kPhilos; i++ )
  {
    // init pthread_mutex for all forks:
    //  pthread_mutex_init(&chop_stick[i], NULL);
     sprintf(buffer, "main(): Creating philosopher: %d\n", i);
     lock_and_print(buffer);
     ids[i] = i;
     rc = pthread_create(&threads[i], NULL,
                         ThreadLoop, (void *)(ids+i));
     if (rc)
     {
        cout << "Error:unable to create thread," << rc << endl;
        exit(-1);
     }
  }

  lock_and_print("Waiting for entry..");
  int pressed = 0;
  do {
    pressed = getch();
    lock_and_print("got char!");
  } while (pressed != 110);
  // cout << "you pressed n!, exitting\n" << endl;
  lock_and_print("you pressed n, exitting\n");
  // exit from all threads:
  exit_mutex.unlock();
  lock_and_print("Waiting for threads to stop spinning...\n");
  sleep(5);
  endwin();
  return 0;
}
