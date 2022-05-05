
#include "jar/any.h"
#include "jar/exec.h"
#include "jar/event.h"

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
        std::promise<float> p;
        e.submit(p, (jar::func<float(float, float)>) [] (float a, float b) -> float {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            return a * b;
        }, a, b);
        float c = p.get_future().get();
        std::cout << jar::now2str() << " - " << "queuer func<float(float, float)> = " << c << std::endl;
    }
    {
        jar::delayer e(std::chrono::milliseconds(500));
        e.submit((jar::func_vv) [] {
            std::cout << jar::now2str() << " - " << "delayer func_vv" << std::endl;
        });
        float a = 3.3;
        float b = 3.3;
        std::promise<float> p;
        e.submit(p, (jar::func<float(float, float)>) [] (float a, float b) -> float {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            return a * b;
        }, a, b);
        e.start();
        float c = p.get_future().get();
        std::cout << jar::now2str() << " - " << "delayer func<float(float, float)> = " << c << std::endl;
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

void test_main_pool() {
    {
        std::promise<void> p;
        jar::async(p, (jar::func_vv) [] {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        });
        p.get_future().wait();
        std::cout << jar::now2str() << " - " << "async func_vv " << std::endl;
    }
    {
        std::promise<float> p;
        jar::async(p, (jar::func<float(void)>) [] () -> float {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            return 3.3;
        });
        float c = p.get_future().get();
        std::cout << jar::now2str() << " - " << "async func<float(void)>: " << c << std::endl;
    }
    {
        float a = 3.3;
        float b = 3.3;
        std::promise<float> p;
        jar::async(p, (jar::func<float(float, float)>) [] (float a, float b) -> float {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            return a * b;
        }, a, b);
        float c = p.get_future().get();
        std::cout << jar::now2str() << " - " << "async func<float(float, float)>: " << c << std::endl;
    }
    {
        std::promise<void> p;
        jar::delay(p, std::chrono::milliseconds(500), (jar::func_vv) [] {
            std::cout << jar::now2str() << " - " << "delay func_vv " << std::endl;
        });
        p.get_future().wait();
    }
    {
        std::promise<float> p;
        jar::delay(p, std::chrono::milliseconds(500), (jar::func<float(void)>) [] () -> float {
            return 3.3;
        });
        float c = p.get_future().get();
        std::cout << jar::now2str() << " - " << "delay func<float(void)>: " << c << std::endl;
    }
    {
        float a = 3.3;
        float b = 3.3;
        std::promise<float> p;
        jar::delay(p, std::chrono::milliseconds(500), (jar::func<float(float, float)>) [] (float a, float b) -> float {
            return a * b;
        }, a, b);
        float c = p.get_future().get();
        std::cout << jar::now2str() << " - " << "delay func<float(float, float)>: " << c << std::endl;
    }
    {
        auto e = jar::loop(std::chrono::milliseconds(500), (jar::func_vv) [] {
            std::cout << jar::now2str() << " - " << "loop func_vv" << std::endl;
        });
        std::this_thread::sleep_for(std::chrono::seconds(3));
        delete e;
    }
    {
        auto e = jar::anim(3.3f, (jar::func_vv) [] {
            std::cout << jar::now2str() << " - " << "anim func_vv" << std::endl;
        });
        std::this_thread::sleep_for(std::chrono::seconds(3));
        delete e;
    }
}

void test_event() {
    {
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
    {
        jar::sub(0x0000000000000001LL, (jar::func_vv) [] {
            std::cout << jar::now2str() << " - " << "main_event_queue" << std::endl;
        });
        jar::pub(0x0000000000000001LL);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

int main() {
    test_any();
    test_exec();
    test_pool();
    test_main_pool();
    test_event();

    std::cout << "Hello World!" << std::endl;

    return 0;
}

