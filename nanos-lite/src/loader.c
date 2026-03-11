#include <proc.h>
#include <elf.h>

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
  Elf_Ehdr elf;
  ramdisk_read(&elf, 0, sizeof(elf));
  assert(elf.e_ident[0] == 0x7f && elf.e_ident[1] == 'E'\
     && elf.e_ident[2] == 'L' && elf.e_ident[3] == 'F');
  assert(elf.e_machine == EXPECT_TYPE_ISA);
  Elf_Phdr ph;
  for (int i = 0; i < elf.e_phnum; i ++) {
    ramdisk_read(&ph, elf.e_phoff + i * elf.e_phentsize, sizeof(ph));
    if (ph.p_type == PT_LOAD) {
      // 使用 uintptr_t 作为中间层
      uintptr_t vaddr = (uintptr_t)ph.p_vaddr;
      ramdisk_read((void *)vaddr, ph.p_offset, ph.p_filesz);
      if (ph.p_memsz > ph.p_filesz) {
        memset((void *)(vaddr + ph.p_filesz), 0, ph.p_memsz - ph.p_filesz);
      }
    }
  }
  return elf.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

