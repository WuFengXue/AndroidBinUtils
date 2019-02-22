/**
 * elf tag modifier, update dynamic section tag of DEBUG to NEEDED and set its value to SO_NAME_DST.
 *
 * @author Reinhard（李剑波）
 * @date 2019/02/22
 */
#include <stdio.h>
#include <jni.h>
#include <elf.h>
#include <malloc.h>
#include <memory.h>
#include <zconf.h>
#include <fcntl.h>

/**
 * character count limit of a dynstr
 */
#define DYNSTR_LEN 32
/**
 * so name to use (add string to dynstr table is difficult, so we just use string that already exist)
 */
#define SO_NAME_SRC "libandroid_runtime.so"
/**
 * new so name (simply truncate string)
 */
#define SO_NAME_DST "android_runtime.so"
/**
 * character count that are truncated
 */
#define SO_NAME_OFFSET 3

/**
 * whether elf file is 32 bit or not
 */
static int isElf32;
/**
 * elf header (32 bit)
 */
static Elf32_Ehdr ehdr32;
/**
 * elf header (64 bit)
 */
static Elf64_Ehdr ehdr64;
/**
 * section header of .dynamic (32 bit)
 */
static Elf32_Shdr shdrDynamic32;
/**
 * section header of .dynamic (64 bit)
 */
static Elf64_Shdr shdrDynamic64;
/**
 * section header of .dynstr (32 bit)
 */
static Elf32_Shdr shdrDynstr32;
/**
 * section header of .dynstr (64 bit)
 */
static Elf64_Shdr shdrDynstr64;
/**
 * dynamic table offset of tag DEBUG
 */
static int64_t dynTableOffset = -1;
/**
 * dynstr table offset of SO_NAME_SRC
 */
static int64_t dynstrTableOffset = -1;

/**
 * option - whether revert modification to DEBUG tag or not
 */
static int do_revert = JNI_FALSE;

