/***************************************************************************
 *            kprobe_hook.c
 *  http://stackoverflow.com/questions/31406898/opening-writing-to-a-file-from-a-kprobe-handler
 * 
 *  Probe handlers are run with preemption disabled. Depending on the architecture and optimization state, 
 *  handlers may also run with interrupts disabled (e.g., kretprobe handlers and optimized kprobe handlers run 
 *  without interrupt disabled on x86/x86-64). In any case, 
 *  your handler should not yield the CPU (e.g., by attempting to acquire a semaphore).
 *  In other words, you cannot sleep inside probe handler. Because read/write operations with file normally use disk I/O, 
 *  you cannot use these operations inside the handler.
 *  Thu December 08 08:10:58 2016
 *  Copyright  2016  zhoushengmeng
 *  <user@host>
 ****************************************************************************/
/*
 * kprobe_hook.c
 *
 * Copyright (C) 2016 - zhoushengmeng
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "kprobe_hook.h"    /* Needed for kprobehook*/
extern char* install_root;
#define KP_PREHANDLE(FUNC_NAME,SYM_NAME) \
static int (*HANDLE_PRE_##FUNC_NAME)(struct kprobe*,struct pt_regs*);\
static int pre_##FUNC_NAME (struct kprobe *p, struct pt_regs *regs) \
{ 								    \
    if(HANDLE_PRE_##FUNC_NAME) HANDLE_PRE_##FUNC_NAME(p,regs);\
	return 0;					            \
}

#define KP_LIST_ENT(FUNC_NAME,SYM_NAME,I) [I]={.pre_handler = pre_##FUNC_NAME,.symbol_name = SYM_NAME}

#define RP_RETHANDLE(FUNC_NAME,SYM_NAME) \
static int (*HANDLE_RET_##FUNC_NAME)(struct kretprobe_instance *p,struct pt_regs *regs); \
static int ret_##FUNC_NAME(struct kretprobe_instance *p,struct pt_regs *regs) \
{\
	if(HANDLE_RET_##FUNC_NAME) HANDLE_RET_##FUNC_NAME(p,regs);\
	return 0;\
}\

#define RP_LIST_ENT(FUNC_NAME,SYM_NAME,I) [I]={.handler = ret_##FUNC_NAME, .kp={.symbol_name = SYM_NAME}}

#define JP_LIST_ENT(FUNC_NAME,SYM_NAME,I) [I]={.entry = HANDLE_ENTRY_##FUNC_NAME, .kp={.symbol_name = SYM_NAME}}

//{@ add a prehandler of a function just add 2 line code like the example
KP_PREHANDLE(kallsyms_lookup_name,"kallsyms_lookup_name")
KP_PREHANDLE(getdents,"sys_getdents")
KP_PREHANDLE(newlstat,"sys_newlstat")
KP_PREHANDLE(open,"sys_open")

static struct kprobe kprobe_list[] = {
KP_LIST_ENT(kallsyms_lookup_name,"sys_newlstat",0),
KP_LIST_ENT(getdents,"sys_getdents",1),
KP_LIST_ENT(newlstat,"sys_newlstat",2),
KP_LIST_ENT(open,"sys_open",3),
};
//@}


//{@ add a return handler of a function just add 2 line code like the example
RP_RETHANDLE(getdents,"sys_getdents")

static struct kretprobe kretprobe_list[] = {
RP_LIST_ENT(getdents,"sys_getdents",0),
};
//@}

//{@ add a ENTRY handler of a function just add 2 line code like the example
long HANDLE_ENTRY_getdents(int fd,struct dirent *dirp,unsigned int count);
int HANDLE_ENTRY_open(const char *pathname, int flags, mode_t mode);
static struct jprobe jprobe_list[] = {
JP_LIST_ENT(getdents,"sys_getdents",0),
JP_LIST_ENT(open,"sys_open",1),
};
//@}

static long alloc_in_user_space(long size)
{
	return round_down(task_pt_regs(current)->sp - size,16);
}

#define CHG_FILE_NAME(FUNC_NAME,REG) \
void handle_pre_##FUNC_NAME(struct kprobe *p,struct pt_regs *regs){ \
	char file_name[PAGESIZE]={0};\
	int len_filename = strlen_user(regs->REG);\
	if(copy_from_user(file_name,regs->REG,len_filename)){\
		return 0;\
	}\
	if(*file_name != '/'){\
		struct fs_struct *fs = current->fs;\
		char tmpbuf[PAGESIZE]={0};\
		char* pwd = d_path(&(fs->pwd), tmpbuf, PAGESIZE);\
		int len_pwd = strlen(pwd);\
		memcpy(file_name+len_filename,pwd,len_pwd+1);\
		memset(file_name+len_filename+len_pwd,'/',1);\
		memcpy(file_name+len_filename+len_pwd+1,file_name,len_filename+1);\
		memcpy(tmpbuf,file_name+len_filename,len_pwd+len_filename+2);\
		memcpy(file_name,tmpbuf,len_pwd+len_filename+2);\
		printk("long file_name:%s,%p:%p\n",file_name,pwd,file_name);\
	}\
	if(0 == strncmp(file_name,install_root,strlen(install_root)-1)){\
		char* test = "/home/zhoushengmeng/test";\
		struct pt_regs *task_regs = task_pt_regs(current); \
		long len = strlen_user(task_regs->REG);\
		void __user *new_name = alloc_in_user_space(strlen(test)+1); \
		if(copy_to_user(new_name,test,strlen(test)+1)){\
		        printk("copy failed!\n");\
		        return 0;\
		}\
		printk("change name from %lx to %lx llll:%d\n",regs->REG,new_name,strlen_user(new_name));\
		regs->REG = new_name;\
	}\
    return 0; \
}

CHG_FILE_NAME(open,di)

void init_func_ptrs(void){
	HANDLE_RET_getdents = handle_ret_getdents;
	HANDLE_PRE_getdents = handle_pre_getdents;
	HANDLE_PRE_open = handle_pre_open;
}

void register_cowfs_hook(void){
	int len = sizeof(kprobe_list)/sizeof(struct kprobe);
	printk("len:%d\n",len);
	int i;
	for(i=0;i<len;i++)
	{   
		register_kprobe(&kprobe_list[i]);
		printk("fuck %s,%p,%p\n",kprobe_list[i].symbol_name,kprobe_list[i].pre_handler,kprobe_list[i].addr);
	}

	len = sizeof(kretprobe_list)/sizeof(struct kretprobe);
	printk("len2:%d\n",len);
	for(i=0;i<len;i++)
	{   
		register_kretprobe(&kretprobe_list[i]);
		printk("fuck %s,%p,%p\n",kretprobe_list[i].kp.symbol_name,kretprobe_list[i].handler,kretprobe_list[i].kp.addr);
	}

	len = sizeof(jprobe_list)/sizeof(struct jprobe);
	printk("len3:%d\n",len);
	for(i=0;i<len;i++)
	{   
		register_jprobe(&jprobe_list[i]);
		printk("fuck %s,%p,%p\n",jprobe_list[i].kp.symbol_name,jprobe_list[i].entry,jprobe_list[i].kp.addr);
	}

}

void unregister_cowfs_hook(void){
	int len = sizeof(kprobe_list)/sizeof(struct kprobe);
	int i;
	for(i=0;i<len;i++)
	{
		unregister_kprobe(&kprobe_list[i]);
	}

	len = sizeof(kretprobe_list)/sizeof(struct kretprobe);
	for(i=0;i<len;i++)
	{   
		unregister_kretprobe(&kretprobe_list[i]);
	}

	len = sizeof(jprobe_list)/sizeof(struct jprobe);
	for(i=0;i<len;i++)
	{   
		unregister_jprobe(&jprobe_list[i]);
	}
}
