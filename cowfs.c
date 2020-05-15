/*  
 *  cowfs.c - Demonstrates module documentation.
 */

#include "kprobe_hook.h"    /* Needed for kprobehook*/

#define DRIVER_AUTHOR "zhoushengmeng <zhoushengmeng@meizu.com>"
#define DRIVER_DESC   "copy on write fs"

char* private_root = "/home/cow_private/";
char* install_root = "/home/zhoushengmeng/Code/";
module_param(private_root, charp, 0000);
MODULE_PARM_DESC(private_root, "A character string for private_root");
module_param(install_root, charp, 0000);
MODULE_PARM_DESC(install_root, "A character string for install_root");

static int __init init_cowfs(void)
{
	printk(KERN_INFO "Cowfs init ============= {@!\n");
	printk("the parameter is %s:%s\n",install_root,private_root);
	
	init_func_ptrs();
	register_cowfs_hook();
	return 0;
}

static void __exit cleanup_cowfs(void)
{
	printk(KERN_INFO "Cowfs exit ===================@}!\n");
	unregister_cowfs_hook();
}

module_init(init_cowfs);
module_exit(cleanup_cowfs);

/*  
 *  You can use strings, like this:
 */

/* 
 * Get rid of taint message by declaring code as GPL. 
 */
MODULE_LICENSE("GPL");

/*
 * Or with defines, like this:
 */
MODULE_AUTHOR(DRIVER_AUTHOR);	/* Who wrote this module? */
MODULE_DESCRIPTION(DRIVER_DESC);	/* What does this module do */
