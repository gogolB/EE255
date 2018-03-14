#include <config/auto.conf>
#include <linux/kernel.h>
#include <linux/sysfs.h>

struct kobject * my_dev;
my_dev = kobject_create_and_add("ee255/partitioning_policy",NULL);
