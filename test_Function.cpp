﻿#include <cstdio>
#include <iostream>
#include "Functional.h"

void func_hello(int i) {
    printf("#%d Hello\n", i);
}

struct func_printnum_t {
    void operator()(int i) const {
        printf("#%d Numbers are: %d, %d\n", i, x, y);
    }
    int x;
    int y;
};

void repeattwice(Function<void(int)> const& func) {
    func(1);
    func(2);
}

int main() {
    int x = 4;
    int y = 2;

    func_printnum_t func_printnum{ x, y };
    
    //repeattwice(func_hello);
    return 0;
}
