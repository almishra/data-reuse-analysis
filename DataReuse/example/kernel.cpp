#define N 10000

int main()
{
    int a[N];
    int cnt = 100;
//#pragma omp target data map(a)
    {
#pragma omp target parallel for 
        for(int i=0; i<N; i++) {
            a[i]=0;
        }
    while(cnt > 0)
    {
#pragma omp target teams distribute parallel for 
        for(int i=0; i<N; i++) {
            a[i]++;
        }
        cnt--;
    }
    }
    return 0;
}
