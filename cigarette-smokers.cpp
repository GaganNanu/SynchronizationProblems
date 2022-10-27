#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <windows.h>

/**
 * Cigarette smokers problem.
 * We used posix threads to create agent and smokers.
 * We used 4 semaphores to handle synchronization.
 * smoking_complete semaphore used by agent to wait for the smoker to finish smoking. This ensures agent doesn't place next ingredients before a smoker finishes smoking
 * notify_tobacco_smoker is to notify smoker with unlimited matches that paper and matches are on the table.
 * notify_matches_smoker is to notify smoker with unlimited tobacco that paper and tobacco are on the table.
 * notify_paper_smoker is to notify smoker with unlimited matches that matches and tobacco are on the table.
 * The main method takes user input from user for average smoking time and runs infinitely.
*/

// structure sent to agent constructor with all semaphores.
struct agent_properties
{
  sem_t *smoking_completed;
  sem_t *tobacco_notifier;
  sem_t *matches_notifier;
  sem_t *paper_notifier;
};

// structure sent to smoker with required semaphores, ingredients he has and time to smoke.
struct smoker_properties
{
  std::string ingredient;
  int time_to_smoke;
  sem_t *smoking_completed;
  sem_t *wait_for_ingredient;
};

// Function declarations to be used by agent and smoker threads.
void *place_ingredients(void *args);
void *wait_or_smoke(void *args);

// Smoker class definition
class Smoker
{
public:
  Smoker(smoker_properties *props);
  pthread_t smoker_thread;
};

// Agent class definition.
class Agent
{
public:
  Agent(agent_properties *props);
  pthread_t agent_thread;
};

// Smoker constructor to create new smoker pthread
Smoker::Smoker(smoker_properties *props)
{
  int error = pthread_create(&smoker_thread, NULL, &wait_or_smoke, (void *)props);
  if (error != 0)
  {
    std::cout << "Error while creating smoker thread.";
    exit(0);
  }
}

// Agent constructor to create new agent pthread
Agent::Agent(agent_properties *props)
{
  int error = pthread_create(&agent_thread, NULL, &place_ingredients, (agent_properties *)props);
  if (error != 0)
  {
    std::cout << "Error while creating agent thread.";
    exit(0);
  }
}

// Method used by agent threads to randomly place 2 smoking ingredients on the table.
void *place_ingredients(void *args)
{
  agent_properties *props = (agent_properties *)args;
  sem_wait(props->smoking_completed);
  while (true)
  {
    // randomly generate a number from 0 to 2 and use it to place different ingredients.
    int number = rand() % 3;
    if (number == 0)
    {
      std::cout << "Agent places Matches and Paper on table\n";
      sem_post(props->tobacco_notifier);
    }
    else if (number == 1)
    {
      std::cout << "Agent places Tobacco and Matches on table\n";
      sem_post(props->paper_notifier);
    }
    else
    {
      std::cout << "Agent places Tobacco and Paper on table\n";
      sem_post(props->matches_notifier);
    }
    std::cout << "Agent waiting for smoker to finish his cigarette.\n";
    sem_wait(props->smoking_completed);
  }

  return NULL;
}

// Method used by smokers to take the ingredients they need to smoke or wait for their turn.
void *wait_or_smoke(void *args)
{
  smoker_properties *props = (smoker_properties *)args;
  std::string name = "Smoker with ";
  name.append(props->ingredient);
  while (true)
  {
    sem_wait(props->wait_for_ingredient);
    std::cout << name << " is smoking\n";
    int time = props->time_to_smoke;
    Sleep(time);
    sem_post(props->smoking_completed);
    std::cout << name << " is finished smoking\n\n";
  }
  return NULL;
}

int main()
{
  int time_to_smoke;
  std::cout << "Enter the time taken to smoke in seconds. -1 for Default: ";
  std::cin >> time_to_smoke;
  time_to_smoke = time_to_smoke == -1 ? 5000 : time_to_smoke * 1000;

  // Create and Intialize all 4 semaphores.
  sem_t notify_tobacco_smoker, notify_matches_smoker, notify_paper_smoker, smoking_complete;

  int error = sem_init(&smoking_complete, 0, 1);
  if (error != 0)
  {
    std::cout << "Error while intializing semaphores.";
    exit(0);
  }

  error = sem_init(&notify_tobacco_smoker, 0, 0);
  if (error != 0)
  {
    std::cout << "Error while intializing semaphores.";
    exit(0);
  }

  error = sem_init(&notify_matches_smoker, 0, 0);
  if (error != 0)
  {
    std::cout << "Error while intializing semaphores.";
    exit(0);
  }

  error = sem_init(&notify_paper_smoker, 0, 0);
  if (error != 0)
  {
    std::cout << "Error while intializing semaphores.";
    exit(0);
  }

  smoker_properties tobacco_props = {"Tobacco", time_to_smoke, &smoking_complete, &notify_tobacco_smoker};
  smoker_properties matches_props = {"Matches", time_to_smoke, &smoking_complete, &notify_matches_smoker};
  smoker_properties paper_props = {"Paper", time_to_smoke, &smoking_complete, &notify_paper_smoker};

  // Create smokers with respective properties.
  Smoker tobacco(&tobacco_props), paper(&paper_props), matches(&matches_props);

  Sleep(1000);

  agent_properties agent_props = {&smoking_complete, &notify_tobacco_smoker, &notify_matches_smoker, &notify_paper_smoker};

  // Create agent with agent properties.
  Agent agent(&agent_props);

  // Join smoker threads so that the process doesn't terminate.
  pthread_join(tobacco.smoker_thread, NULL);
  pthread_join(matches.smoker_thread, NULL);
  pthread_join(paper.smoker_thread, NULL);

  pthread_exit(0);
  return 0;
}
