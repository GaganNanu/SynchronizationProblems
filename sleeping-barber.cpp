
#include <iostream>
#include <vector>
#include <pthread.h>
#include <semaphore.h>
#include <windows.h>
#include <queue>

/** 
 * Sleeping barber problem.
 * We used posix threads to create barber and customers.
 * We used 3 semaphores and a mutex to handle synchronization.
 * waiting_room is a counting semaphore to make sure only x number of customers can enter the waiting room where x is the number of chairs.
 * check_waiting_room is a binary semaphore is to make sure two threads don't look at the waiting room at the exact same instance.
 * wake_up_barber is binary semaphore that notifies barber to wake up and waits when he goes to sleep.
 * modify_queue is a mutex used for critical section where we push, pop or read from the queue.
 * The main method takes number of chairs, expected customers and hair cut time. It generates customers in random interval between 0 to 4 seconds.
**/
class BarberShop
{
public:
    pthread_t barber;
    BarberShop(int chairs, int time, int customers);
    bool enter_shop(int customer_id);
    sem_t check_waiting_room;
    sem_t waiting_room;
    sem_t wake_up_barber;
    pthread_mutex_t modify_queue;
    std::queue<int> customer_queue;
    bool is_sleeping;
    int occupied_count;
    int chairs_count;
    int time_taken;
    int customers_serviced;
    int customers_expected;
};

// structure used by property threads to access semaphores and other resources and store customerid.
struct customer_properties
{
    int id;
    BarberShop *barber_shop;
};

// Method used by customers to try to get a haircut or leave if waiting room is full.
void *try_to_get_haircut(void *args1)
{
    customer_properties *props = (customer_properties *)args1;
    int id = props->id;
    BarberShop *barber_shop = props->barber_shop;
    // Get access to waiting room.
    sem_wait(&barber_shop->waiting_room);
    bool can_enter = barber_shop->enter_shop(id);
    if (can_enter == true)
    {
        std::cout << "Customer " << id << " entered the barber shop.\n";
        sem_wait(&barber_shop->check_waiting_room);
        if (barber_shop->is_sleeping == true)
        {
            sem_post(&barber_shop->wake_up_barber);
        }
        sem_post(&barber_shop->check_waiting_room);
        sem_post(&barber_shop->waiting_room);
    }
    else
    {
        std::cout << "Customer " << id << " left the shop because it is full.\n";
        barber_shop->customers_expected--;
        sem_post(&barber_shop->waiting_room);
    }
    return NULL;
}

// Method used by barber to go to sleep or do haircuts.
void *cut_hair_or_sleep(void *args2)
{

    while (true)
    {
        BarberShop *barber_shop = (BarberShop *)args2;
        pthread_mutex_lock(&barber_shop->modify_queue);
        bool queue_empty = barber_shop->customer_queue.empty();
        pthread_mutex_unlock(&barber_shop->modify_queue);

        if (queue_empty == true)
        {
            if (barber_shop->customers_serviced == barber_shop->customers_expected)
            {
                std::cout << "Serviced expected number of customers. Closing shop. \n";
                pthread_exit(0);
            }
            std::cout << "Barber is sleeping. \n";
            barber_shop->is_sleeping = true;
            sem_wait(&barber_shop->wake_up_barber);
        }

        barber_shop->is_sleeping = false;
        pthread_mutex_lock(&barber_shop->modify_queue);
        int customer_to_serve = barber_shop->customer_queue.front();
        barber_shop->customer_queue.pop();
        pthread_mutex_unlock(&barber_shop->modify_queue);

        int time = barber_shop->time_taken;
        std::cout << "Cutting hair of Customer " << customer_to_serve << ".\n";
        // Simulating hair cut by adding a time delay.
        Sleep(std::rand() % time);
        barber_shop->occupied_count--;
        barber_shop->customers_serviced++;
        std::cout << "Customer " << customer_to_serve << " left the shop after haircut.\n";
    }
}

int main()
{
    int no_of_chairs, no_of_customers, time_for_haircut;

    std::cout << "Enter the number of chairs in waiting hall in the barber shop: ";
    std::cin >> no_of_chairs;
    std::cout << "Enter the number of customer that might come to the barber shop. Enter -1 to simulate infinite customers: ";
    std::cin >> no_of_customers;
    std::cout << "Enter time taken for a haircut in seconds. Enter -1 for random time: ";
    std::cin >> time_for_haircut;
    if (no_of_customers == -1)
    {
        no_of_customers = INT_MAX;
    }
    std::cout << "A barbershop with " << no_of_chairs << " chairs is created\n";
    BarberShop barber_shop(no_of_chairs, time_for_haircut, no_of_customers);
    // Simulate time take for opening the shop
    Sleep(2500);

    int customer_id = 0;

    std::vector<pthread_t> customers(no_of_customers);

    while (customer_id < no_of_customers)
    {

        Sleep(std::rand() % 4000);
        customer_properties property = {customer_id, &barber_shop};
        int error = pthread_create(&customers[customer_id], NULL, &try_to_get_haircut, &property);
        if (error != 0)
        {
            std::cout << "Error while creating thread.";
        }
        customer_id++;
    }

    // Join all threads so that main thread will not exit.
    pthread_join(barber_shop.barber, NULL);
    for (int i = 0; i < no_of_customers; i++)
    {
        pthread_join(customers[i], NULL);
    }

    pthread_exit(0);
    return 0;
}

BarberShop::BarberShop(int chairs, int time, int customers)
{
    chairs_count = chairs;
    time_taken = time == -1 ? 6000 : time * 1000;
    occupied_count = 0;
    customers_expected = customers;
    customers_serviced = 0;
    is_sleeping = true;
    pthread_mutex_init(&modify_queue, NULL);
    int error = sem_init(&waiting_room, 0, chairs);
    if (error != 0)
    {
        std::cout << "Unable to initialize the waiting area semaphore.Due to error: " << error << "\n";
        error = 0;
        exit(0);
    }
    error = sem_init(&wake_up_barber, 0, 0);
    if (error != 0)
    {
        std::cout << "Unable to initialize the wake up barber semaphore.Due to error: " << error << "\n";
        error = 0;
        exit(0);
    }
    error = sem_init(&check_waiting_room, 0, 1);
    if (error != 0)
    {
        std::cout << "Unable to initialize the check waiting room semaphore. Due to error: " << error << "\n";
        exit(0);
    }

    error = pthread_create(&barber, NULL, &cut_hair_or_sleep, this);
    if (error)
    {
        std::cout << "Unable to create the barber thread.\n";
        exit(0);
    }
}

bool BarberShop::enter_shop(int customer_id)
{
    if (occupied_count == chairs_count)
    {
        return false;
    }

    if (occupied_count < chairs_count)
    {
        pthread_mutex_lock(&modify_queue);
        occupied_count++;
        customer_queue.push(customer_id);
        pthread_mutex_unlock(&modify_queue);
        // std::cout<<customer_thread<<"is in the waiting area";
    }

    return true;
}
