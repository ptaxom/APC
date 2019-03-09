#include <iostream>
#include <stdio.h>
#include <Windows.h>
#include <cstdlib>
#include <string>
#include <ctime>
#include <random>



const int N = 8;
const int measured_times = 1;
const bool debug = true;

using word = short;

word A[N][N];
word B[N][N];
word B_Transpose[N][N];

void random_init_matrix(word X[N][N])
{
	std::random_device rd;
	for (int i = 0; i < N; i++)
		for (int j = 0; j < N; j++)
			X[i][j] = rd() % 8;
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
	int C[N][N];
	for(int u = 0; u < measured_times; u++)
	{
		for (int i = 0; i < N; i++)
			for (int j = 0; j < N; j++)
			{
				int sum = 0;
				for (int k = 0; k < N; k++)
					sum += A[i][k] * B_Transpose[j][k];
				C[i][j] = sum;
			}
	}
	if (debug) {
		print_Matrix("A: ", A);
		print_Matrix("B: ", B);
		print_Matrix("C: ", C);
	}
}


void MMX_Mult()
{
	int C[N][N];
	{
		int cnt, row_size = N * sizeof(word), i = 0, j = 0;
		_asm {
			//using eax, edi, esi, ebx
			pusha
			mov ecx, measured_times
			loop_measure:
			mov i, 0
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

			dec ecx
			jnz loop_measure
			emms
			popa
		}
	}
	if (debug)
		print_Matrix("MMX: ", C);
}

void AVX_Mult()
{
	int C[N][N];
	{
		int cnt, row_size = N * sizeof(word), i = 0, j = 0;
		_asm {
			//using eax, edi, esi, ebx
			pusha
			mov ecx, measured_times
		loop_measure :
			mov i, 0
			mov eax, N		//eax - max_size
			xor edi, edi	//edi - ptr for A matrix
		loop_i :
			xor esi, esi	//esi - ptr for B_T matrix
			mov j, 0
		loop_j :
			vpxor XMM3, XMM3, XMM3
			mov edx, N
		loop_k :
		
			movdqu XMM0, A[edi]
			movdqu XMM1, B_Transpose[esi]

			vpmaddwd XMM2, XMM0, XMM1
			vpaddd	XMM3, XMM3, XMM2

			add edi, 16
			add esi, 16
			sub edx, 8
			jnz loop_k
			//end k loop
			sub edi, row_size	//restore esi
			//save value in C[i][j]

			vphaddd XMM2, XMM3, XMM3
				
			vmovdqu XMM0, XMM2
			vpsrlq XMM2, XMM2, 32
			vpaddd XMM2, XMM2, XMM0
			

			push esi
			push edi
			imul edi, 2
			
			mov esi, j
			imul esi, 4		//multiply on sizeof(int)
			add esi, edi	//now esi points to C[i][j]
			
			movdqu C[esi], XMM2

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

			dec ecx
			jnz loop_measure
			emms
			popa
		}
	}
	if (debug)
		print_Matrix("AVX: ", C);
}


void ASM_Mult() {
	int C[N][N];
	{
		int cnt, row_size = N * sizeof(word), i = 0, j = 0, m_times = measured_times, k = 0;
		_asm {
			//using eax, edi, esi, ebx
			pusha
			loop_measure :
			mov i, 0
			mov ebx, N		//ebx - max_size
			xor edi, edi	//edi - ptr for A matrix
		loop_i :
			xor esi, esi	//esi - ptr for B_T matrix
			mov j, 0
		loop_j :
			xor edx, edx
			mov k, ebx
			xor ecx, ecx
		loop_k :
			xor eax, eax
			mov ax, word ptr A[edi]
			mov cx, word ptr B_Transpose[esi]
			
			imul ax, cx
			add edx, eax

			add edi, 2
			add esi, 2
			dec k
			jnz loop_k
			//end k loop
			sub edi, row_size	//restore edi
			//save value in C[i][j]
			push esi
			push edi
			imul edi, 2

			mov esi, j
			imul esi, 4		//multiply on sizeof(int)
			add esi, edi	//now esi points to C[i][j]
			mov C[esi], edx

			pop edi
			pop esi
			//update iteratos
			inc j
			cmp j, ebx
			jne loop_j
			//end j loop
			add edi, row_size
			inc i
			cmp i, ebx
			jne loop_i

			dec m_times
			jnz loop_measure

			popa
		}
	}
	if (debug)
		print_Matrix("C_ASM: ", C);
}


void time_measure_wrapper(void(*fun)(), const char *label)
{
	LARGE_INTEGER frequency, start, finish;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&start);
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

	if (debug) {
		C_Matrix_Mult();
		//ASM_Mult();
		//MMX_Mult();
		AVX_Mult();
	}
	else {
		time_measure_wrapper(&C_Matrix_Mult, "C function:   ");
		time_measure_wrapper(&ASM_Mult, "ASM function: ");
		time_measure_wrapper(&MMX_Mult, "MMX function: ");
	}


	system("pause");
	return 0;
}