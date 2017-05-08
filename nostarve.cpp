/*
Multicore Programming PA2
Version 3 of problem 2: this version also uses a policy to avoid starvation : each time a philosopher becomes hungry, he recieves a number.
each Philosopher checks state and priority of the philosopher in his left before picking his left chop stick
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
mutex print_mutex, exit_mutex, que_mutex; // if exit mutex is unlcoked, threads start quiting ,
pthread_mutex_t first_state; // first_state is the mutex for state of first philosopher (#0)
pthread_cond_t first_is_thinking; // condition variable for alerting last philosopher of state of first one (has to do with dead_lock free policy)

// give out a number as priority:
int get_number()
{
  static int prio=0;
  lock_guard<mutex> lock(que_mutex);
  return ++prio;
}

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
  void change_thinking_status(bool stat)
  {
    pthread_mutex_lock(&thinking_mutex_);
    if (stat) // if is thinking, he wouldnt need que
    {
      thinking_ = true;
      que_ = -1;
      // let other philosopher know you ate:
      if (seat_) pthread_cond_broadcast(&is_thinking_); // for first philosopher, two might be listenning
      else pthread_cond_signal(&is_thinking_);
    }
    else // he is getting hungry
    {
      thinking_ = false;
      que_ = get_number();
    }
    pthread_mutex_unlock(&thinking_mutex_);
  }
public:
  int seat_= 0;
  bool thinking_ = false;
  char *buffer_ = new char[50];
  int que_;
  pthread_mutex_t thinking_mutex_;
  pthread_cond_t is_thinking_;
  Philosopher *right_philo_, *left_philo_;

  Philosopher(){};

  Philosopher(int id, Philosopher *left, Philosopher *right) : seat_(id), left_philo_(left), right_philo_(right) , thinking_(false)
  {
    pthread_mutex_init(&thinking_mutex_, NULL);
    pthread_cond_init(&is_thinking_, NULL);
    que_ = -1;
  };

  int get_seat() {return seat_;};

  // this methods returns -1 if philosopher is thinking and returns que number if he is hungry
  int que_or_thinking()
  {
    int que;
    pthread_mutex_lock(&thinking_mutex_);
    que = que_;
    pthread_mutex_unlock(&thinking_mutex_);
    return que;
  };

  void start_eating(int duration)
  {
    int left_philo_que;

      left_philo_que = left_philo_->que_or_thinking();
      if ((left_philo_que <= que_) && (left_philo_que != -1)) //(left philosopher is hungry and got que before you)
      {
        pthread_mutex_lock(&(left_philo_->thinking_mutex_)); // always give prio to first philo, he is watching your que ;)
        pthread_cond_wait(&(left_philo_->is_thinking_), &(left_philo_->thinking_mutex_)); //
        left_philo_que = left_philo_->que_;
        pthread_mutex_unlock(&(left_philo_->thinking_mutex_));
      }
      pick_stick(seat_ - 1); // pick left stick first
      pick_stick(seat_); // pick right stick
      sprintf(buffer_, "%d is now eating.\n\r", seat_);
      lock_and_print(buffer_);
    // sleep(duration);

  };

  void thinking(int duration)
  {
    sprintf(buffer_, "%d is now thinking.\n", seat_);
    lock_and_print(buffer_);
    change_thinking_status(true);
    leave_stick(seat_); //leave right stick
    leave_stick(seat_ - 1); //leave left stick
    // sleep(duration);
    change_thinking_status(false);
    sprintf(buffer_, "%d is now hungry.\n", seat_);
    lock_and_print(buffer_);
  };

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
    Philo->thinking(duration);
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
