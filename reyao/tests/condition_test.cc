#include "reyao/mutex.h"
#include <thread>
#include <unistd.h>

using namespace std;
using namespace reyao;

Mutex mtx;
Condition cond(mtx);

int main() {
    thread t([&]() {
        sleep(3);
        cond.notifyAll();
    });

    cond.waitForSeconds(10);

    if (t.joinable())   t.join();
}