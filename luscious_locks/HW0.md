CH1

Q1.

#include <unistd.h>
int main() {
	write(1, "Hi! My name is Ming Wang.", 25);
	return 0;
}



Q2.

#include <unistd.h>
	
void write_triangle(int n) {
	for (int i = 1; i <= n; i++) {
		int count = 1;
		for (int j = 1, j <= count; j++) {
			write (2, "*", 1);
		}
		write (2, "\n", 1);
	}
}


Q3.

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>


int main() {
	mode_t mode = S_IRUSR | S_IWUSR;
	int fildes = open("hello_world.txt", O_CREAT | O_TRUNC | O_RDWR, mode);
	write(fildes, "Hi! My name is Ming Wang.", 25);
	close(fildes);
	return 0;
}

Q4.
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>


int main() {
	mode_t mode = S_IRUSR | S_IWUSR;
	close(1);
	int fildes = open("hello_world.txt", O_CREAT | O_TRUNC | O_RDWR, mode);
	printf("Hi! My name is Ming Wang.\n");
	close(fildes);
	open(1);
	return 0;
}

Q5.

The differences between write() and printf() are as follows:

	1. printf() allows system calls and lets callers to write data in various formatting.
	2. write() is designed to write strings/sequences of bytes solely.

CH2

Q1. There are at least 8 bits in a byte.

Q2. There are 1 byte in a char.

Q3. sizeof(int) is 4 bytes.
       sizeof(double) is 8 bytes.
       sizeof(float) is 4 bytes.
       sizeof(long) is 4 bytes.
       size of (long long) is 8 bytes.

Q4. The address of data+2 is 0x7fbd9d50.

Q5. data[3] is equivalent to *(data+3) in C.

Q6. This SEGFAULT because the string constant "hello" can only be read, while the line: "*ptr = 'j';" tries to change the constant memory, i.e. string in C is not changeable.

Q7. The value of the variable str_size is 12.

Q8. The value of the variable str_len is 5.

Q9. When X is the string "Hi", the sizeof(X) is 3.

Q10. When Y is int* ptr, then sizeof(Y) == 4 in 32-bit machine, sizeof(Y) == 8 in 64- bit machine.

CH3

Q1. Ways to find the length of argv:
	1. printf("argc = %d\n", argc);

	2. loop till argv[i] == NULL.

Q2. argv[0] represents the execution name of the program.

Q3. The pointers to environment variables are stored above the stack.

Q4. The values of sizeof(ptr) and sizeof(array) are 8, 6 respectively.

Q5. The stack data structure manages the lifetime of automatic variables.

CH4

Q1. You should put it in heap. It can put there by adding key word static or by using malloc().

Q2. The allocation and deallocation of stack are automatically done while those of heap need to be manually done.
       Stack is used for static memory allocation, and heap is used for dynamic memory allocation.
       Variables of stack could not be resized while those of heap could be.

Q3. There are also Data Segment as a kind of memory in a process.

Q4. free

Q5. One reason malloc can fail is that there is no usable block of memory on heap that could satisfy the malloc() memory request.

Q6. The function time() returns the time since the Epoch in seconds, while ctime() converts "machine time" into ASCII time, which is readable to human.

Q7. This code snippet double frees the memory address pointed to by ptr.

Q8. The second line of the code snippet uses the dangling pointer as the second parameter. The memory address which ptr points to was freed in line 1.

Q9. To avoid the previous two mistakes, set freed pointers to NULL once they are freed.

Q10.
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

struct Person {
	char * name;
	int age;
	struct Person* list;
};

typedef struct Person person_t;


int main() {
	person_t* ptr_S = (struct Person*) malloc(sizeof(person_t));
	ptr_S->name = "Agent Smith";
	ptr_S->age = 128;
	person_t* ptr_M = (struct Person*) malloc(sizeof(person_t));
	ptr_M->name = "Sonny Moore";
	ptr_M->age = 256;
	ptr_M->list = ptr_S;
	ptr_S->list = ptr_M;
	return 0;
}
}
Q11.

person_t* create(char* aname, int aage) {
	printf("Creating person %s : %d", aname, aage);
	person_t* result = (person_t*) malloc(sizeof(person_t));
	result->name = strdup(aname);
	result->age = aage;
	result->list = (person_t*) malloc(sizeof(person_t) * 10);
	return result;
}


Q12.

void destroy(person_t* p) {
	p->age = 0;
	free(p->name);
	memset(p->list, 0, 10 * sizeof(person_t));
	free(p);
}


CH5.

Q1. gets(), puts()

Q2. One issue with gets() is that there could be buffer overflow, and corruption occurs to other variables.

Q3.
#include <stdarg.h>
#include <stdio.h>

int main() {
	char * data = "Hello 5 World";
	
	char var1[10];
	int var2;
	char var3[10];
	sscanf(data, "%s %d %s", var1, &var2, var3);
	printf("Result: %s : %d : %s", var1, var2, var3);
	return 0;
}

Q4. _GNU_SOURCE

Q5.
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

int main() {
	FILE * fl;
	char * buffer = NULL;
	size_t capacity = 0;
	
	fl = fopen("/etc/motd", "r");
	
	if (stdin == NULL) {
		return 0;
	}
	
	while (getline(&buffer, &capacity, stdin) != EOF) {
		printf("%s\n", buffer);
	}
	
	fclose(fl);
	if (buffer != NULL) {
		free(buffer);
	}
	return 0;
}

C Development

Q1. The compiler flag: -g is used to generate a debug build.

Q2. Solely generating a new build is insufficient because if not using the "make clean" command, some objects and dependencies wouldn't be cleaned. Then, some compile errors could occur because of that.

Q3.Only tabs are used to indent the commands after the rule in a Makefile while spaces aren't.

Q4. git commit saves changes to the local repository.  sha in the context of git is a secure hash algorithm that returns a unique key of a specific commit and store it in git database.

Q5. git log shows all commits in the history of the repository, with she, author, date, and commit messages.

Q6. git status tell users the state of the working directory and the staging area. .gitignore excludes files and objects which you want to ignore from the git status.

Q7. git push uploads the content of local repository to remote repository. it is insufficient because changes of a certain commit isn't specified in the message, which only vaguely says all bug fixe.

Q8. It means that the remote brand can't be fat forwarded onto the pushed commit. The most common way of dealing with this is to merge commits with the new tip of the remote branch.
