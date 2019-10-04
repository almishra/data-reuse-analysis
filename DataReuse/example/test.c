int main() {
    int a = 10;

#pragma omp target teams distribute parallel
    for(int i=0; i<10; i++) {
        int b = a;
        a++;
    }
    return 0;
}
