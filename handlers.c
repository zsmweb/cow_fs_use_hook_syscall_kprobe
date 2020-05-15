/***************************************************************************
 *            handlers.c
 *
 *  Thu December 08 12:02:15 2016
 *  Copyright  2016  zhoushengmeng
 *  <user@host>
 ****************************************************************************/
#include "kprobe_hook.h"    /* Needed for kprobehook*/
#include "utils.h"
extern char* private_root;
extern char* install_root;


#define COPY_MAX_SIZE 4096*1000
static int kernel_copy_file(char* fin,char* fout){
	mm_segment_t fs = get_fs();
    set_fs(KERNEL_DS);
	struct file* file_in = filp_open(fin,O_RDWR|O_CREAT,0);
	set_fs(fs);
	if(IS_ERR(file_in)){
		printk("open  file  filed:%s\n",fin);
		return 0;
	}
	fs = get_fs();
    set_fs(KERNEL_DS);
	//create_path(fout);
	struct file* file_out = filp_open(fout,O_RDWR|O_CREAT,0777);
	set_fs(fs);
	if(IS_ERR(file_out)){
		printk("open  file  filed:%s\n",fout);
        filp_close(file_in,NULL);
		return 0;
	}
	long fsize = file_in->f_path.dentry->d_inode->i_size;
	long bufsize = fsize;
	if(fsize >= COPY_MAX_SIZE)
		bufsize = COPY_MAX_SIZE; 
	char * buf = kmalloc(bufsize,GFP_KERNEL);
	do{
		loff_t *pos_in = &(file_in->f_pos);
		loff_t *pos_out = &(file_out->f_pos);
		//printk("COPYED start %lx:%lx:%lx\n",bufsize,*pos_in,fsize);
        fs = get_fs();
        set_fs(KERNEL_DS);
		vfs_read(file_in, buf, bufsize, pos_in); 
		vfs_write(file_out, buf, bufsize, pos_out);
	    set_fs(fs);
		fsize -= bufsize;
		//printk("COPYED size %lx:%lx:%lx\n",bufsize,*pos_out,fsize);
		if(fsize < COPY_MAX_SIZE)
			bufsize = fsize;
	}while(fsize);
	kfree(buf);
	fs = get_fs();
    set_fs(KERNEL_DS);
	filp_close(file_in,NULL);
	filp_close(file_out,NULL);
	set_fs(fs);
	return fsize;
}
static int list_files(struct files_struct * files,char* name)
{
	int i, j;
	struct fdtable *fdt;

	j = 0;

	/*
	 * It is safe to dereference the fd table without RCU or
	 * ->file_lock because this is the last reference to the
	 * files structure.  But use RCU to shut RCU-lockdep up.
	  */
    char tmp[PAGESIZE];
	rcu_read_lock();
	fdt = files_fdtable(files);
	rcu_read_unlock();
	for (;;) {
		unsigned long set;
		i = j * BITS_PER_LONG;
		if (i >= fdt->max_fds)
			break;
		set = fdt->open_fds[j++];
		while (set) {
			if (set & 1) {
				struct file * file = fdt->fd[i];
				if (file) {
				    char *p_p = tmp;
              	    memset(p_p,0,PAGESIZE);
              	    p_p = d_path(&(file->f_path), p_p, PAGESIZE);
                    if(strcmp(p_p,name)){
                        //printk("compare failed :%s:%s\n",p_p,name);
                    }else{
						printk("compare ok :%s:%s\n",p_p,name);
                        return i;
                    }
              	    //printk("FD:%d:filename:%s\n",i,p_p);
				}
			}
			i++;
			set >>= 1;
		}
	}
    return -1;
}

