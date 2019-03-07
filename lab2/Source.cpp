#include <iostream>
#include <stdio.h>
#include <Windows.h>
#include <cstdlib>
#include <string>
#include <ctime>
#include <random>

const int N = 32;
const int measured_times = 1000 * 1000;

using word = short;

word A[N][N];
word B[N][N];
word B_Transpose[N][N];

void random_init_matrix(word X[N][N])
{
	std::random_device rd;
	for (int i = 0; i < N; i++)
		for (int j = 0; j < N; j++)
			X[i][j] = rd() % 10;
}


void transpose()
{
	for (int i = 0; i < N; i++)
		for (int j = 0; j < N; j++)
			B_Transpose[i][j] = B[j][i];
}


template<class Type>
void print_Matrix(std::string label,Type X[N][N])
{
	std::cout << label << std::endl;
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++)
			std::cout << X[i][j] << " ";
		std::cout << std::endl;
	}
	std::cout << std::endl;
}


void C_Matrix_Mult() {
	{
		int C[N][N];
		for (int i = 0; i < N; i++)
			for (int j = 0; j < N; j++)
			{
				int sum = 0;
				for (int k = 0; k < N; k++)
					sum += A[i][k] * B_Transpose[j][k];
				C[i][j] = sum;
			}
	}
	/*print_Matrix("A: ", A);
	print_Matrix("B: ", B);
	print_Matrix("C: ", C);*/
}


void MMX_Mult()
{
	{
		int C[N][N];
		int cnt, row_size = N * sizeof(word), i = 0, j = 0;
		_asm {
			//using eax, edi, esi, ebx
			pusha
			mov eax, N		//eax - max_size
			xor edi, edi	//edi - ptr for A matrix
			loop_i :
			xor esi, esi	//esi - ptr for B_T matrix
				mov j, 0
				loop_j :
				pxor MM7, MM7
				mov edx, N
				loop_k :
			movq MM0, A[edi]
				movq MM1, B_Transpose[esi]

				pmaddwd MM0, MM1
				paddd MM7, MM0
				add edi, 8
				add esi, 8
				sub edx, 4
				jnz loop_k
				//end k loop
				sub edi, row_size	//restore esi
				//save value in C[i][j]
				movq MM0, MM7
				psrlq MM7, 32
				paddd MM7, MM0

				push esi
				push edi
				imul edi, 2

				mov esi, j
				imul esi, 4		//multiply on sizeof(int)
				add esi, edi	//now esi points to C[i][j]

				movd edx, MM7
				movd C[esi], MM7

				pop edi
				pop esi
				//update iteratos
				inc j
				cmp j, eax
				jne loop_j
				//end j loop
				add edi, row_size
				inc i
				cmp i, eax
				jne loop_i

				emms
				popa
		}
	}
	//print_Matrix("C: ", C);
}


void time_measure_wrapper(void(*fun)(), const char *label)
{
	LARGE_INTEGER frequency, start, finish;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&start);
	for(int i = 0; i < measured_times; i++)
		fun();
	QueryPerformanceCounter(&finish);
	float delay = (finish.QuadPart - start.QuadPart) * 1000.0f / frequency.QuadPart;
	printf("%s time elapsed : %f ms\n", label, delay);
}




int main()
{
	random_init_matrix(A);
	random_init_matrix(B);
	transpose();
	time_measure_wrapper(&C_Matrix_Mult, "C function:   ");
	time_measure_wrapper(&MMX_Mult,      "MMX function: ");


	system("pause");
	return 0;
}