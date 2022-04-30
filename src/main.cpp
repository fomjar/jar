
#include "any.h"
#include "exec.h"
#include "event.h"

#include <iostream>

void test_any() {
    jar::any a1 = 3;
    jar::any a2 = 3.3f;
    jar::any a3 = std::string("3.3.3");
    std::cout << jar::now2str() << " - " << "any int: " << a1.cast<int>() << std::endl;
    std::cout << jar::now2str() << " - " << "any float: " << a2.cast<float>() << std::endl;
    std::cout << jar::now2str() << " - " << "any string: " << a3.cast<std::string>() << std::endl;
}

void test_exec() {
    {
        jar::queuer e;
        e.start();
        e.submit((jar::func_vv) [] {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::cout << jar::now2str() << " - " << "queuer func_vv" << std::endl;
        });
        float a = 3.3;
        float b = 3.3;
        float c = e.submit((jar::func<float, float, float>) [] (float a, float b) -> float {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            return a * b;
        }, a, b).get_future().get();
        std::cout << jar::now2str() << " - " << "queuer func<float, float, float> = " << c << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    {
        jar::delayer e(std::chrono::milliseconds(500));
        e.submit((jar::func_vv) [] {
            std::cout << jar::now2str() << " - " << "delayer func_vv" << std::endl;
        });
        float a = 3.3;
        float b = 3.3;
        auto p = e.submit((jar::func<float, float, float>) [] (float a, float b) -> float {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            return a * b;
        }, a, b);
        e.start();
        float d = p.get_future().get();
        std::cout << jar::now2str() << " - " << "delayer func<float, float, float> = " << d << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    }
    {
        jar::looper e(std::chrono::milliseconds(500));
        e.submit((jar::func_vv) [] {
            std::cout << jar::now2str() << " - " << "looper func_vv" << std::endl;
        });
        e.start();
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    {
        jar::animator e(3.33f);
        e.submit((jar::func_vv) [] {
            std::cout << jar::now2str() << " - " << "animator func_vv" << std::endl;
        });
        e.start();
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

void test_pool() {
    {
        jar::fixed_pool pool;
        for (int i = 0; i < 6; i++) {
            pool.submit((jar::func_vv) [=] {
                std::cout << jar::now2str() << " - " << "fixed_pool " << i << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            });
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    }
    {
        jar::cached_pool pool;
        for (int i = 0; i < 6; i++) {
            pool.submit((jar::func_vv) [=] {
                std::cout << jar::now2str() << " - " << "cached_pool " << i << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            });
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void test_event() {
    jar::event_queue<uint32_t>      queue_int;
    jar::event_queue<std::string>   queue_str;

    queue_int.sub(0x00000001, (jar::func_v<std::string>) [] (std::string str) {
        std::cout << jar::now2str() << " - " << "int event_queue: " << str << std::endl;
    });
    queue_str.sub("0x00000001", (jar::func_v<std::string>) [] (std::string str) {
        std::cout << jar::now2str() << " - " << "string event_queue: " << str << std::endl;
    });

    queue_int.pub(0x00000001, std::string("Hello World!"));
    queue_str.pub("0x00000001", std::string("Hello World!"));

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

int main() {
    test_any();
    test_exec();
    test_pool();
    test_event();

    std::cout << "Hello World!" << std::endl;

    return 0;
}