void handle_pre_getdents(struct kprobe *p,struct pt_regs *regs)
{
    kuid_t uid = current_fsuid();
    if((unsigned int)uid.val == 0)
        return;
    struct pt_regs *task_regs = task_pt_regs(current); 
    int fd = task_regs->di;
    long count  = task_regs->dx;
    struct file* dir = fget(fd);
    char pppath[PAGESIZE] = {0};
    FAKE_ROOT
    memcpy(pppath,fakeroot,frlen);
    sprintf(pppath+frlen,"%d",uid);
    int len_priroot = strlen(pppath);
    //printk("%d:%s\n",len_priroot,pppath);
    char* ppath = pppath + len_priroot;
    ppath = d_path(&(dir->f_path), ppath, PAGESIZE-len_priroot);
    memcpy(pppath +len_priroot,ppath,strlen(ppath));
    //printk("file:%s:%s:fd:%d,ret:%d:current:%p:\n",pppath,ppath,fd,regs->ax,current);
    //printk("hello pos is %lx\n",dir->f_pos);
    if(strncmp(ppath,install_root,strlen(install_root)-1)==0){
		printk("func: %s  \n", __func__);
		if(dir->f_pos == 0){
		    
		}else if(dir->f_pos>=0x7fffffffffffffff){
			int fakefd = list_files(current->files, pppath);
		    if(fakefd >= 0){
		        struct file* fakedir = fget(fakefd);
		        if(fakedir && fakedir->f_pos >= 0x7fffffffffffffff){
		            printk("nothing done !!!!!!!!!!!!\n");
		        }else{
		            regs->di = fakefd;
		        }
		    }
		}
    }
    fput(dir);
}

void handle_ret_getdents(struct kretprobe_instance *p,struct pt_regs *regs)
{
    kuid_t uid = current_fsuid();
    if((unsigned int)uid.val == 0)
        return;
    struct pt_regs *task_regs = task_pt_regs(current); 
    int fd = task_regs->di;
    long count  = task_regs->dx;
    struct file* dir = fget(fd);
    char pppath[PAGESIZE] = {0};
    FAKE_ROOT
    memcpy(pppath,fakeroot,frlen);
    sprintf(pppath+frlen,"%d",uid);
    int len_priroot = strlen(pppath);
    //printk("%d:%s\n",len_priroot,pppath);
    char* ppath = pppath + len_priroot;
    ppath = d_path(&(dir->f_path), ppath, PAGESIZE-len_priroot);
    memcpy(pppath +len_priroot,ppath,strlen(ppath));
    //printk("file:%s:%s:fd:%d,ret:%d:current:%p:\n",pppath,ppath,fd,regs->ax,current);
    //printk("hello pos is %lx\n",dir->f_pos);
    if(strncmp(ppath,install_root,strlen(install_root)-1)==0){
		printk("func: %s  \n", __func__);
		if(dir->f_pos == 0){
		    printk("none access 1 !!!!!!!!!!!!\n");
		}else if(dir->f_pos>=0x7fffffffffffffff){
			int fakefd = list_files(current->files, pppath);
		    if(fakefd >= 0){
		        struct file* fakedir = fget(fakefd);
		        if(fakedir && fakedir->f_pos >= 0x7fffffffffffffff){
		            mm_segment_t o_fs = get_fs();
					set_fs(KERNEL_DS);
		            sys_close(fakefd);
					set_fs(o_fs);
		            printk("XXXXXXXXXXXXXXXXX:close fd:%d\n",fakefd);
		        }
				if(fakedir&&fakedir->f_pos>0){
					struct pt_regs* task_regs = task_pt_regs(current);
					long len = regs->ax;
					printk("%lx:%lx\n",regs->di,task_regs->di);
					printk("%lx:%lx\n",regs->si,task_regs->si);
					printk("%lx:%lx\n",regs->dx,task_regs->dx);
					printk("%lx:%lx\n",regs->ax,task_regs->ax);
					if(len<=0||len>task_regs->dx)
						return 0;

					char *buf = kmalloc(len,GFP_KERNEL);
					if(buf){
						copy_from_user(buf,task_regs->si,len);
						//unsigned short int lec = (unsigned short int)buf[16];
						//printk(":fuck::%x  :%d\n",lec,sizeof(short int));
						int j = 0;
						int i = 0;
						for(;i<len;){
							int entlen = (unsigned short int)buf[i+16];
							printk("xxx:%s\n",buf+i+16+2);
							if(strcmp(buf+i+16+2,".")&&strcmp(buf+i+16+2,"..")){
								printk("is not the . or ..\n");
								copy_to_user(task_regs->si+j,buf+i,entlen);
								j+=entlen;
							}
							i+=entlen;
						}
						regs->ax = j;
						printk("change return value 2 %x\n",j);
						kfree(buf);
					}
					goto end_return;
				}
		    }	
		}
		//here change result for normal mode
		printk("here change result for normal mode!{@\n");
		struct pt_regs* task_regs = task_pt_regs(current);
		long len = regs->ax;
		printk("%lx:%lx\n",regs->di,task_regs->di);
		printk("%lx:%lx\n",regs->si,task_regs->si);
		printk("%lx:%lx\n",regs->dx,task_regs->dx);
		printk("%lx:%lx\n",regs->ax,task_regs->ax);
		if(len<=0||len>task_regs->dx){
			printk("return 0 @}\n");
			return 0;
		}

		char *buf = kmalloc(len,GFP_KERNEL);
		if(buf){
			copy_from_user(buf,task_regs->si,len);
			//unsigned short int lec = (unsigned short int)buf[16];
			//printk(":fuck::%x  :%d\n",lec,sizeof(short int));
			int j = 0;
			int i = 0;
			for(;i<len;){
				int entlen = (unsigned short int)buf[i+16];
				printk("xxx:%s\n",buf+i+16+2);
				if(strcmp(buf+i+16+2,"zsm")&&strcmp(buf+i+16+2,"ddddd")){
					printk("is not the zsm or ddddd\n");
					copy_to_user(task_regs->si+j,buf+i,entlen);
					j+=entlen;
				}
				i+=entlen;
			}
			regs->ax = j;
			printk("change return value 2 %x\n",j);
			kfree(buf);
		}
		printk("here change result for normal mode!@}\n");
    }
end_return:
    fput(dir);
}
/***
 *	we can't call the functions caused sleeping like sys_open .etc in prehandle
 *  so we call those functions in entry in jprobes
 */
