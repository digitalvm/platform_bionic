#undef _FORTIFY_SOURCE
#include <link.h>
#include <elf.h>
#include <fcntl.h>
#include <pthread.h>
#include <stddef.h>
#include <sys/socket.h>
#include <unistd.h>
#include "private/libc_logging.h"
#include "private/bionic_globals.h"
#include "private/bionic_page.h"
#include "pthread_internal.h"

extern "C" size_t __malloc_object_size(const void*);
int __pthread_attr_getstack_main_thread(void** stack_base, size_t* stack_size);

extern "C" size_t __dynamic_object_size(const void* ptr) {
  if (__predict_false(!__libc_globals->enable_object_size_checks || __get_thread()->in_malloc)) {
    return __BIONIC_FORTIFY_UNKNOWN_SIZE;
  }

  pthread_internal_t* thread = __get_thread();

  void* stack_base = nullptr;
  void* stack_top = nullptr;
  void* stack_frame = __builtin_frame_address(0);

  if (thread->tid == getpid()) {
    if (__libc_globals->main_thread_stack_top) {
      stack_base = stack_frame;
      stack_top = __libc_globals->main_thread_stack_top;
    }
  } else {
    stack_base = thread->attr.stack_base;
    stack_top = static_cast<char*>(stack_base) + thread->attr.stack_size;
  }

  if (ptr > stack_base && ptr < stack_top) {
    if (__predict_false(ptr < stack_frame)) {
      __libc_fatal("%p is an invalid object address (in unused stack space %p-%p)", ptr, stack_base,
                   stack_frame);
    }
    return static_cast<char*>(stack_top) - static_cast<char*>(const_cast<void*>(ptr));
  }

  if (ptr > __libc_globals->executable_start && ptr < __libc_globals->executable_end) {
    return static_cast<char*>(__libc_globals->executable_end) - static_cast<char*>(const_cast<void*>(ptr));
  }

  return __malloc_object_size(ptr);
}

static size_t phdr_table_get_load_size(const ElfW(Phdr)* phdr_table, size_t phdr_count,
                                       ElfW(Addr)* out_min_vaddr,
                                       ElfW(Addr)* out_max_vaddr) {
  ElfW(Addr) min_vaddr = UINTPTR_MAX;
  ElfW(Addr) max_vaddr = 0;

  bool found_pt_load = false;
  for (size_t i = 0; i < phdr_count; ++i) {
    const ElfW(Phdr)* phdr = &phdr_table[i];

    if (phdr->p_type != PT_LOAD) {
      continue;
    }
    found_pt_load = true;

    if (phdr->p_vaddr < min_vaddr) {
      min_vaddr = phdr->p_vaddr;
    }

    if (phdr->p_vaddr + phdr->p_memsz > max_vaddr) {
      max_vaddr = phdr->p_vaddr + phdr->p_memsz;
    }
  }
  if (!found_pt_load) {
    min_vaddr = 0;
  }

  min_vaddr = PAGE_START(min_vaddr);
  max_vaddr = PAGE_END(max_vaddr);

  if (out_min_vaddr != nullptr) {
    *out_min_vaddr = min_vaddr;
  }
  if (out_max_vaddr != nullptr) {
    *out_max_vaddr = max_vaddr;
  }
  return max_vaddr - min_vaddr;
}

static int callback(struct dl_phdr_info *info, size_t, void* data) {
  void* addr = reinterpret_cast<void*>(info->dlpi_addr);
  if (addr == nullptr) {
    return 0;
  }
  size_t size = phdr_table_get_load_size(info->dlpi_phdr, info->dlpi_phnum, nullptr, nullptr);
  libc_globals* globals = static_cast<libc_globals*>(data);
  globals->executable_start = addr;
  globals->executable_end = static_cast<char*>(addr) + size;
  return 1;
}

void __libc_init_dynamic_object_size(libc_globals* globals) {
  globals->enable_object_size_checks = true;

  dl_iterate_phdr(callback, globals);

  void* current_base;
  size_t current_size;
  if (access("/proc/self/stat", R_OK) == -1) {
    return;
  }
  if (__pthread_attr_getstack_main_thread(&current_base, &current_size)) {
    return;
  }
  globals->main_thread_stack_top = static_cast<char*>(current_base) + current_size;
}

ssize_t readlink(const char* path, char* buf, size_t size) {
  return readlinkat(AT_FDCWD, path, buf, size);
}

ssize_t __readlink_chk(const char* path, char* buf, size_t size, size_t buf_size) {
  if (size > __dynamic_object_size(buf)) {
    __fortify_chk_fail("readlink: prevented write past end of buffer", 0);
  }
  return __unchecked___readlink_chk(path, buf, size, buf_size);
}

ssize_t readlinkat(int dirfd, const char* path, char* buf, size_t size) {
  if (size > __dynamic_object_size(buf)) {
    __fortify_chk_fail("readlinkat: prevented write past end of buffer", 0);
  }
  return __unchecked_readlinkat(dirfd, path, buf, size);
}

