#include <proc.h>
#include <elf.h>
#include <fs.h>

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

#if defined(__ISA_AM_NATIVE__)
#define EXPECT_TYPE_ISA EM_X86_64
#elif defined(__ISA_X86_64__)
#define EXPECT_TYPE_ISA EM_X86_64
#elif defined(__ISA_RISCV32__)
#define EXPECT_TYPE_ISA EM_RISCV
#elif defined(__ISA_RISCV64__)
#define EXPECT_TYPE_ISA EM_RISCV
#else
# error "Unsupported ISA"
#endif

extern size_t ramdisk_read(void *buf, size_t offset, size_t len);
extern size_t ramdisk_write(const void *buf, size_t offset, size_t len);

uintptr_t loader(PCB *pcb, const char *filename) {
  int fd = fs_open(filename, 0, 0);
  assert(fd >= 0); assert(pcb != NULL);
  Elf_Ehdr elf;
  fs_read(fd, &elf, sizeof(Elf_Ehdr));
  if(*(uint32_t *)elf.e_ident != 0x464c457f) {
    printf("Invalid ELF magic number: 0x%x\n", *(uint32_t *)elf.e_ident);
    panic("Invalid ELF file: %s", filename);
  }
  if(elf.e_machine != EXPECT_TYPE_ISA) {
  printf("Unsupported ISA in ELF file: e_machine = %d, expected = %d\n",
          elf.e_machine, EXPECT_TYPE_ISA);
  panic("Unsupported ISA in ELF file: %s", filename);
  }
  Elf_Phdr ph;
  for (int i = 0; i < elf.e_phnum; i ++) {
    fs_lseek(fd, elf.e_phoff + i * elf.e_phentsize, SEEK_SET);
    fs_read(fd, &ph, sizeof(Elf_Phdr));
    if (ph.p_type == PT_LOAD) {
      // 使用 uintptr_t 作为中间层
      fs_lseek(fd, ph.p_offset, SEEK_SET);
      fs_read(fd, (void *)ph.p_vaddr, ph.p_filesz);
      if (ph.p_memsz > ph.p_filesz) {
        memset((void *)(ph.p_vaddr + ph.p_filesz), 0, ph.p_memsz - ph.p_filesz);
      }
      if(pcb->max_brk < (uintptr_t)(ph.p_vaddr + ph.p_memsz)) {
        pcb->max_brk = (uintptr_t)(ph.p_vaddr + ph.p_memsz);
      }
    }
  }
  fs_close(fd);
  return elf.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  context_kload(pcb, (void (*)(void *))entry, NULL);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

