#define N 10000000

int main()
{
    int a[N];
    int cnt = 10000;
    while(cnt > 0)
    {
#pragma omp target teams distribute parallel for 
        for(int i=0; i<N; i++) {
            a[i]++;
        }
        cnt--;
    }

    return 0;
}
