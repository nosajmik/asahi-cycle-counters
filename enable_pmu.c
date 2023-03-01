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

    uint64_t val = 1ULL << 8;
    val |= 1ULL << 9;
    asm volatile("msr CNTKCTL_EL1, %0" :: "r" (val));
    uint64_t res;
    asm volatile("mrs %0, CNTKCTL_EL1" : "=r" (res));
    printk(KERN_INFO "CNTKCTL_EL1: %llx\n", res);

    const uint64_t CORE_CYCLES = 0x02;
    asm volatile("msr S3_1_c15_c5_0, %0" :: "r" (CORE_CYCLES));
    const uint64_t AARCH64_EL0 = 1 << (8 + 2);
    asm volatile("msr S3_1_c15_c1_0, %0" :: "r" (AARCH64_EL0));
    uint64_t PMCR0_EL1;
    asm volatile("mrs %0, S3_1_c15_c0_0" : "=r" (PMCR0_EL1));
    PMCR0_EL1 |= 1 << 2;
    asm volatile("msr S3_1_c15_c0_0, %0; isb" :: "r" (PMCR0_EL1));

    return 0;
}
  
static void __exit stop(void)
{
    printk(KERN_INFO "Unloading asahi-cycle-counters module\n");
}
  
module_init(start);
module_exit(stop);
