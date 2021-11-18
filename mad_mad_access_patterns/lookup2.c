/**
 * mad_mad_access_patterns
 * CS 241 - Fall 2021
 */
#include "tree.h"
#include "utils.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdbool.h>


/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses mmap to access the data.

  ./lookup2 <data_file> <word> [<word> ...]
*/

BinaryTreeNode * recur_search (uint32_t os_, char* addr_, char* w_) {
	if (0 == os_) {
		return NULL;
	}
	BinaryTreeNode* n_ = (BinaryTreeNode*) (os_ + addr_);
	BinaryTreeNode* n_w;
	if (0 < strcmp(w_, n_->word)) {
		n_w = recur_search(n_->right_child, addr_, w_);
		if (n_w) {
			return n_w;
		}
	} else if (0 > strcmp(w_, n_->word)) {
		n_w = recur_search(n_->left_child, addr_, w_);
		if (n_w) {
			return n_w;
		}
	} else {
		return n_;
	}
}

int main(int argc, char **argv) {
	if (3 > argc) {
		printArgumentUsage();
		exit(1);
	}
	char* f_n = argv[1];
	int op_ = open(f_n, O_RDONLY);
	if (0 > op_) {
		openFail(f_n);
		exit(2);
	}
	struct stat fs_;
	if (0 != fstat(op_, &fs_)) {
		openFail(f_n);
		exit(2);
	}
	char* addr_ = mmap(NULL, fs_.st_size, PROT_READ, MAP_PRIVATE, op_, 0);
	if ((void*) -1 == addr_) {
		mmapFail(f_n);
		exit(2);
	}
	if (strncmp(addr_, BINTREE_HEADER_STRING, BINTREE_ROOT_NODE_OFFSET)) {
    	formatFail(f_n);
    	exit(2);
  	}
	for (int j = 2; j < argc; ++j) {
		BinaryTreeNode* n_w = recur_search(BINTREE_ROOT_NODE_OFFSET, addr_, argv[j]);
		if (!n_w) {
			printNotFound(argv[j]);
		} else {
			printFound(n_w->word, n_w->count, n_w->price);
		}
	}
	close(op_);
	return 0;
}
