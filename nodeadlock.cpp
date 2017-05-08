/*
Multicore Programming PA2
Version 2 of problem 2: this version uses a policy to avoid dead locks : each philosopher yields to the one in his right
author: karan daei-mojdehi (k.mojdehi@knights.ucf.edu), Fall 2016
*/

#include <pthread.h>
#include <mutex>
#include <iostream>
#include <cstdio>
#include <time.h>
#include <unistd.h>
#include <ncurses.h>
#include <condition_variable>

#define kPhilos 5

using namespace std;

pthread_mutex_t chop_stick[kPhilos];
// mutex chop_stick[kPhilos]; //using std mutex
mutex print_mutex, exit_mutex; // if exit mutex is unlcoked, threads start quiting ,
pthread_mutex_t first_state; // first_state is the mutex for state of first philosopher (#0)
pthread_cond_t first_is_thinking; // condition variable for alerting last philosopher of state of first one (has to do with dead_lock free policy)

// positive modulus of a number:
int positive_mod(int a, int b)
{
  return ((a%b + b)%b);
}

// function used for printing from threads, locks what is printed to screen.
void lock_and_print(char const *s)
{
  lock_guard<mutex> lock(print_mutex);
  cout <<"\r" << s;
  return;
}

void pick_stick(int stick_num)
{
  stick_num = positive_mod(stick_num, kPhilos); // the case where philo at seat 0 picks left stick
  pthread_mutex_lock(&chop_stick[stick_num]);
  // chop_stick[stick_num].lock();
}

void leave_stick(int stick_num)
{
  stick_num = positive_mod(stick_num, kPhilos); // the case where philo at seat 0 leaves left stick
  pthread_mutex_unlock(&chop_stick[stick_num]);
  // chop_stick[stick_num].unlock();
}

class Philosopher {
public:
  int seat_= 0;
  bool thinking_ = false;
  char *buffer_ = new char[50];
  Philosopher *right_philo_, *left_philo_;

  Philosopher(){};

  Philosopher(int id, Philosopher *left, Philosopher *right) : seat_(id), left_philo_(left), right_philo_(right) , thinking_(false){};

  int get_seat() {return seat_;};

  bool is_thinking() {return thinking_;}

  void start_eating(int duration)
  {
    // apply version 2 policy here: pick left stick first, and then right stick,
    // with exception for last philosopher: make sure first philosopher is thinking:
    if (seat_==kPhilos-1)
    {
      pthread_mutex_lock(&first_state);
      if (!right_philo_->is_thinking())
      {
        lock_and_print("last Philo: waiting for first to finish...\n");
        pthread_cond_wait(&first_is_thinking, &first_state); //
        pick_stick(seat_); //pick right stick first
        lock_and_print("last Philo: first philo is done, I got my right chop stick!\n");
      }
      pthread_mutex_unlock(&first_state);
    }
    pick_stick(seat_ - 1); // pick left stick first (if you are last philosopher, you have picked right stick by now)
    if (seat_ != kPhilos-1) pick_stick(seat_); //pick right stick (for last philosopher: don't pick it again if you alread have it)
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
    thinking_ = true;
    // sleep(duration);
    thinking_ = false;
    sprintf(buffer_, "%d is now hungry.\n", seat_);
    lock_and_print(buffer_);
  };

  // same as thinking, except that it uses a mutex for first philosopher
  void thinking_first(int duration)
  {
    sprintf(buffer_, "%d is now thinking.\n", seat_);
    lock_and_print(buffer_);
    leave_stick(seat_); //leave right stick
    leave_stick(seat_ - 1); //leave left stick
    pthread_mutex_lock(&first_state);
    thinking_ = true;
    pthread_cond_signal(&first_is_thinking); //signal the last philosopher in case he is waiting for first to finish
    pthread_mutex_unlock(&first_state);
    // sleep(duration);
    pthread_mutex_lock(&first_state);
    thinking_ = false;
    pthread_mutex_unlock(&first_state);
    sprintf(buffer_, "%d is now hungry.\n", seat_);
    lock_and_print(buffer_);
  }
};

void *ThreadLoop(void *data)
{
  char *buffer = new char[200];
  int duration=5;
  Philosopher *Philo;
  Philo = (Philosopher *) data;
  while (!exit_mutex.try_lock())
  {
    // generate a random duration (2-5secs) for thinking
    duration = rand()%4 + 2;
    Philo->start_eating(duration);
    // generate a random number (2-5 secs) for eating:
    duration = rand()%4 + 2;
    if (Philo->get_seat()==0)
    {
      Philo->thinking_first(duration); // this method has mutex and condition_variables for changing state
    } else Philo->thinking(duration);

  }
  sprintf(buffer, "Thread %d stopped spinning\n", Philo->seat_);
  lock_and_print(buffer);
  exit_mutex.unlock();
  pthread_exit(NULL);
}

int main()
{
  initscr(); //these first 4 lines are for initializing ncurses library (used for reaciting instantly to keyboard input n for exit)
  cbreak();
  noecho();
  refresh();
  pthread_t threads[kPhilos];
  Philosopher Philo[kPhilos];
  int ids[kPhilos];
  int rc;
  void *status;
  char *buffer = new char[200];
  srand(time(NULL));
  exit_mutex.lock(); // unlocking it will cause all threads to stop spinning
  pthread_mutex_init(&first_state, NULL);
  pthread_cond_init(&first_is_thinking, NULL);
  for(int i=0; i < kPhilos; i++ )
  {
    // init pthread_mutex for all forks:
     pthread_mutex_init(&chop_stick[i], NULL);
     sprintf(buffer, "main(): a philosopher sits at seat #%d\n", i);
     lock_and_print(buffer);
     Philo[i] = Philosopher(i, &Philo[positive_mod((i-1),kPhilos)], &Philo[(i+1)%kPhilos]);
     rc = pthread_create(&threads[i], NULL,
                         ThreadLoop, (void *)(Philo+i));
     if (rc)
     {
        cout << "Error:unable to create thread," << rc << endl;
        exit(-1);
     }
  }
  int pressed = 0;
  do {
    pressed = getch();
    lock_and_print("got char!");
  } while (pressed != 110);
  lock_and_print("you pressed n, exitting\n");
  // exit from all threads:
  exit_mutex.unlock();
  lock_and_print("Waiting for threads to stop spinning...\n");
  sleep(7);
  endwin();
  return 0;
}
