#include <iostream>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <ctime>

using namespace std;

class TaskScheduler {
public:
    TaskScheduler() : stopScheduler(false) {
        workerThread = thread(&TaskScheduler::run, this);
    }

    ~TaskScheduler() {
        {
            lock_guard<std::mutex> lock(mutex);
            stopScheduler = true;
        }
        condVar.notify_all();
        workerThread.join();
    }

    void Add(std::function<void()> task, std::time_t timestamp) {
        lock_guard<std::mutex> lock(mutex);
        tasks.push({task, timestamp});
        condVar.notify_one();
    }

private:
    struct ScheduledTask {
        function<void()> task;
        time_t timestamp;
    };

    priority_queue<ScheduledTask, vector<ScheduledTask>,
            function<bool(ScheduledTask, ScheduledTask)>> tasks{
        [](ScheduledTask a, ScheduledTask b) { return a.timestamp > b.timestamp; }
    };

    thread workerThread;
    mutex mutex;
    condition_variable condVar;
    bool stopScheduler;

    void run() {
        while (true) {
            ScheduledTask scheduledTask;
            {
                unique_lock<std::mutex> lock(mutex);
                condVar.wait(lock, [this] { return stopScheduler || !tasks.empty(); });

                if (stopScheduler && tasks.empty()) {
                    return;
                }

                scheduledTask = tasks.top();
                tasks.pop();
            }

            time_t now = time(nullptr);
            time_t waitTime = scheduledTask.timestamp - now;
            if (waitTime > 0) {
                this_thread::sleep_for(chrono::seconds(waitTime));
            }

            scheduledTask.task();
        }
    }
};


int main() {

    //setlocale(0, "");
    TaskScheduler scheduler;

    scheduler.Add([] { cout << "Task 1: " << time(nullptr) << "\n"; }, time(nullptr) + 3);

    scheduler.Add([] { cout << "Task 2: " << time(nullptr) << "\n"; }, time(nullptr) + 5);

    scheduler.Add([] { cout << "Task 3: " << time(nullptr) << "\n"; }, time(nullptr) + 1);

    this_thread::sleep_for(chrono::seconds(10));

    return 0;
}