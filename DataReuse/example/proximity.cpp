#define N 100

void func1(int a[N], int b[N], int c[N])
{
    while(1) {
        #pragma omp target teams distribute parallel for
        for(int i=0; i<N; i++) {
            a[i] = 10*a[i];
            b[i] = 100+b[i];
        }
        #pragma omp target teams distribute parallel for
        for(int i=0; i<N; i++) {
            int x;
            x = a[i]*b[i];
            c[i] = x;
        }
    }
}


void func2(int a[N], int b[N], int c[N])
{
    #pragma omp target teams distribute parallel for
    for(int i=0; i<N; i++) {
        a[i] = i;
        b[i] = 100-i;
    }
    for(;;) {
        #pragma omp target teams distribute parallel for
        for(int i=0; i<N; i++) {
            c[i] = a[i]+b[i];
        }
    }
}

