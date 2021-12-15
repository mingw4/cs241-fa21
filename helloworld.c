//author: mingw4
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
int main(int argc, char* argv[])
{
   
    if (strcmp(argv[0], "./encrypt") == 0) {
	if(argc != 3){
	     printf("Hello World");
	     return 0;
	}   
 
        int fp_r = open(argv[1], O_RDWR);
        
        if(fp_r == -1){
	    printf("Hellow World");
	    return 0;
	}
        struct stat statbuf;
        stat(argv[1], &statbuf);
        int length = statbuf.st_size;
    
        
       
        int fp_rd = open("/dev/urandom", O_RDONLY);
        char* rd_data = (char*)malloc(length);
        //char* r_buffer =(char*)malloc(length);
	char* addr = NULL;
        read(fp_rd,rd_data,length);
        addr =(char*) mmap(NULL, (size_t)length, PROT_READ|PROT_WRITE, MAP_SHARED, fp_r, 0);
       
        FILE* fp_w = fopen(argv[2], "wb+");
	if(fp_w == NULL){
	    printf("Hello World");
	    return 0;
	}
        for (size_t i = 0; i < length; i++) {
            fputc(rd_data[i], fp_w);
        }
 
        
        for (size_t i = 0; i < length; i++) {
            int en_byte = rd_data[i]^addr[i];
            fputc(en_byte, fp_w);            
            addr[i] = 0xff;
        }
	msync((void*) addr, length, MS_SYNC);
	munmap(addr, length);
	free(rd_data);
        close(fp_rd);
        close(fp_r);
        fclose(fp_w);
    }
    else if (strcmp(argv[0], "./decrypt") == 0) {
	if(argc != 3){
	    printf("Hello World");
	    return 0;
	}
	int fp = open(argv[1], O_RDWR);
	if(fp == -1){
	    printf("Hell0 World");
	    return 0;
	}

	struct stat statbuf;
        stat(argv[1], &statbuf);
        int len = statbuf.st_size;


        char* data = NULL;
        
        data = (char*) mmap(NULL, (size_t)len, PROT_READ|PROT_WRITE, MAP_SHARED, fp, 0);

	
        FILE * fp_w_2 = fopen(argv[2], "wb+");
	if(fp_w_2 == NULL){
	    printf("Hello World");
	    return 0;
	}
        for (size_t i = 0; i < len/2; i++) {
            int de_byte = data[i]^data[len/2+i];
            fputc(de_byte, fp_w_2);
        }
        close(fp);
        unlink(argv[1]);
        fclose(fp_w_2);
	return 0;
    }else  {
	printf("Hello World");
	return 0;}

    return 0;
}
