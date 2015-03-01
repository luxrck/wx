#ifndef __ELF_H__
#define __ELF_H__

#define ELF_MAGIC 0x464C457FU	/* "\x7FELF" in little endian */

struct elf {
	uint32_t magic;	// must equal ELF_MAGIC
	uint8_t elf[12];
	uint16_t type;
	uint16_t machine;
	uint32_t version;
	uint32_t entry;
	uint32_t phoff;
	uint32_t shoff;
	uint32_t flags;
	uint16_t ehsize;
	uint16_t phentsize;
	uint16_t phnum;
	uint16_t shentsize;
	uint16_t shnum;
	uint16_t shstrndx;
};

struct proghdr {
	uint32_t type;
	uint32_t offset;
	uint32_t va;
	uint32_t pa;
	uint32_t filesz;
	uint32_t memsz;
	uint32_t flags;
	uint32_t align;
};

struct secthdr {
	uint32_t name;
	uint32_t type;
	uint32_t flags;
	uint32_t addr;
	uint32_t offset;
	uint32_t size;
	uint32_t link;
	uint32_t info;
	uint32_t addralign;
	uint32_t entsize;
};

// Values for proghdr::type
#define ELF_PROG_LOAD		1

// Flag bits for proghdr::flags
#define ELF_PROG_FLAG_EXEC	1
#define ELF_PROG_FLAG_WRITE	2
#define ELF_PROG_FLAG_READ	4

// Values for secthdr::type
#define ELF_SHT_NULL		0
#define ELF_SHT_PROGBITS	1
#define ELF_SHT_SYMTAB		2
#define ELF_SHT_STRTAB		3

// Values for secthdr::name
#define ELF_SHN_UNDEF		0

// Flag bits for secthdr::flags
#define ELF_SHF_WRITE		1
#define ELF_SHF_ALLOC		2

#endif /* !__ELF_H__ */
