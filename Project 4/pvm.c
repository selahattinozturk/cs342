#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

#define PAGE_SIZE 4096
#define ENTRY_SIZE 8
void print_flags(uint64_t PFN){
    int fd = open("/proc/kpageflags", O_RDONLY);

    unsigned long offset = PFN * ENTRY_SIZE;

    lseek(fd, offset, SEEK_SET);

    unsigned long flags;
    read(fd, &flags, 8);

    printf("Frame Number: %ld (0x%lx)\n", PFN, PFN);
    close(fd);
    printf("Page Flags: \n");
    printf("  LOCKED: %ld\n", (flags >> 0) & 1);
    printf("  ERROR: %ld\n", (flags >> 1) & 1);
    printf("  REFERENCED: %ld\n", (flags >> 2) & 1);
    printf("  UPTODATE: %ld\n", (flags >> 3) & 1);
    printf("  DIRTY: %ld\n", (flags >> 4) & 1);
    printf("  LRU: %ld\n", (flags >> 5) & 1);
    printf("  ACTIVE: %ld\n", (flags >> 6) & 1);
    printf("  SLAB: %ld\n", (flags >> 7) & 1);
    printf("  WRITEBACK: %ld\n", (flags >> 8) & 1);
    printf("  RECLAIM: %ld\n", (flags >> 9) & 1);
    printf("  BUDDY: %ld\n", (flags >> 10) & 1);
    printf("  MMAP: %ld\n", (flags >> 11) & 1);
    printf("  ANON: %ld\n", (flags >> 12) & 1);
    printf("  SWAPCACHE: %ld\n", (flags >> 13) & 1);
    printf("  SWAPBACKED: %ld\n", (flags >> 14) & 1);
    printf("  COMPOUND_HEAD: %ld\n", (flags >> 15) & 1);
    printf("  COMPOUND_TAIL: %ld\n", (flags >> 16) & 1);
    printf("  HUGE: %ld\n", (flags >> 17) & 1);
    printf("  UNEVICTABLE: %ld\n", (flags >> 18) & 1);
    printf("  HWPOISON: %ld\n", (flags >> 19) & 1);
    printf("  NOPAGE: %ld\n", (flags >> 20) & 1);
    printf("  KSM: %ld\n", (flags >> 21) & 1);
    printf("  THP: %ld\n", (flags >> 22) & 1);
    printf("  BALLOON: %ld\n", (flags >> 23) & 1);
    printf("  ZERO_PAGE: %ld\n", (flags >> 24) & 1);
    printf("  IDLE: %ld\n", (flags >> 25) & 1);
}

void print_frame_info(unsigned long frame_number){

    int fd = open("/proc/kpagecount", O_RDONLY);

    unsigned long offset = frame_number * ENTRY_SIZE;

    lseek(fd, offset, SEEK_SET);

    unsigned long flags;
    read(fd, &flags, 8);
    print_flags(frame_number);
    printf("  Count: %ld\n", flags );
    close(fd);
}

void print_memory_usage(int pid){
    // maps
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "/proc/%d/maps", pid);
    FILE *file = fopen(file_path, "rb");
    if (file == NULL){
        perror("Failed to open /proc/pid/maps");
        return;
    }
    // pagemap
    char file_path2[256];
    snprintf(file_path2, sizeof(file_path2), "/proc/%d/pagemap", pid);
    int file2 = open(file_path2, O_RDONLY);

    int total_virtual_memory = 0;
    int inc = 0;
    int ex = 0;
    char line[256];
    while (fgets(line, sizeof(line), file)){
        unsigned long start_vaddr, end_vaddr;
        sscanf(line, "%lx-%lx", &start_vaddr, &end_vaddr);

        total_virtual_memory += (end_vaddr - start_vaddr);

        unsigned long pageStart = start_vaddr / PAGE_SIZE;
        unsigned long pageEnd = end_vaddr / PAGE_SIZE;
        // printf("val:%lx \n", ((start_vaddr)>>12));
        int fd = open("/proc/kpagecount", O_RDONLY);
        for (unsigned long i = pageStart; i < pageEnd; i = i + 1){
            // printf("i is %lx\n", i);
            unsigned long vToP;
            lseek(file2, i * 8, SEEK_SET);
            read(file2, &vToP, 8);
            // printf("pagemap val: %lx\n", vToP);
            unsigned long shift = 1;
            if ((vToP & (shift << 63)) != 0){
                if (i < 0x007FFFFFFFF){
                    unsigned long PFN = (vToP & 0x007FFFFFFFFFF);
                    lseek(fd, (PFN)*8, SEEK_SET);
                    unsigned long flags;
                    read(fd, &flags, 8);
                    // printf("count: 0x%lx\n\n", flags);

                    if (flags > 1){
                        ex++;
                    }
                    else
                    {
                        inc++;
                        ex++;
                    }
                }
            }
        }
        close(fd);
    }
    fclose(file);
    close(file2);
    printf("Total Virtual Memory Used: %d KB\n", total_virtual_memory / 1024);
    printf("Total Physical Memory Used (Exclusive): %d KB\n", ex * 4);
    printf("Total Physical Memory Used (Inclusive): %d KB\n", inc * 4);
}

