#include <seccomp.h>
#include <error.h>
#include <link.h>
#include <errno.h>

#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>

#include <sys/mman.h>   // mmap & munmap

typedef int (*main_t)(int, char**, char**);
main_t realmain;

typedef long (*sysconf_t)(int);
sysconf_t realsysconf;

int wrap_main(int argc, char** argv, char** environ)
{
  // Ensure none of our children will ever be granted more priv
  // (via setuid, capabilities, ...)
  prctl(PR_SET_NO_NEW_PRIVS, 1);

  // Ensure no escape is possible via ptrace
  prctl(PR_SET_DUMPABLE, 0);
 
  // Init the filter
  scmp_filter_ctx ctx;

  // ctx = seccomp_init(SCMP_ACT_KILL);
  // ctx = seccomp_init(SCMP_ACT_ERRNO(ENOSYS));
  ctx = seccomp_init(SCMP_ACT_TRAP);
  // ctx = seccomp_init(SCMP_ACT_TRACE (2));

  if (!ctx) {
    printf("seccomp_init failed");
    return 0;
  }
 
  // Setup syscall whitelist 

  // Allow reading and writing for stdin, stdout, stderr, and fd 3.
  if (0>seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(read), 1,
                         SCMP_A0(SCMP_CMP_LE, STDERR_FILENO+1)))
    goto print;

  if (0>seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(write), 1,
                         SCMP_CMP(0, SCMP_CMP_LE, STDERR_FILENO+1))) 
    goto print;
  if (0>seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(close), 1,
                         SCMP_A0(SCMP_CMP_LE, STDERR_FILENO+1))) 
    goto print;

  // malloc
  if (0>seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(brk), 0)) 
    goto print;

  if (0>seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(clock_gettime), 0)) 
    goto print;

  // if (0>seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(dup), 0)) 
  //   goto print;

  if (0>seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit), 0)) 
    goto print;

  if (0>seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit_group), 0)) 
    goto print;

  if (0>seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(futex), 0)) 
    goto print;

  if (0>seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(gettimeofday), 0)) 
    goto print;

  if (0>seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mmap), 0)) 
    goto print;

  if (0>seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mmap2), 0)) 
    goto print;

  // v8::internal::OS::ProtectCode
  if (0>seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mprotect), 1,
      SCMP_CMP(2, SCMP_CMP_EQ, PROT_READ | PROT_EXEC))) 
    goto print;

  // v8::internal::VirtualMemory::Guard
  if (0>seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mprotect), 1,
      SCMP_CMP(2, SCMP_CMP_EQ, PROT_NONE))) 
    goto print;

  // v8::internal::OS::Free
  if (0>seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(munmap), 0)) 
    goto print;

  // build and load the filter
  if (0<seccomp_load(ctx)) 
    goto print;

  seccomp_release(ctx);

  return (*realmain)(argc, argv, environ);

 print:
  perror("seccomp_load");
  return -1;
}

long sysconf(int name) {
  switch (name) {
    case 84:
      return 1;
    default:
      return realsysconf(name);
  }
}

int __libc_start_main(main_t main,
                      int argc,
                      char ** ubp_av,
                      ElfW(auxv_t) * auxvec,
                      __typeof (main) init,
                      void (*fini) (void),
                      void (*rtld_fini) (void), void * stack_end)
{
  void *libc;
  int (*libc_start_main)(main_t main,
                         int,
                         char **,
                         ElfW(auxv_t) *,
                         __typeof (main),
                         void (*fini) (void),
                         void (*rtld_fini) (void),
                         void * stack_end);
  libc = dlopen("libc.so.6", RTLD_LOCAL | RTLD_LAZY);
  if (!libc)
    error (-1, errno, "dlopen() failed: %s\n", dlerror());
  libc_start_main = dlsym(libc, "__libc_start_main");
  if (!libc_start_main)
    error (-1, errno, "Failed: %s\n", dlerror());
  realmain = main;
  realsysconf = dlsym(libc, "sysconf");
  return (*libc_start_main)(wrap_main, argc, ubp_av, auxvec, init, fini, rtld_fini, stack_end);
}