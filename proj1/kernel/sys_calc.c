#include <linux/kernel.h>

asmlinkage int sys_calc(int param1, int param2, char operand, int *result)
{
	if(result == 0)
	{
		// Recieved a null pointer
		printk(KERN_WARNING"Got a bad result pointer");
		return -1;	
	}

	printk(KERN_INFO"Param1=%d,Param2=%d",param1,param2);

	// Reset the value at the pointer
	*result = 0;

	if(operand == '+')
	{
		// Operation is add
		// Returns param1 + param2
		
		*result = param1 + param2;
	}
	else if(operand == '-')
	{
		// Operation is subtraction
		// Returns param1 - param2

		*result = param1 - param2;
	}
	else if(operand == '*')
	{
		// Operation is multiplication
		// Returns param1 * param2

		*result = param1 * param2;
	}
	else if(operand == '/')
	{
		// Operation is division
		// Returns param1 / param2
		
		// If param2 is 0 return -1 (Divide by zero not possible)
		if(param2 == 0)
		{
			printk(KERN_WARNING"Divide by zero not possible");
			return -1;
		}

		*result = param1 / param2;
	}
	else
	{
		// User entered an operand that isn't supported
		printk(KERN_WARNING"Operand not supported: %c", operand);
		return -1;
	}
	return 0;
}