void print_all_mapping(int pid){
    // maps
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "/proc/%d/maps", pid);
    FILE *file = fopen(file_path, "rb");
    if (file == NULL){
        perror("Failed to open /proc/pid/maps");
        return;
    }
    // pagemap
    char file_path2[256];
    snprintf(file_path2, sizeof(file_path2), "/proc/%d/pagemap", pid);
    int file2 = open(file_path2, O_RDONLY);

    char line[256];

    while (fgets(line, sizeof(line), file)){
        unsigned long start_vaddr, end_vaddr;
        sscanf(line, "%lx-%lx", &start_vaddr, &end_vaddr);

        unsigned long pageStart = start_vaddr / PAGE_SIZE;
        unsigned long pageEnd = end_vaddr / PAGE_SIZE;
        // printf("val:%lx \n", ((start_vaddr)>>12));
        for (unsigned long i = pageStart; i < pageEnd ; i = i + 1){
            
            printf("page number %ld (0x%lx)\t", i,i);
            unsigned long vToP;
            lseek(file2, i * 8, SEEK_SET);
            read(file2, &vToP, 8);
            // printf("pagemap val: %lx\n", vToP);
            unsigned long shift = 1;
            if ((vToP & (shift << 63)) != 0){
                if (i < 0x000007FFFFFFFFFF){
                    unsigned long PFN = (vToP & 0x007FFFFFFFFFFFFF);
                    printf("mapped to %ld (0x%lx)\n", PFN,PFN);
                }
            }
            else
            {
                printf("not-in-memory\n");
            }
        }
    }
    fclose(file);
    close(file2);
}
void fill_print(unsigned long a, unsigned long b){
    for (unsigned long i = a; i < b; i = i + 1){
        printf("page 0x0000%.12lx\tunused\n", i);
    }
}

