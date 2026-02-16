#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include <assert.h>
#include "sdb.h"

typedef struct {
    Elf32_Sym *symtab;
    int symtab_count;
    char *shstrtab;
    char *strtab; // 新增
} ELFInfo;

ELFInfo elf_info;

void init_elf(const char *filename) {
    CHECK_AND_RETURN(filename == NULL, "ELF filename is NULL, skipping ELF initialization");
    FILE *file = fopen(filename, "rb");
    CHECK_AND_RETURN(file == NULL, "Failed to open ELF file: %s, skipping ELF initialization", filename);
    Log("Loading ELF file: %s", filename);

    // 读取ELF头
    Elf32_Ehdr ehdr;
    int read_bytes = 0;
    read_bytes = fread(&ehdr, sizeof(Elf32_Ehdr), 1, file);
    Assert(read_bytes == 1, "Failed to read ELF header: %s", filename);
    Assert(ehdr.e_ident[0] == 0x7f && ehdr.e_ident[1] == 'E' && ehdr.e_ident[2] == 'L' && ehdr.e_ident[3] == 'F', 
        "Invalid ELF file: %s", filename);

    // 读取节区头表
    Elf32_Shdr shdrs[ehdr.e_shnum];
    fseek(file, ehdr.e_shoff, SEEK_SET);
    read_bytes = fread(shdrs, sizeof(Elf32_Shdr), ehdr.e_shnum, file);
    Assert(read_bytes == ehdr.e_shnum, "Failed to read section headers: %s", filename);

    // 读取节区名称字符串表
    elf_info.shstrtab = malloc(shdrs[ehdr.e_shstrndx].sh_size);
    Assert(elf_info.shstrtab != NULL , "Failed to allocate memory for section header string table: %s", filename);
    fseek(file, shdrs[ehdr.e_shstrndx].sh_offset, SEEK_SET);
    read_bytes = fread(elf_info.shstrtab, shdrs[ehdr.e_shstrndx].sh_size, 1, file);
    Assert(read_bytes == 1, "Failed to read section header string table: %s", filename);

    // 查找符号表节区
    for (int i = 0; i < ehdr.e_shnum; i++) {
        if (shdrs[i].sh_type == SHT_SYMTAB) {
            elf_info.symtab_count = shdrs[i].sh_size / sizeof(Elf32_Sym);
            elf_info.symtab = malloc(shdrs[i].sh_size);
            Assert(elf_info.symtab != NULL , "Failed to allocate memory for symbol table: %s", filename);
            fseek(file, shdrs[i].sh_offset, SEEK_SET);
            read_bytes = fread(elf_info.symtab, shdrs[i].sh_size, 1, file);
            Assert(read_bytes == 1, "Failed to read symbol table: %s", filename); 
        }
        if (shdrs[i].sh_type == SHT_STRTAB) {
            if (i == ehdr.e_shstrndx) continue;
            elf_info.strtab = malloc(shdrs[i].sh_size);
            Assert(elf_info.strtab != NULL, "Failed to allocate memory for symbol string table: %s", filename);
            fseek(file, shdrs[i].sh_offset, SEEK_SET);
            read_bytes = fread(elf_info.strtab, shdrs[i].sh_size, 1, file);
            Assert(read_bytes == 1, "Failed to read symbol string table: %s", filename);
        }
    }

    fclose(file);
}

const char* get_symbol_name(paddr_t addr) {
    Assert(elf_info.symtab != NULL, "Symbol table not initialized!");
    Assert(elf_info.strtab != NULL, "Symbol string table not initialized!");
    for (int i = 0; i < elf_info.symtab_count; i++) {
        Elf32_Sym *sym = elf_info.symtab + i;
        if (sym->st_value <= addr && addr < sym->st_value + sym->st_size) {
            return elf_info.strtab + sym->st_name;
        }
    }
    return NULL;
}
