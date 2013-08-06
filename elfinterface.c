#include <assert.h>
#include <elf.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

struct file {
    char* mem;
    off_t size;
};

static void openfile(struct file* f, const char* fn) {
    int fd;
    struct stat st;
    int res;
    
    fd = open(fn, O_RDWR);
    assert(fd != -1);
    res = fstat(fd, &st);
    assert(res != -1);
    f->size = st.st_size;
    f->mem = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert(f->mem != MAP_FAILED);
    res = close(fd);
    assert(res != -1);
}

static void closefile(struct file* f) {
    int res;
    res = munmap(f->mem, f->size);
    assert(res != -1);
}

static void parsesymbols(Elf32_Sym* symtab, Elf32_Word* symnum,
			 Elf32_Versym* vertab, Elf32_Word* vernum) {
    unsigned int c = 1;
    unsigned int d = 1;
    assert(*symnum / sizeof(*symtab) == *vernum / sizeof(*vertab));
    while (c < *symnum / sizeof(*symtab)) {
	if (symtab[c].st_shndx != SHN_UNDEF) {
	    memmove(symtab + d, symtab + c, sizeof(*symtab));
	    memmove(vertab + d, vertab + c, sizeof(*vertab));
	    ++d;
	}
	++c;
    }
    *symnum = d * sizeof(*symtab);
    *vernum = d * sizeof(*vertab);
    printf("%d/%d dynamic symbols stripped.\n", (c - d), c);
}

static void parseshdrs(void* mem, Elf32_Shdr* tab, int num) {
    int c;
    Elf32_Sym* symtab;
    Elf32_Word* symnum;
    int symtabfound = 0;
    Elf32_Versym* vertab;
    Elf32_Word* vernum;
    int vertabfound = 0;
    for (c = 0; c < num; ++c) {
	switch (tab[c].sh_type) {
	case SHT_DYNSYM:
	    symtab = mem + tab[c].sh_offset;
	    symnum = &(tab[c].sh_size);
	    assert(tab[c].sh_entsize == sizeof(Elf32_Sym));
	    ++symtabfound;
	    break;
	case SHT_GNU_versym:
	    vertab = mem + tab[c].sh_offset;
	    vernum = &(tab[c].sh_size);
	    assert(tab[c].sh_entsize == sizeof(Elf32_Versym));
	    ++vertabfound;
	    break;
	}
    }
    assert(symtabfound == 1);
    assert(vertabfound == 1);
    parsesymbols(symtab, symnum, vertab, vernum);
}

static void parseobject(void* mem) {
    Elf32_Ehdr* hdr;

    hdr = (Elf32_Ehdr*)(mem);
    assert(hdr->e_ident[EI_MAG0] == ELFMAG0);
    assert(hdr->e_ident[EI_MAG1] == ELFMAG1);
    assert(hdr->e_ident[EI_MAG2] == ELFMAG2);
    assert(hdr->e_ident[EI_MAG3] == ELFMAG3);
    assert(hdr->e_ident[EI_CLASS] == ELFCLASS32);
    assert(hdr->e_ident[EI_DATA] == ELFDATA2LSB);
    assert(hdr->e_ident[EI_VERSION] == EV_CURRENT);
    assert(hdr->e_ident[EI_OSABI] == ELFOSABI_NONE);
    assert(hdr->e_ident[EI_ABIVERSION] == 0);
    assert(hdr->e_ident[EI_PAD + 0] == 0);
    assert(hdr->e_ident[EI_PAD + 1] == 0);
    assert(hdr->e_ident[EI_PAD + 2] == 0);
    assert(hdr->e_type == ET_DYN);
    /*assert(hdr->e_machine == EM_MIPS);*/
    assert(hdr->e_version == EV_CURRENT);
    assert(hdr->e_shentsize == sizeof(Elf32_Shdr));
    parseshdrs(mem, mem + hdr->e_shoff, hdr->e_shnum);
}

int main(int argc, char** argv) {
    struct file f;
    assert(argc == 2);
    openfile(&f, argv[1]);
    parseobject(f.mem);
    closefile(&f);
    return 0;
}

/* Local Variables:  */
/* mode: c           */
/* c-basic-offset: 4 */
/* End:              */