ssize_t __readlinkat_chk(int dirfd, const char* path, char* buf, size_t size, size_t buf_size) {
  if (size > __dynamic_object_size(buf)) {
    __fortify_chk_fail("readlinkat: prevented write past end of buffer", 0);
  }
  return __unchecked___readlinkat_chk(dirfd, path, buf, size, buf_size);
}

char* getcwd(char* buf, size_t size) {
  if (buf != nullptr && size > __dynamic_object_size(buf)) {
    __fortify_chk_fail("getcwd: prevented write past end of buffer", 0);
  }
  return __unchecked_getcwd(buf, size);
}

char* __getcwd_chk(char* buf, size_t len, size_t buflen) {
  if (buf != nullptr && len > __dynamic_object_size(buf)) {
    __fortify_chk_fail("getcwd: prevented write past end of buffer", 0);
  }
  return __unchecked___getcwd_chk(buf, len, buflen);
}

ssize_t read(int fd, void* buf, size_t count) {
  if (count > __dynamic_object_size(buf)) {
    __fortify_chk_fail("read: prevented write past end of buffer", 0);
  }
  return __unchecked_read(fd, buf, count);
}

ssize_t __read_chk(int fd, void* buf, size_t count, size_t buf_size) {
  if (count > __dynamic_object_size(buf)) {
    __fortify_chk_fail("read: prevented write past end of buffer", 0);
  }
  return __unchecked___read_chk(fd, buf, count, buf_size);
}

ssize_t write(int fd, const void* buf, size_t count) {
  if (count > __dynamic_object_size(buf)) {
    __fortify_chk_fail("write: prevented read past end of buffer", 0);
  }
  return __unchecked_write(fd, buf, count);
}

ssize_t __write_chk(int fd, const void* buf, size_t count, size_t buf_size) {
  if (count > __dynamic_object_size(buf)) {
    __fortify_chk_fail("write: prevented read past end of buffer", 0);
  }
  return __unchecked___write_chk(fd, buf, count, buf_size);
}

ssize_t pread(int fd, void* buf, size_t byte_count, off_t offset) {
  return pread64(fd, buf, byte_count, static_cast<off64_t>(offset));
}

ssize_t pwrite(int fd, const void* buf, size_t byte_count, off_t offset) {
  return pwrite64(fd, buf, byte_count, static_cast<off64_t>(offset));
}

ssize_t pread64(int fd, void* buf, size_t byte_count, off64_t offset) {
  if (byte_count > __dynamic_object_size(buf)) {
    __fortify_chk_fail("pread64: prevented write past end of buffer", 0);
  }
  return __unchecked_pread64(fd, buf, byte_count, offset);
}

ssize_t pwrite64(int fd, const void* buf, size_t byte_count, off64_t offset) {
  if (byte_count > __dynamic_object_size(buf)) {
    __fortify_chk_fail("pwrite64: prevented read past end of buffer", 0);
  }
  return __unchecked_pwrite64(fd, buf, byte_count, offset);
}

ssize_t send(int socket, const void* buf, size_t len, int flags) {
  return sendto(socket, buf, len, flags, nullptr, 0);
}

ssize_t recv(int socket, void *buf, size_t len, int flags) {
  return recvfrom(socket, buf, len, flags, nullptr, 0);
}

ssize_t recvfrom(int fd, void* buf, size_t len, int flags, const struct sockaddr* src_addr, socklen_t* addr_len) {
  if (len > __dynamic_object_size(buf)) {
    __fortify_chk_fail("recvfrom: prevented write past end of buffer", 0);
  }
  return __unchecked_recvfrom(fd, buf, len, flags, src_addr, addr_len);
}

ssize_t __recvfrom_chk(int socket, void* buf, size_t len, size_t buflen,
                       int flags, const struct sockaddr* src_addr,
                       socklen_t* addrlen) {
  if (len > __dynamic_object_size(buf)) {
    __fortify_chk_fail("recvfrom: prevented write past end of buffer", 0);
  }
  return __unchecked___recvfrom_chk(socket, buf, len, buflen, flags, src_addr, addrlen);
}

ssize_t sendto(int fd, const void* buf, size_t len, int flags, const struct sockaddr* dest_addr, socklen_t addr_len) {
  if (len > __dynamic_object_size(buf)) {
    __fortify_chk_fail("sendto: prevented read past end of buffer", 0);
  }
  return __unchecked_sendto(fd, buf, len, flags, dest_addr, addr_len);
}

ssize_t __sendto_chk(int socket, const void* buf, size_t len, size_t buflen,
                     int flags, const struct sockaddr* dest_addr,
                     socklen_t addrlen) {
  if (len > __dynamic_object_size(buf)) {
    __fortify_chk_fail("sendto: prevented read past end of buffer", 0);
  }
  return __unchecked___sendto_chk(socket, buf, len, buflen, flags, dest_addr, addrlen);
}
