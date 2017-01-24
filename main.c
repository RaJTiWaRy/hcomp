#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

/* defining macros for ANSI escape codes */
#define RED_BOLD "\033[31;1m"
#define BOLD "\033[1m"
#define ITALIC "\033[3m"
#define RESET "\033[0m"

/* defining the huffman tree node as t_node */
typedef struct t_node{
	uint8_t character;
	uint32_t frequency;
	struct t_node* left;
	struct t_node* right;	
} t_node;

const float version = 1.3;

#include "list.h"
#include "huffman.h"


void show_help() {
	printf("usage : hcomp [--version] [--help] [-command $source $target]\n");
	printf("commands : \n");
	printf("    c    compress\n    d    decompress\n");
}


void show_invalid_args() {
	fprintf(stderr, BOLD "hcomp: " RED_BOLD "error: " RESET "invalid arguements\n");
	fprintf(stderr, "see 'hcomp --help' for usage\n");
}


/* Gets unit of file size (B, KB, MB, GB) */
uint8_t get_unit(size_t b) {
	float log_val = log2(b);
	if(log_val >= 30) {
		return 'G';
	}
	else if(log_val >= 20) {
		return 'M';
	}
	else if(log_val >= 10) {
		return 'K';
	}
	else {
		return '0';
	}
}


/* Returns value of the file size as per the unit */
float get_value(size_t b, char c) {
	switch(c) {
		case 'G' :
			return (float)b/(1024*1024*1024);
		case 'M' :
			return (float)b/(1024*1024);
		case 'K' :
			return (float)b/(1024);
		default :
			return (float)b;
	}
}


int main(const int argc, const char *argv[]) {
	switch (argc) {
		case 1: {
			/* no arguements */
			fprintf(stderr, BOLD "hcomp: " RED_BOLD "error: " RESET "no arguements\n");
			fprintf(stderr, "see 'hcomp --help' for usage\n");
			break;
		}
		
		case 2 : {
			/* --version , --help */
			if(strcmp(argv[1], "--help") == 0) {
				show_help();
			}
			else if(strcmp(argv[1], "--version") == 0) {
				printf("hcomp version %.1f\n", version);
			}
			else {
				show_invalid_args();
			}
			break;
		}

		case 4 : {
			l_node* pending_list = NULL;
			t_node * root = NULL;

			size_t bytes_in = 0;
			size_t bytes_out = 0;
			uint16_t len_pending_list = 0;

			uint32_t freq_table[256];
			memset(freq_table, 0, sizeof(freq_table));

			/* compression */
			if(strcmp(argv[1], "-c") == 0) {
				char code[512];
				memset(code, '\0', sizeof(code));
				
				char huff_code[256][256];
				memset(huff_code, '\0', sizeof(huff_code));
				
				bytes_in = read(argv[2], freq_table);

				for(uint16_t i = 0; i < 256; i++) {
						if(freq_table[i]) {
						t_node* new_node = (t_node*)malloc(sizeof(t_node));
						new_node->frequency = freq_table[i];
						new_node->character = (uint8_t)(i);
						new_node->left = NULL;
						new_node->right = NULL;
						append(&pending_list, new_node);
						len_pending_list++;
					}
				}
				
				root = build_tree(&pending_list);
				if(len_pending_list == 1) {
					huff_code[root->character][0] = '0';
				}
				else {
					get_code(root, code, huff_code);
				}

				bytes_out = write_check_num(argv[2], argv[3], huff_code);
				bytes_out += write_header_info(argv[3], freq_table, (uint32_t)bytes_in);
				bytes_out += compress(argv[2], argv[3], bytes_in, huff_code);

				printf("read %-15s " ITALIC "~%.3f%c%c\n", argv[2], get_value(bytes_in, get_unit(bytes_in)), get_unit(bytes_in), 'B');
				printf(RESET);
				printf("wrote %-15s" ITALIC "~%.3f%c%c\n", argv[3], get_value(bytes_out, get_unit(bytes_out)), get_unit(bytes_out), 'B');
				printf(RESET);
			}
			
			/* decompression */
			else if(strcmp(argv[1], "-d") == 0) {
				check(argv[2]);
				
				bytes_in += parse_header_info(argv[2], freq_table);
				
				for(uint16_t i = 0; i < 256; i++) {
						if(freq_table[i]) {
						t_node* new_node = (t_node*)malloc(sizeof(t_node));
						new_node->frequency = freq_table[i];
						new_node->character = (uint8_t)(i);
						new_node->left = NULL;
						new_node->right = NULL;
						append(&pending_list, new_node);
					}
				}

				root = build_tree(&pending_list);
				bytes_out += decompress(argv[2], argv[3], root, &bytes_in);
			
				printf("read %-15s " ITALIC "~%.3f%c%c\n", argv[2], get_value(bytes_in, get_unit(bytes_in)), get_unit(bytes_in), 'B');
				printf(RESET);
				printf("wrote %-15s" ITALIC "~%.3f%c%c\n", argv[3], get_value(bytes_out, get_unit(bytes_out)), get_unit(bytes_out), 'B');
				printf(RESET);
			}

			else {
				show_invalid_args();
			}
			break;
		}

		default:
			show_invalid_args();
			break;
	}
}