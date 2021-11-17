/**
 * mad_mad_access_patterns
 * CS 241 - Fall 2021
 */
#include "tree.h"
#include "utils.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses fseek() and fread() to access the data.

  ./lookup1 <data_file> <word> [<word> ...]
*/

int recur_search(uint32_t os_, FILE *f_, char *w_) {
	if (0 == os_) {
		return 0;
	}
	fseek(f_, os_, SEEK_SET);
	BinaryTreeNode n_;
	fread(&n_, sizeof(BinaryTreeNode), 1, f_);
	fseek(f_, sizeof(BinaryTreeNode) + os_, SEEK_SET);
	char n_w[10];
	if (0 < strcmp(w_, n_w)) {
		if (recur_search(n_.right_child, f_, w_)) {
			return 1;
		}
	} else if (0 > strcmp(w_, n_w)) {
		if (recur_search(n_.left_child, f_, w_)) {
			return 1;
		}
	} else {
		printFound(n_w, n_.count, n_.price);
	}
	return 0;
}


int main(int argc, char **argv) {
	if (3 > argc) {
		printArgumentUsage();
		exit(1);
	}
	char *f_n = argv[1];
	FILE * f_ = fopen(f_n, "r");
	if (0 == f_) {
		openFail(f_n);
		exit(2);
	}
	char r_[BINTREE_ROOT_NODE_OFFSET];
	fread(r_, 1, BINTREE_ROOT_NODE_OFFSET, f_);
	if (strcmp(r_, BINTREE_HEADER_STRING)) {
		formatFail(f_n);
		exit(2);
	}
	for (int j = 2; j < argc; ++j) {
		bool flag_ = recur_search(BINTREE_ROOT_NODE_OFFSET, f_, argv[j]);
		if (!flag_) {
			printNotFound(argv[j]);
		}
	}
	fclose(f_);
	return 0;
}

