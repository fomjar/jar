
#include "any.h"
#include "exec.h"

#include <string>
#include <iostream>

void test_any() {
    jar::any a1 = 3;
    jar::any a2 = 3.3f;
    jar::any a3 = std::string("3.3.3.3");
    std::cout << a1.cast<int>() << std::endl;
    std::cout << a2.cast<float>() << std::endl;
    std::cout << a3.cast<std::string>() << std::endl;
}

void test_exec() {
    {
        jar::queuer q;
        q.start();
        q.submit((jar::func_vv) [] {
            std::cout << "queuer func_vv" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        });
        float a = 3.3;
        float b = 3.3;
        float c = q.submit((jar::func<float, float, float>) [] (float a, float b) -> float {
            std::cout << "queuer func<float, float, float>" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            return a * b;
        }, a, b).get_future().get();
        std::cout << "queuer func<float, float, float> = " << c << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main() {
    test_any();
    test_exec();

    std::cout << "Hello World!" << std::endl;

    return 0;
}