void print_map_range(int pid, unsigned long VA1, unsigned long VA2){//not used
    VA1=VA1/PAGE_SIZE;
    VA2=VA2/PAGE_SIZE;
    // maps
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "/proc/%d/maps", pid);
    FILE *file = fopen(file_path, "rb");
    if (file == NULL){
        perror("Failed to open /proc/pid/maps");
        return;
    }
    // pagemap
    char file_path2[256];
    snprintf(file_path2, sizeof(file_path2), "/proc/%d/pagemap", pid);
    int file2 = open(file_path2, O_RDONLY);

    char line[256];
    unsigned long start[10000];
    unsigned long finish[10000];
    int count = 0;
    while (fgets(line, sizeof(line), file)&& count < 10000){
        unsigned long start_vaddr, end_vaddr;
        sscanf(line, "%lx-%lx", &start_vaddr, &end_vaddr);
        start[count] = start_vaddr/PAGE_SIZE;
        finish[count] = end_vaddr/PAGE_SIZE;
        count++;
    }

    unsigned long oldfinish = VA1;
    unsigned long oldstart = VA1;
    for (int j = 0; j < count - 1; j++){
        if (start[j] > VA1 && finish[j] < VA2){// normal printing
                fill_print(oldfinish, start[j]);
                for (unsigned long i = start[j]; i < finish[j]; i = i + 1){
                    printf("page number is 0x%lx\t", i);
                    unsigned long vToP;
                    lseek(file2, i * 8, SEEK_SET);
                    read(file2, &vToP, 8);
                    // printf("pagemap val: %lx\n", vToP);
                    unsigned long shift = 1;
                    if ((vToP & (shift << 63)) != 0){
                        if (i < 0x007FFFFFFFFFFFFF){
                            unsigned long PFN = (vToP & 0x007FFFFFFFFFFFFF);
                            printf("mapped to %ld (0x%lx)\n", PFN,PFN);
                        }
                    }
                    else
                    {
                        printf("not-in-memory\n");
                    }
                    
            }
            oldfinish = finish[j];
            oldstart = start[j];
        }
        if (start[j] < VA1 && finish[j] > VA1&& finish[j] <= VA2){ // start is not in range

            for (unsigned long i = VA1; i < finish[j]; i = i + 1){
                printf("page number is 0x%lx\t", i);
                unsigned long vToP;
                lseek(file2, i * 8, SEEK_SET);
                read(file2, &vToP, 8);
                // printf("pagemap val: %lx\n", vToP);
                unsigned long shift = 1;
                if ((vToP & (shift << 63)) != 0){
                    if (i < 0x007FFFFFFFFFFFFF){
                        unsigned long PFN = (vToP & 0x007FFFFFFFFFFFFF);
                        printf("mapped to %ld (0x%lx)\n", PFN,PFN);
                    }
                }
                else
                {
                    printf("not-in-memory\n");
                }
            }
            oldfinish = finish[j];
            oldstart = VA1;
        }
        if (start[j] > VA1 && start[j] < VA2 && finish[j] > VA2){ // finish is not in range
            fill_print(oldfinish, start[j]);
            for (unsigned long i = start[j]; i < VA2; i = i + 1){
                printf("page number is 0x%lx\t", i);
                unsigned long vToP;
                lseek(file2, i * 8, SEEK_SET);
                read(file2, &vToP, 8);
                // printf("pagemap val: %lx\n", vToP);
                unsigned long shift = 1;
                if ((vToP & (shift << 63)) != 0){
                    if (i < 0x007FFFFFFFFFFFFF){
                        unsigned long PFN = (vToP & 0x007FFFFFFFFFFFFF);
                       printf("mapped to %ld (0x%lx)\n", PFN,PFN);
                    }
                }
                else
                {
                    printf("not-in-memory\n");
                }
            }
            oldfinish = VA2;
            oldstart = start[j];
        }
    }

    fclose(file);
    close(file2);
}

