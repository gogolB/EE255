#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>

int main(int argc, char *argv[] )
{
	char* operand;
	char* A;
	char* B;
	int P1 = 0;
	char op = '';
	int P2 = 0;
	int* result;
	char resFinal;
	
	
	if(argc > 4)
	{
		printf("Too many input arguements\n");
	}
	else if(argc < 4)
	{
		printf("Too few input arguements\n");
	}
	else
	{
		A = argv[1];
		operand = argv[2];
		B = argv[3];
		// Check the paramters to make sure they are actually numbers
		if(isalpha(A[0]) || isalpha(B[0]))
		{
			printf("Input parmeters are non-numeric \n");
			return -1;
		}
		else
		{	
			P1 = atoi(A);
			P2 = atoi(B);
		}
		// Check the operand to see if it's surrounded by single quotes
		if(operand[0] == ''')
		{
			op = operand[1];
		}

		// Call the calc system call
		syscall(__NR_calc(P1,P2,op,result);
		resFinal =  atoi(result,buffer,10);
		printf(resfinal);
		printf("\n");
		if(isalpha(resFinal[0])
		{
			return -1;
		}
		else
		{
			return 0;
		}
		

	}
		
	return 0;

}