static void usage(FILE *stream) {
    fprintf(stream, "usage: elftag [option] <elffile>\n");
    fprintf(stream,
            " modify dynamic section tag of DEBUG to NEEDED and set its value to %s\n",
            SO_NAME_DST);
    fprintf(stream, " Options are:\n");
    fprintf(stream, "  -r        Revert modification to DEBUG tag\n");
    fprintf(stream, "  -h        Display this information\n");
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wint-conversion"

/**
 * get elf header
 * @param handle file handle
 * @return 1 on success，0 otherwise
 */
static int get_file_header(FILE *handle) {
    unsigned char e_ident[EI_NIDENT];
    /* Read in the identity array.  */
    if (fread(e_ident, EI_NIDENT, 1, handle) != 1) {
        fprintf(stdout, "Fail to read elf header's e_ident field\n");
        return JNI_FALSE;
    }

    /* Determine how to read the header.  */
    switch (e_ident[EI_CLASS]) {
        case ELFCLASS32:
            fprintf(stdout, "The elf file is 32 bit\n");
            isElf32 = JNI_TRUE;
            fseek(handle, 0, SEEK_SET);
            if (fread(ehdr32.e_ident, sizeof(ehdr32), 1, handle) != 1) {
                fprintf(stdout, "Fail to read elf header\n");
                return JNI_FALSE;
            }
            break;
        case ELFCLASS64:
            fprintf(stdout, "The elf file is 64 bit\n");
            isElf32 = JNI_FALSE;
            fseek(handle, 0, SEEK_SET);
            if (fread(ehdr64.e_ident, sizeof(ehdr64), 1, handle) != 1) {
                fprintf(stdout, "Fail to read elf header\n");
                return JNI_FALSE;
            }
            break;
        default:
            fprintf(stdout, "Unknown elf file\n");
            return JNI_FALSE;
    }
    return JNI_TRUE;
}

#pragma clang diagnostic pop

/**
 * get section header of .dynamic and .dynstr
 * @param handle file handle
 * @return 1 on success, 0 otherwise
 */
static int get_32bit_section_headers(FILE *handle) {
    Elf32_Shdr shdr32;
    unsigned int size = ehdr32.e_shentsize;
    unsigned int num = ehdr32.e_shnum;
    int getDynamic = JNI_FALSE;
    int getDynstr = JNI_FALSE;

    if (size == 0 || num == 0) {
        fprintf(stdout, "Cope with unexpected section header sizes.\n");
        return JNI_FALSE;
    }
    if (size < sizeof shdr32) {
        fprintf(stdout,
                "The e_shentsize field in the ELF header is less than the size of an ELF section header\n");
        return JNI_FALSE;
    }
    if (size > sizeof shdr32) {
        fprintf(stdout,
                "The e_shentsize field in the ELF header is larger than the size of an ELF section header\n");
        return JNI_FALSE;
    }
    fseek(handle, ehdr32.e_shoff, SEEK_SET);
    for (int i = 0; i < ehdr32.e_shnum; i++) {
        if (fread(&shdr32, 1, size, handle) != size) {
            fprintf(stdout, "Read section header failed for entry %d\n", i);
            return JNI_FALSE;
        }
        switch (shdr32.sh_type) {
            case SHT_DYNAMIC:
                memcpy(&shdrDynamic32, &shdr32, sizeof(shdr32));
                getDynamic = JNI_TRUE;
                break;
            case SHT_STRTAB:
                if (shdr32.sh_flags == SHF_ALLOC) {
                    memcpy(&shdrDynstr32, &shdr32, sizeof(shdr32));
                    getDynstr = JNI_TRUE;
                }
                break;
            default:
                break;
        }
    }
    if (!getDynamic) {
        fprintf(stdout, "Cannot get section header of .dynamic\n");
        return JNI_FALSE;
    } else if (!getDynstr) {
        fprintf(stdout, "Cannot get section header of .dynstr\n");
        return JNI_FALSE;
    } else {
        return JNI_TRUE;
    }
}

/**
 * get section header of .dynamic and .dynstr
 * @param handle file handle
 * @return 1 on success, 0 otherwise
 */
static int get_64bit_section_headers(FILE *handle) {
    Elf64_Shdr shdr64;
    unsigned int size = ehdr64.e_shentsize;
    unsigned int num = ehdr64.e_shnum;
    int getDynamic = JNI_FALSE;
    int getDynstr = JNI_FALSE;

    if (size == 0 || num == 0) {
        fprintf(stdout, "Cope with unexpected section header sizes.\n");
        return JNI_FALSE;
    }
    if (size < sizeof shdr64) {
        fprintf(stdout,
                "The e_shentsize field in the ELF header is less than the size of an ELF section header\n");
        return JNI_FALSE;
    }
    if (size > sizeof shdr64) {
        fprintf(stdout,
                "The e_shentsize field in the ELF header is larger than the size of an ELF section header\n");
        return JNI_FALSE;
    }
    fseek(handle, ehdr64.e_shoff, SEEK_SET);
    for (int i = 0; i < ehdr64.e_shnum; i++) {
        if (fread(&shdr64, 1, size, handle) != size) {
            fprintf(stdout, "Read section header failed for entry %d\n", i);
            return JNI_FALSE;
        }
        switch (shdr64.sh_type) {
            case SHT_DYNAMIC:
                memcpy(&shdrDynamic64, &shdr64, sizeof(shdr64));
                getDynamic = JNI_TRUE;
                break;
            case SHT_STRTAB:
                if (shdr64.sh_flags == SHF_ALLOC) {
                    memcpy(&shdrDynstr64, &shdr64, sizeof(shdr64));
                    getDynstr = JNI_TRUE;
                }
                break;
            default:
                break;
        }
    }
    if (!getDynamic) {
        fprintf(stdout, "Cannot get section header of .dynamic\n");
        return JNI_FALSE;
    } else if (!getDynstr) {
        fprintf(stdout, "Cannot get section header of .dynstr\n");
        return JNI_FALSE;
    } else {
        return JNI_TRUE;
    }
}

/**
 * get dynamic section string for dynamic table entry dyn
 * @param file_name file name (full path)
 * @param dyn dynamic table entry
 * @param dynstr placeholder to store result dynamic section string
 */
static void get_32bit_dynamic_section_string(const char *file_name, Elf32_Dyn dyn, char *dynstr) {
    FILE *handle = fopen(file_name, "r");
    fseek(handle, shdrDynstr32.sh_offset + dyn.d_un.d_val, SEEK_SET);
    fgets(dynstr, DYNSTR_LEN, handle);
    fclose(handle);
}

/**
 * get dynamic section string for dynamic table entry dyn
 * @param file_name file name (full path)
 * @param dyn dynamic table entry
 * @param dynstr placeholder to store result dynamic section string
 */
static void get_64bit_dynamic_section_string(const char *file_name, Elf64_Dyn dyn, char *dynstr) {
    FILE *handle = fopen(file_name, "r");
    fseek(handle, shdrDynstr64.sh_offset + dyn.d_un.d_val, SEEK_SET);
    fgets(dynstr, DYNSTR_LEN, handle);
    fclose(handle);
}

/**
 * get dynamic table offset of tag DEBUG and dynstr table offset of tag NEEDED with SO_NAME_SRC
 * @param file_name file name (full path)
 * @param handle file handle
 * @return 1 on success, 0 otherwise
 */
static int get_32bit_dynamic_sections(const char *file_name, FILE *handle) {
    Elf32_Dyn dyn;
    char dynstr[DYNSTR_LEN];
    fseek(handle, shdrDynamic32.sh_offset, SEEK_SET);
    for (int i = 0; i < shdrDynamic32.sh_size; i += shdrDynamic32.sh_entsize) {
        fread(&dyn, 1, sizeof(dyn), handle);
        switch (dyn.d_tag) {
            case DT_NEEDED:
                // get so name
                get_32bit_dynamic_section_string(file_name, dyn, dynstr);
                if (!strcmp(dynstr, SO_NAME_SRC)) {
                    dynstrTableOffset = dyn.d_un.d_val;
                } else if (!strcmp(dynstr, SO_NAME_DST)) {
                    if (do_revert) {
                        dynTableOffset = i;
                    } else {
                        fprintf(stdout, "Already processed!\n");
                        return JNI_FALSE;
                    }
                }
                break;
            case DT_DEBUG:
                if (do_revert) {
                    fprintf(stdout, "No need to revert!\n");
                    return JNI_FALSE;
                } else {
                    dynTableOffset = i;
                }
                break;
            default:
                break;
        }
    }
    if (dynTableOffset < 0) {
        fprintf(stdout, "Cannot get tag DEBUG!\n");
        return JNI_FALSE;
    } else if (dynstrTableOffset < 0) {
        fprintf(stdout, "Cannot get tag NEEDED with %s!\n", SO_NAME_SRC);
        return JNI_FALSE;
    } else {
        return JNI_TRUE;
    }
}

/**
 * get dynamic table offset of tag DEBUG and dynstr table offset of tag NEEDED with SO_NAME_SRC
 * @param file_name file name (full path)
 * @param handle file handle
 * @return 1 on success, 0 otherwise
 */
static int get_64bit_dynamic_sections(const char *file_name, FILE *handle) {
    Elf64_Dyn dyn;
    char dynstr[DYNSTR_LEN];
    fseek(handle, shdrDynamic64.sh_offset, SEEK_SET);
    for (int i = 0; i < shdrDynamic64.sh_size; i += shdrDynamic64.sh_entsize) {
        fread(&dyn, 1, sizeof(dyn), handle);
        switch (dyn.d_tag) {
            case DT_NEEDED:
                // get so name
                get_64bit_dynamic_section_string(file_name, dyn, dynstr);
                if (!strcmp(dynstr, SO_NAME_SRC)) {
                    dynstrTableOffset = dyn.d_un.d_val;
                } else if (!strcmp(dynstr, SO_NAME_DST)) {
                    if (do_revert) {
                        dynTableOffset = i;
                    } else {
                        fprintf(stdout, "Already processed!\n");
                        return JNI_FALSE;
                    }
                }
                break;
            case DT_DEBUG:
                if (do_revert) {
                    fprintf(stdout, "No need to revert!\n");
                    return JNI_FALSE;
                } else {
                    dynTableOffset = i;
                }
                break;
            default:
                break;
        }
    }
    if (dynTableOffset < 0) {
        fprintf(stdout, "Cannot get tag DEBUG!\n");
        return JNI_FALSE;
    } else if (dynstrTableOffset < 0) {
        fprintf(stdout, "Cannot get tag NEEDED with %s!\n", SO_NAME_SRC);
        return JNI_FALSE;
    } else {
        return JNI_TRUE;
    }
}

/**
 * update dynamic section tag of DEBUG to NEEDED with SO_NAME_DST
 * @param file_name file name (full path)
 * @return 1 on success, 0 otherwise
 */
static int process_32bit_dyn_tag(const char *file_name) {
    int ret = JNI_TRUE;
    FILE *handle = fopen(file_name, "r+");
    fseek(handle, shdrDynamic32.sh_offset + dynTableOffset, SEEK_SET);
    size_t byteCount = shdrDynamic32.sh_entsize / 2;
    int buf = do_revert ? DT_DEBUG : DT_NEEDED;
    // write d_tag
    if (fwrite(&buf, 1, byteCount, handle) != byteCount) {
        fprintf(stdout, "Fail to write d_tag!\n");
        ret = JNI_FALSE;
    } else {
        buf = do_revert ? 0 : dynstrTableOffset + SO_NAME_OFFSET;
        // write d_val
        if (fwrite(&buf, 1, byteCount, handle) != byteCount) {
            fprintf(stdout, "Fail to write d_val!\n");
            ret = JNI_FALSE;
        }
    }
    fclose(handle);
    return ret;
}

/**
 * update dynamic section tag of DEBUG to NEEDED with SO_NAME_DST
 * @param file_name file name (full path)
 * @return 1 on success, 0 otherwise
 */
static int process_64bit_dyn_tag(const char *file_name) {
    int ret = JNI_TRUE;
    FILE *handle = fopen(file_name, "r+");
    fseek(handle, shdrDynamic64.sh_offset + dynTableOffset, SEEK_SET);
    size_t byteCount = shdrDynamic64.sh_entsize / 2;
    int64_t buf = do_revert ? DT_DEBUG : DT_NEEDED;
    // write d_tag
    if (fwrite(&buf, 1, byteCount, handle) != byteCount) {
        fprintf(stdout, "Fail to write d_tag!\n");
        ret = JNI_FALSE;
    } else {
        buf = do_revert ? 0 : dynstrTableOffset + SO_NAME_OFFSET;
        // write d_val
        if (fwrite(&buf, 1, byteCount, handle) != byteCount) {
            fprintf(stdout, "Fail to write d_val!\n");
            ret = JNI_FALSE;
        }
    }
    fclose(handle);
    return ret;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "android-cloexec-fopen"

static int process_file(const char *file_name) {
    int ret = JNI_FALSE;
    FILE *handle = fopen(file_name, "r");
    // get elf header
    if (get_file_header(handle)) {
        if (isElf32) {
            // get section header of .dynamic and .dynstr
            if (get_32bit_section_headers(handle)) {
                // get dynamic table offset of tag DEBUG and dynstr table offset of tag NEEDED with SO_NAME_SRC
                if (get_32bit_dynamic_sections(file_name, handle)) {
                    // update dynamic section tag of DEBUG
                    ret = process_32bit_dyn_tag(file_name);
                }
            }
        } else {
            // get section header of .dynamic and .dynstr
            if (get_64bit_section_headers(handle)) {
                // get dynamic table offset of tag DEBUG and dynstr table offset of tag NEEDED with SO_NAME_SRC
                if (get_64bit_dynamic_sections(file_name, handle)) {
                    // update dynamic section tag of DEBUG
                    ret = process_64bit_dyn_tag(file_name);
                }
            }
        }
    }
    fclose(handle);
    return ret;
}

#pragma clang diagnostic pop

static int parse_args(int argc, char **argv) {
    if (argc < 2) {
        return JNI_FALSE;
    }
    int ch;
    while ((ch = getopt(argc, argv, "rh")) != EOF) {
        switch (ch) {
            case 'r':
                do_revert = JNI_TRUE;
                fprintf(stdout, "Revert modification to DEBUG tag\n");
                break;
            case 'h':
                return JNI_FALSE;
            default:
                break;
        }
    }
    return JNI_TRUE;
}

int main(int argc, char **argv) {
    int ret = JNI_ERR;
    if (parse_args(argc, argv)) {
        if (process_file(argv[argc - 1])) {
            fprintf(stdout, "Success!\n");
            ret = JNI_OK;
        }
    } else {
        usage(stdout);
    }
    return ret;
}