long HANDLE_ENTRY_getdents(int fd,struct dirent *dirp,unsigned int count)
{
    kuid_t uid = current_fsuid();
    if((unsigned int)uid.val == 0)
        jprobe_return();
    //printk("func: %s  \n", __func__);    
    struct file* dir = fget(fd);
    char pppath[PAGESIZE] = {0};
    FAKE_ROOT
    memcpy(pppath,fakeroot,frlen);
    sprintf(pppath+frlen,"%d",uid);
    int len_priroot = strlen(pppath);
    //printk("%d:%s\n",len_priroot,pppath);
    char* ppath = pppath + len_priroot;
    ppath = d_path(&(dir->f_path), ppath, PAGESIZE-len_priroot);
    memcpy(pppath +len_priroot,ppath,strlen(ppath));
    //printk("hello pos is %lx\n",dir->f_pos);
    if(strncmp(ppath,install_root,strlen(install_root)-1)==0){
		if(dir->f_pos == 0){
		    bool irq_stat = irqs_disabled();
		    unsigned long flags;
		    local_irq_save(flags);
		    irq_stat = irqs_disabled();

			mm_segment_t o_fs = get_fs();
		    if(irq_stat) 
		        local_irq_enable();
		    set_fs(KERNEL_DS);
		    /***
			 * call sys_open in userspase
			 * because we have hooked sys_open ,we should not call sys_open straitly
			 ***/
			int fakefd = get_unused_fd_flags(O_DIRECTORY);
			if (fakefd >= 0) {
				struct file *f  = filp_open(pppath,O_DIRECTORY,0);
				if (IS_ERR(f)) {
					put_unused_fd(fakefd);
					fd = PTR_ERR(f);
				} else {
					fd_install(fakefd, f);
				}
			}
		    set_fs(o_fs);

		    local_irq_restore(flags);

		}else if(dir->f_pos>=0x7fffffffffffffff){
			
		}
    }

    jprobe_return();
    return 0;
}

int HANDLE_ENTRY_open(const char *pathname, int flags, mode_t mode){
	//printk("func: %s  \n", __func__);
	if ((flags & O_ACCMODE) & (O_WRONLY | O_RDWR ) != 0){
		if(pathname[0] != '/'){
			struct fs_struct *fs = current->fs;
			char pppath[PAGESIZE] = {0};
			char* pwd = d_path(&(fs->pwd), pppath, PAGESIZE);
			printk("renative wirte  the file %s/%s\n",pwd,pathname);
			unsigned long flags;
		    local_irq_save(flags);
		    bool irq_stat = irqs_disabled();
			if(irq_stat) 
		        local_irq_enable();
			//preempt_enable();/*use set_fs instead!*/
			create_path("/home/zsmweb/aaaa/bbbbb/ccccc/234.txt");
			kernel_copy_file("/home/zsmweb/123.txt","/home/zsmweb/aaaa/bbbbb/ccccc/234.txt");
			local_irq_restore(flags);
		}else
			printk("wirte  the file %s\n",pathname);
	}
	
	jprobe_return();
    return 0;
}

