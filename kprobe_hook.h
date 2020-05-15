/***************************************************************************
 *            kprobe_hook.h
 *
 *  Thu December 08 08:10:58 2016
 *  Copyright  2016  zhoushengmeng
 *  <user@host>
 ****************************************************************************/
/*
 * kprobe_hook.h
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
#include <linux/kprobes.h> /* Needed for use kprobe funcs */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */
#include <linux/version.h>
#include <linux/limits.h>
#include <linux/slab.h>
#include <linux/dcache.h>
#include <linux/uaccess.h>
#include <linux/file.h>
#include <asm/mman.h>
#include <linux/fs.h>
#include <linux/syscalls.h>
#include <linux/fdtable.h>
#include <linux/fs_struct.h>
#include <linux/namei.h>


void register_cowfs_hook(void);
void unregister_cowfs_hook(void);
void init_func_ptrs(void);

void handle_pre_getdents(struct kprobe *p,struct pt_regs *regs);
void handle_ret_getdents(struct kretprobe_instance *p,struct pt_regs *regs);


#define PAGESIZE 4096
#define FAKE_ROOT char* fakeroot = private_root ;\
	int frlen = strlen(fakeroot);