void print_map_range2(int pid, unsigned long VA1, unsigned long VA2){
// maps
    VA1 = VA1/PAGE_SIZE;
    VA2 = VA2/PAGE_SIZE;
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "/proc/%d/maps", pid);
    FILE *file = fopen(file_path, "rb");
    if (file == NULL){
        perror("Failed to open /proc/pid/maps");
        return;
    }
    // pagemap
    char file_path2[256];
    snprintf(file_path2, sizeof(file_path2), "/proc/%d/pagemap", pid);
    int file2 = open(file_path2, O_RDONLY);

    char line[256];

    while (fgets(line, sizeof(line), file)){
        unsigned long start_vaddr, end_vaddr;
        sscanf(line, "%lx-%lx", &start_vaddr, &end_vaddr);

        unsigned long pageStart = start_vaddr / PAGE_SIZE;
        unsigned long pageEnd = end_vaddr / PAGE_SIZE;
        // printf("val:%lx \n", ((start_vaddr)>>12));
        for (unsigned long i = pageStart; i < pageEnd ; i = i + 1){
            if(i <VA2 && i > VA1){
                printf("page number %ld (0x%lx)\t", i,i);}
            unsigned long vToP;
            lseek(file2, i * 8, SEEK_SET);
            read(file2, &vToP, 8);
            // printf("pagemap val: %lx\n", vToP);
            unsigned long shift = 1;
            if ((vToP & (shift << 63)) != 0){
                if (i < 0x000007FFFFFFFFFF){
                    unsigned long PFN = (vToP & 0x007FFFFFFFFFFFFF);
                    if(i <VA2 && i > VA1)
                        printf("mapped to %ld (0x%lx)\n", PFN,PFN);
                }
            }
            else
            {
                if(i <VA2 && i > VA1)
                    printf("not-in-memory\n");
            }
        }
    }
    fclose(file);
    close(file2);
}
void print_all_mapping_in(int pid){
    // maps
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "/proc/%d/maps", pid);
    FILE *file = fopen(file_path, "rb");
    if (file == NULL){
        perror("Failed to open /proc/pid/maps");
        return;
    }
    // pagemap
    char file_path2[256];
    snprintf(file_path2, sizeof(file_path2), "/proc/%d/pagemap", pid);
    int file2 = open(file_path2, O_RDONLY);

    char line[256];
    while (fgets(line, sizeof(line), file)){
        unsigned long start_vaddr, end_vaddr;
        sscanf(line, "%lx-%lx", &start_vaddr, &end_vaddr);

        unsigned long pageStart = start_vaddr / PAGE_SIZE;
        unsigned long pageEnd = end_vaddr / PAGE_SIZE;
        // printf("val:%lx \n", ((start_vaddr)>>12));
        int fd = open("/proc/kpagecount", O_RDONLY);
        for (unsigned long i = pageStart; i < pageEnd; i = i + 1){

            unsigned long vToP;
            lseek(file2, i * 8, SEEK_SET);
            read(file2, &vToP, 8);
            // printf("pagemap val: %lx\n", vToP);
            unsigned long shift = 1;
            if ((vToP & (shift << 63)) != 0){
                if (i < 0x007FFFFFFFFFFFFF){
                    unsigned long PFN = (vToP & 0x007FFFFFFFFFFFFF);
                    printf("page number is %lx\t", i);
                    printf("mapped to 0x0000%.12lx\n", PFN);
                }
            }
        }
        close(fd);
    }
    fclose(file);
    close(file2);
}

void print_all_table_size(int pid){
    // maps
    char file_path[256];

    snprintf(file_path, sizeof(file_path), "/proc/%d/maps", pid);
    FILE *file = fopen(file_path, "rb");
    if (file == NULL){
        perror("Failed to open /proc/pid/maps");
        return;
    }
    // pagemap
    char file_path2[256];
    snprintf(file_path2, sizeof(file_path2), "/proc/%d/pagemap", pid);
    int file2 = open(file_path2, O_RDONLY);

    char line[256];
    while (fgets(line, sizeof(line), file)){
        unsigned long start_vaddr, end_vaddr;
        sscanf(line, "%lx-%lx", &start_vaddr, &end_vaddr);

        unsigned long pageStart = start_vaddr / PAGE_SIZE;
        unsigned long pageEnd = end_vaddr / PAGE_SIZE;
        // printf("val:%lx \n", ((start_vaddr)>>12));
        int fd = open("/proc/kpagecount", O_RDONLY);
        int count = 0;
        for (unsigned long i = pageStart; i < pageEnd; i = i + 1){
            unsigned long vToP;
            lseek(file2, i * 8, SEEK_SET);
            read(file2, &vToP, 8);
            // printf("pagemap val: %lx\n", vToP);
            unsigned long shift = 1;
            if ((vToP & (shift << 63)) != 0){
                if (i < 0x007FFFFFFFF){
                    unsigned long PFN = (vToP & 0x007FFFFFFFFFF);
                    printf("page number is %lx\t", i);
                    printf("mapped to 0x0000%.12lx\n", PFN);
                }
            }
        }
        close(fd);
    }
    fclose(file);
    close(file2);
}

