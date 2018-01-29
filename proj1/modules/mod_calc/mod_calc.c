#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/unistd.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Souradeep Bhattacharya and Jesse Garcia");
MODULE_DESCRIPTION("A module that modifies the functionality of the sys_calc system call");
MODULE_VERSION("0.1");


unsigned long *sys_call_table;





asmlinkage int (*original_sys_calc)(int, int, char, int*);

asmlinkage int mod_sys_calc(int param1, int param2, char operand, int* result)
{
	// Check if we have a good result address
	if(result == 0)
	{
		return -1;
	}

	// Make sure it was an orignally supported operand.
	if(operand == '+' || operand == '-' || operand == '*' || operand == '/')
	{
		// You can't mod by 0
		if(param2 == 0)
		{
			return -1;
		}
		
		// return param1 % param2
		*result = param1 % param2;
	}
	else
	{
		// Got a garbage operand.
		return -1;
	}

	return 0;
}

static int init_mod_calc(void)
{
	printk("Loading Mod_calc\n");
	sys_call_table =(unsigned long*) simple_strtoul("0x80010064",NULL,16);

	// Store a copy of the original one so we can restore the functionality
	original_sys_calc=sys_call_table[__NR_calc];
	printk("Saved original sys_calc");

	// Load our mod calc
	sys_call_table[__NR_calc]=mod_sys_calc;
	printk("Loaded in mod_calc");	
	
	return 0;
}

static void exit_mod_calc(void)
{
	printk("Exiting Mod_Calc");

	// Restore the original calc function
	sys_call_table[__NR_calc]=original_sys_calc;
	printk("Restored original sys_calc");
}

module_init(init_mod_calc);
module_exit(exit_mod_calc);

