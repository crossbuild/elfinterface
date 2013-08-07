#include <assert.h>
#include <elf.h>
#include <fcntl.h>
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

static void parsesymbols(Elf32_Sym* symtab, Elf32_Word symnum) {
    unsigned int c;
    for (c = 1; c < symnum; ++c)
	if ((symtab[c].st_shndx == SHN_UNDEF) &&
	    (ELF32_ST_BIND(symtab[c].st_info) == STB_GLOBAL))
	    symtab[c].st_info =
		ELF64_ST_INFO(STB_WEAK, ELF32_ST_TYPE(symtab[c].st_info));
}

static void parseshdrs(void* mem, Elf32_Shdr* tab, int num) {
    int c;
    for (c = 0; c < num; ++c)
	if (tab[c].sh_type == SHT_DYNSYM) {
	    assert(tab[c].sh_entsize == sizeof(Elf32_Sym));
	    parsesymbols(mem + tab[c].sh_offset, tab[c].sh_size / sizeof(Elf32_Sym));
	}
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