void print_mapping_va(int pid, unsigned long VA){
    // pagemap
    char file_path2[256];
    snprintf(file_path2, sizeof(file_path2), "/proc/%d/pagemap", pid);
    int file2 = open(file_path2, O_RDONLY);

    unsigned long vToP;
    lseek(file2, (VA>>12) * 8, SEEK_SET);
    read(file2, &vToP, 8);
    vToP = (vToP<<12) | (VA & 0xFFF);
    printf("physical address in hex: 0x0000%.12lx\n", vToP);

    close(file2);
}
void print_pte(int pid, unsigned long va){
    // pagemap
    char file_path2[256];
    snprintf(file_path2, sizeof(file_path2), "/proc/%d/pagemap", pid);
    int file2 = open(file_path2, O_RDONLY);

    unsigned long vToP;
    lseek(file2, (va >> 12) * 8, SEEK_SET);
    read(file2, &vToP, 8);
    unsigned long shift = 1;
    unsigned long realaddress = (vToP<<12) | (va & 0xFFF);

    printf("page present: 0x0000%.12lx\n", (vToP >> 63));
    printf("page swapped: 0x0000%.12lx\n", ((vToP << 1) >> 63));
    printf("file page: 0x0000%.12lx\n", ((vToP << 2) >> 63));
    printf("PTE is soft-dirty: 0x0000%.12lx\n", ((vToP << 8) >> 63));
     printf("physical address in hex: 0x0000%.12lx\n", realaddress);
    if ((vToP & (shift << 63)) != 0){
        printf("physical frame number: 0x0000%.12lx\n", (vToP & 0x007FFFFFFFFFF));
    }
    else if ((vToP & (shift << 62)) != 0){
        printf("swap offset: 0x0000%.12lx\n", (vToP & 0x00000000001F));
    }

    close(file2);
}

int main(int argc, char *argv[]){

    char *command = argv[1];
    if (strcmp(command, "-frameinfo") == 0){
        if (argc != 3){
            printf("Usage: %s -frameinfo <PFN>\n", argv[0]);
            return 1;
        }

        uint64_t PFN = strtoul(argv[2], NULL, 0);
        print_frame_info(PFN);
    }
    else if (strcmp(command, "-memused") == 0){
        if (argc != 3){
            printf("Usage: %s -meminfo <PID>\n", argv[0]);
            return 1;
        }

        int PID = atoi(argv[2]);
        print_memory_usage(PID);
    }
    else if (strcmp(command, "-mapall") == 0){
        if (argc != 3){
            printf("Usage: %s -mapall <PID>\n", argv[0]);
            return 1;
        }

        int PID = atoi(argv[2]);
        print_all_mapping(PID);
    }
    else if (strcmp(command, "-mapallin") == 0){
        if (argc != 3){
            printf("Usage: %s -mapallin <PID>\n", argv[0]);
            return 1;
        }

        int PID = atoi(argv[2]);
        print_all_mapping_in(PID);
    }
    else if (strcmp(command, "-alltablesize") == 0){
        if (argc != 3){
            printf("Usage: %s -alltablesize <PID>\n", argv[0]);
            return 1;
        }

        int PID = atoi(argv[2]);
        print_all_table_size(PID);
    }
    else if (strcmp(command, "-mapva") == 0){
        if (argc != 4){
            printf("Usage: %s -mapva <PID> <VA>\n", argv[0]);
            return 1;
        }

        int PID = atoi(argv[2]);
        uint64_t VA = strtoul(argv[3], NULL, 0);
        print_mapping_va(PID, VA);
    }
    else if (strcmp(command, "-maprange") == 0){
        if (argc != 5){
            printf("Usage: %s -maprange <PID> <VA1> <VA2>\n", argv[0]);
            return 1;
        }

        int PID = atoi(argv[2]);
        uint64_t VA1 = strtoul(argv[3], NULL, 0);
        uint64_t VA2 = strtoul(argv[4], NULL, 0);
        print_map_range2(PID, VA1, VA2);
    }
    else if (strcmp(command, "-pte") == 0){
        if (argc != 4){
            printf("Usage: %s -pte <PID> <VA>\n", argv[0]);
            return 1;
        }

        int PID = atoi(argv[2]);
        uint64_t VA = strtoul(argv[3], NULL, 0);
        print_pte(PID, VA);
    }
    else
    {
        printf("Invalid command: %s\n", command);
        return 1;
    }

    return 0;
}
