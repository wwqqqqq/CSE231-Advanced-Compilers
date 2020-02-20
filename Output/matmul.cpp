#include <stdlib.h>
using namespace std;

int* genMat(int x, int y) {
    int* m = (int*)malloc(sizeof(int)*x*y);
    for(int i = 0; i < x * y; i++)
        m[i] = i;
    return m;
}

void matmul(int x, int y, int z, int* A, int *B, int *C) {
    for(int i = 0; i < x; i++) {
        for(int j = 0; j < y; j++) {
            for(int k = 0; k < z; k++) {
                C[i*z+j] += A[i*y+k] * B[k*z+j];
            }
        }
    }
}

int main () {
	int* A = genMat(5,6);
    int* B = genMat(6,4);
    int* C = (int*)malloc(sizeof(int)*5*4);
    matmul(5,6,4,A,B,C);
    free(A);
    free(B);
    free(C);
	return 0;
}
