/* contains routines used for compression and decompression */

#ifndef HUFFMAN_H
#define HUFFMAN_H

/* Reads the input file and makes the frequency table */
size_t read(const char *source, uint32_t *table) {
	size_t bytes_read = 0;
	FILE *fin = fopen(source, "rb");
	if(fin == NULL) {
		fprintf(stderr,"error: %s not found\n", source);
		exit(1);
	}

	printf("Reading %s...\n", source);
	uint8_t buff;
	while(!feof(fin)) {
		if(fread(&buff, sizeof(buff), 1, fin)) {
			table[(uint8_t)(buff)]++;
			bytes_read++;
		}
	}

	fclose(fin);
	return bytes_read;
}



/* Writes chars and their frequencies to compressed file so that the Huffman
 * tree can be rebuilt during decompression.
 * This is called before the huffman code is written
 */
size_t write_header_info(const char* target, uint32_t* table, uint32_t uncomp_bytes) {
	FILE* fout = fopen(target, "wb");

	size_t bytes_written = 0;
	uint8_t ascii_char;
	uint32_t frequency;
	
	uint16_t c = 0; /* counts the no. of distinct ascii chars i.e #leaf_nodes*/
	for(int i = 0; i < 256; i++) {
		if(table[i] != 0) {
			c++;
		}
	}

	fwrite(&c, sizeof(c), 1, fout);
	bytes_written += sizeof(c);

	for(int i = 0; i < 256; i++) {
		if(table[i] != 0) {
			ascii_char = (uint8_t)(i);
			frequency = table[i];

			fwrite(&ascii_char, sizeof(ascii_char), 1, fout);
			fwrite(&frequency, sizeof(frequency), 1, fout);
			bytes_written += sizeof(ascii_char) + sizeof(frequency);
		//	printf("%c  %lld\n", ascii_char, frequency);
		}
	}
	fwrite(&uncomp_bytes, sizeof(uncomp_bytes), 1, fout);
	bytes_written += sizeof(uncomp_bytes);
	fclose(fout);
	return bytes_written;
}



/* Reads frequency table from compressed file for rebuilding Huffman tree */
size_t parse_header_info(const char* source, uint32_t *table){
	FILE* fin = fopen(source, "rb");
	if(fin == NULL) {
		fprintf(stderr,"error: %s not found\n", source);
		exit(1);
	}

	printf("reading %s...\n", source);
	size_t bytes_read = 0;
	uint8_t ascii_char;
	uint32_t frequency;
	uint16_t c;
	fread(&c, sizeof(c), 1, fin);
	bytes_read += sizeof(c);

	for(int i = 0; i < c; i++) {
		fread(&ascii_char, sizeof(ascii_char), 1, fin);
		fread(&frequency, sizeof(frequency), 1, fin);
		table[ascii_char] = frequency;
		bytes_read += sizeof(frequency) + sizeof(ascii_char);
	}
	fclose(fin);
	return bytes_read;
}



/* Builds Huffman tree using nodes from the list of pending nodes.
 * This is done till there is only one node left in the pending list (i.e a single tree)
 */
t_node *build_tree(l_node **t) {
	l_node *w_list = *t;
	t_node *root = NULL;
	t_node *min1 = NULL, *min2 = NULL;
	uint32_t min_val = 0;

	while(w_list->next != NULL) {
		root = (t_node*)malloc(sizeof(t_node));
		
		min_val = find_min(&w_list);
		min1 = extract(&w_list, min_val);
		
		min_val = find_min(&w_list);
		min2 = extract(&w_list, min_val);
		
		root->left = min1;
		root->right = min2;
		root->frequency = min1->frequency + min2->frequency;
		append(&w_list, root);
	}
	return root;
}



/* Traverses the tree to generate Huffman code and store it in a string
 * at corresponding ascii char valued index
 */
void get_code(t_node *t, char *current_code, char hcode[][256]) {
	if(t->left == NULL && t->right == NULL) {
		strcpy(hcode[(int)(t->character)], current_code);
	}
	else {
		uint8_t len = strlen(current_code);
		char left_code[256],right_code[256];
		
		strcpy(left_code,current_code);
		strcpy(right_code,current_code);
		
		left_code[len] = '0';
		left_code[len+1] = '\0';
		right_code[len] = '1';
		right_code[len+1] = '\0';

		get_code(t->left,left_code, hcode);
		get_code(t->right,right_code, hcode);
	}
}



