#include <stdio.h>
#include <stdlib.h>
#include <asm/unistd.h>

int main(int argc, char *argv[] )
{
	char* operand;
	char* A;
	char* B;
	int P1 = 0;
	char op = '/';
	int P2 = 0;
	int r;
	int result;
	char buffer[20];
	
	
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
			//printf("%d",P1);
			//printf("\n");
			P2 = atoi(B);
			//~ printf("%d",P2);
			//~ printf("\n");

		}
		// Check the operand to see if it's surrounded by single quotes
		if(operand[0] == '\'')
		{
			op = operand[1];
			//~ printf("%c",op);
			//~ printf("\n");

			

		}
		else
		{
			op = operand[0];
			//~ printf("%c",op);
			//~ printf("\n");

		}
		
		// Call the calc system call
		r = syscall(397,P1,P2,'+',&result);
		
		if(r == 0)
		{
			//itoa(&result,buffer,10);
			printf("%d",result);
			printf("\n");
			return 0;
		}
		else
		{
			printf("NaN\n");
			return 0;
		}
		

	}
		
	return 0;

}
