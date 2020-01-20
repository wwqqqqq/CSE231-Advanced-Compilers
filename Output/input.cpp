int foo (int x) {
    int i = 0;
    volatile int count = 0;
    for(; i < x; i++) {
        count ++;
    }

    return count;
}

int main() {
    foo(5);
    return 0;
}