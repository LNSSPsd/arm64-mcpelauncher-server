#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <csignal>
#include <cxxabi.h>
#include <execinfo.h>
#include <mcpelauncher/linker.h>
#include <thread>
#include <fcntl.h>
#include <unistd.h>

void print_disasm(uint32_t instruction, uint64_t addr, uint64_t base) {
	if((instruction&0xfffffc1f)==0xd63f0000) {
		printf("BLR X%u ",((instruction>>5)&0x1f));
	}else if((instruction&0xfc000000)==0x94000000) {
		int64_t offset=(instruction&(0x3ffffff))<<2;
		if((offset&0xf000000)==0xf000000)
			offset|=((uint64_t)-1)<<28;
		uint64_t iaddr=(uint64_t)addr+offset;//-4 in call stack;
		printf("BL %4p ",(void*)(iaddr-base));
	}else if((instruction&0xbfe00400)==0xe8400400) {
		// LDR (Pre-index or Post-index)
		printf("LDR ");
		if(instruction>>30==2)
			printf("W");
		else
			printf("X");
		printf("%u, [ ", instruction&((1<<5)-1));
		uint32_t rd=(instruction>>5)&((1<<5)-1);
		if(rd==31)
			printf("SP ");
		else
			printf("X%u ",rd);
		bool postindex=(instruction&0xc00)==0x400;
		int32_t offset=(rd>>12)&((1<<9)-1);
		if(offset&0x100)
			offset|=0xfffffe;
		if(postindex)
			printf("], #%d ",offset);
		else
			printf(", #%d ]! ",offset);
	}else if((instruction&0xbfc00000)==0xb9400000) {
		// LDR (Unsigned)

	}
}

static bool _hasCrashed = false;

void _handleSignal(int signal, void *aptr) {
    printf("Signal %i received\n", signal);

    struct sigaction act;
    act.sa_handler = nullptr;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGSEGV, &act, 0);
    sigaction(SIGABRT, &act, 0);
    sigaction(SIGFPE, &act, 0);
    sigaction(SIGBUS, &act, 0);
    sigaction(SIGILL, &act, 0);

    if(_hasCrashed)
        return;
    _hasCrashed = true;

    void **ptr=&aptr;

    int pipeval[2];
    if(pipe2(pipeval,O_NONBLOCK)==-1) {
	    perror("pipe");
    }
    printf("Dumping stack...\n");
    for(int i=0;i<1500;i++) {
        void* pptr;
       	if(write(pipeval[1],ptr,8)==-1||read(pipeval[0],(void*)&pptr,8)==-1||!pptr) {
		ptr++;
		continue; // not accessible
	}
        Dl_info symInfo;
        if (linker::dladdr(pptr, &symInfo)) {
            int status = 0;
	    printf("#%i \033[1;36m%4p \033[1;32m",i,(void *) ((size_t) pptr - (size_t) symInfo.dli_fbase));
	    size_t instruction;//=(size_t)*((uint32_t*)pptr -1);
	    if(write(pipeval[1],(void*)((uint64_t)pptr-4),4)==-1||read(pipeval[0],(void*)&instruction,4)==-1)
		    instruction=0;
	    if((instruction&0xfffffc1f)==0xd63f0000) {
		    printf("BLR X%zu ",((instruction>>5)&0x1f));
	    }else if((instruction&0xfc000000)==0x94000000) {
		    int64_t offset=(instruction&(0x3ffffff))<<2;
		    if((offset&0xf000000)==0xf000000)
			    offset|=((uint64_t)-1)<<28;
		    size_t addr=(size_t)pptr+offset-4;
		    printf("BL %4p ",(void*)(addr-(size_t)symInfo.dli_fbase));
	    }
	    const char *name=symInfo.dli_fname;
	    for(const char *i=symInfo.dli_fname;*i!=0;i++) {
		    if(*i=='/')
			    name=i+1;
	    }
            printf("\033[min %s [%4p]\n", name, pptr);
        }
        ptr++;
    }
    uint64_t fpval;
    uint64_t retval;
    asm("mov %0, x29\nmov %1, x30":"=r"(fpval),"=r"(retval));
    printf("Calling stack:\n");
    while(1) {
	    Dl_info symInfo;
	    int hasInfo=linker::dladdr((void*)retval,&symInfo);
	    if(!hasInfo) {
		    symInfo.dli_fbase=0;
		    symInfo.dli_fname="unknown";
	    }
	    printf("\033[1;36m%4p \033[1;32m",(void *) ((size_t)retval - (size_t) symInfo.dli_fbase));
	    size_t instruction=(size_t)*((uint32_t*)retval -1);
	    if((instruction&0xfffffc1f)==0xd63f0000) {
		    printf("BLR X%zu ",((instruction>>5)&0x1f));
	    }else if((instruction&0xfc000000)==0x94000000) {
		    int64_t offset=(instruction&(0x3ffffff))<<2;
		    if((offset&0xf000000)==0xf000000)
			    offset|=((uint64_t)-1)<<28;
		    size_t addr=(size_t)retval+offset-4;
		    printf("BL %4p ",(void*)(addr-(size_t)symInfo.dli_fbase));
	    }
	    const char *name=symInfo.dli_fname;
	    for(const char *i=symInfo.dli_fname;*i!=0;i++) {
		    if(*i=='/')
			    name=i+1;
	    }
            printf("\033[min %s [%4p]\n", name, (void*)retval);
	    if(write(pipeval[1],(void*)fpval,8)==-1||write(pipeval[1],(void*)(fpval+8),8)==-1)
		    break;
	    if(read(pipeval[0],(void*)&fpval,8)==-1||read(pipeval[0],(void*)&retval,8)==-1)
		    break;
	    //fpval=*(uint64_t*)fpval;
	    //retval=*(uint64_t*)(fpval+8);
    }
    close(pipeval[1]);
    close(pipeval[0]);
    printf("program failed with unix signal number: %d\n", signal);
    fflush(stdout);
    _Exit(signal);
}

void _registerCrashHandler() {
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_handler = (void (*)(int)) _handleSignal;
    act.sa_flags = 0;
    sigaction(SIGSEGV, &act, 0);
    sigaction(SIGABRT, &act, 0);
    sigaction(SIGFPE, &act, 0);
    sigaction(SIGBUS, &act, 0);
    sigaction(SIGILL, &act, 0);
}
