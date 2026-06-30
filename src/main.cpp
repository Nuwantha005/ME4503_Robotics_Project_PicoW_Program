#include <stdio.h>
#include "pico/stdlib.h"
#include <memory>

int main()
{
    stdio_init_all();

    std::unique_ptr<int> a = std::make_unique<int>(42);
}
