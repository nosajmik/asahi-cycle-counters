#include <linux/module.h>     /* Needed by all modules */
#include <linux/kernel.h>     /* Needed for KERN_INFO */
#include <linux/init.h>       /* Needed for the macros */
  
///< The license type -- this affects runtime behavior
MODULE_LICENSE("GPL");
  
///< The author -- visible when you use modinfo
MODULE_AUTHOR("nosajmik");
  
///< The description -- see modinfo
MODULE_DESCRIPTION("Enable cycle counting from Asahi");
  
///< The version of the module
MODULE_VERSION("0.1");
  
static int __init start(void)
{
    printk(KERN_INFO "Loading asahi-cycle-counters module\n");
    printk(KERN_INFO "Hello, world!\n");
    return 0;
}
  
static void __exit stop(void)
{
    printk(KERN_INFO "Unloading asahi-cycle-counters module\n");
}
  
module_init(start);
module_exit(stop);