/* Writes the source file to target using corresponding Huffman codes instead of ascii codes.
 * Starts by reading the source file again, fills up current_buff(an unsigned char) with Huffman code
 * and writes it to the target file.
 */
size_t compress(const char *target, const char *source, uint32_t n_bytes, char code[][256]) {
	FILE *fin = fopen(source, "rb");
	FILE *fout = fopen(target, "ab");

	size_t bytes_written = 0;
	uint8_t current_buff = 0;  /* char written to fout (target) */
	uint8_t current_char = 0;  /* char read from fin (source) */
	uint8_t ors = 0;  /* counts OR operations on current_buff i.e #bits filled in current_buff */
	
	int16_t len = 0, i = -1, j;
	size_t x = 0; /* counts the no. of bytes that have been read from input */

	printf("writing %s...\n", target);

	do {
		i = -1;
		if(feof(fin) == false && fread(&current_char, sizeof(current_char), 1, fin) == true) {
			i = current_char, j = 0;
		
			len = strlen(code[i]);
			while(j < len) {
				if(code[i][j] == '1') {
					current_buff = current_buff | 1;
				}
				ors++;

				/* if current_buff is filled with 8 bits, write it to fout */
				if(ors == 8) {
					fputc(current_buff, fout);
				//	printf("%d\n", current_buff );
					current_buff = 0;
					ors = 0;
					bytes_written++;
				}

				/* if reached to the end of the source file, shift code completely to the left */
				else if(x == n_bytes-1 && j == len - 1) {
					current_buff = current_buff << (8 - ors);
					fputc(current_buff, fout);
				//	printf("%d\n", current_buff );
					bytes_written++;
				}

				/* otherwise, left shift all bits by 1 */
				else {
					current_buff = current_buff << 1;
				}				
				j++;
			}
			x++;
		}
	} while(i != -1);

	fclose(fin);
	fclose(fout);
	return bytes_written;
}



/* Traverse Huffman tree while parsing Huffman codes, and as a leaf node is reached, 
 * write the corresponding char to target file.
 * As 8 bits are read at a time, bit manipulations are used to differentiate 1s from 0s.
 * The additional arguement b is just used for counting the no. of bytes read
 */
size_t decompress(const char *source, const char *target, t_node* root, size_t *b) {
	FILE *fin = fopen(source, "rb");
	FILE *fout = fopen(target, "wb");

	/* First reading #leaf_nodes (2 bytes), and the skipping the #leaf_nodes entries if the frequency table 
	 * as they are already read, and the tree is ready
	 * Then, reading the original(uncompressed) file size so that the extra padding of 0s doesn't 
	 * lead to writing extra bytes
	 * (original file size was written to file in write_header_info())
	 */
	uint16_t c = 0; /* #leaf_nodes */
	uint32_t uncomp_bytes = 0;

	fread(&c, sizeof(c), 1, fin);
	fseek(fin, c*(sizeof(uint8_t) + sizeof(uint32_t)), SEEK_CUR); 
	fread(&uncomp_bytes, sizeof(uncomp_bytes), 1, fin);
	
	(*b) += sizeof(uncomp_bytes);

	/* Storing powers of 2 for fast lookup instead of re-calculation */
	uint8_t two_raised_to[8];
	for(int8_t i = 0; i < 8; i++) {
		two_raised_to[i] = (uint8_t)pow(2,i);
	}

	size_t bytes_written = 0;
	uint8_t current_char = 0;  /* char read from fin (source) */
	uint8_t check = 0; /* used to check bits in current_char */

	/* Now parsing the actual Huffman code and decoding it*/
	printf("decoding Huffman code and writing %s...\n", target);
	t_node *current = root;
	while(feof(fin) == false && fread(&current_char, sizeof(current_char), 1, fin) == true) {
		for(int8_t i = 7; i >= 0; i--) {
			check = current_char & two_raised_to[i];
			
			if(check != 0 && current->right != NULL) {
				current = current->right;
			}
			else if(check == 0 && current->left != NULL) {
				current = current->left;
			}

			if(current->left == NULL && current->right == NULL && uncomp_bytes != 0) {
				fputc(current->character, fout);
				current = root;
				bytes_written++;
				uncomp_bytes--;
			}
		}
		(*b)++;
	}
	return bytes_written;
}

#endif