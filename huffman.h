/* Contains routines used for compression and decompression */

#ifndef HUFFMAN_H
#define HUFFMAN_H

/* exit codes used
 * 1 -> file not found
 * 2 -> invalid file
 * 3 -> empty file
 */

/* Read the input file and make the frequency table */
size_t read(const char *source, uint32_t *table) {
	size_t bytes_read = 0;
	FILE *fin = fopen(source, "rb");
	
	if(!fin) {
		fprintf(stderr,"error: %s not found\n", source);
		exit(1);
	}

	fseek(fin, 0, SEEK_END);
	long int size = ftell(fin);

	if(size == 0){
		fclose(fin);
		fprintf(stderr, "error: %s is an empty file\n", source);
		exit(3);
	}
	else {
		fseek(fin, 0, SEEK_SET);
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
}



/* Write first char (i.e first 8 bits) of the encoded bitstring at the beginning of the compressed file.
 * Before starting decompression, these the chars are compared (see check() function) , if they donot match,
 * it implies that the file is not a hcomp archive.
 *
 * It's a heuristic and in an in some rare case, the chars may match and hcomp may produce some
 * garbage output file or a lead to a segmentation fault while running decompression routines.
 * It's rare because here's how works:
 *   - 1st byte -> read to check_num
 *   - the next 2 bytes -> read to c
 *   - the next [c*(1 + 4) + 4] bytes are skipped  (this value is depends on c)
 *   - then 1 byte is read to num
 *   - compare check_num and num
 */
size_t write_check_num(const char *source, const char* target, char code[][256]) {
	FILE *fin = fopen(source, "rb");
	FILE *fout = fopen(target, "wb");
	
	uint8_t check_num = 0;
	uint8_t current_char = 0;
	uint8_t ors = 0;
	uint16_t len = 0;

	while(ors < 8) {
		if(!feof(fin) && fread(&current_char, sizeof(current_char), 1, fin)) {
			len = strlen(code[current_char]);
			for(int i = 0; i < len; i++) {
				if(code[current_char][i] == '1') {
					check_num = check_num | 1;
				}
				ors++;
				if(ors == 8) {
					break;
				}
				else {
					check_num = check_num << 1;
				}
			}
		}
		else {
			check_num = check_num << 8 - ors;
			break;
		}
	}
	
	fputc(check_num, fout);
	fclose(fin);
	fclose(fout);
	return sizeof(check_num);
}



void check(const char* source){
	FILE *fin = fopen(source, "rb");
	if(!fin) {
		fprintf(stderr, "error: %s not found\n", source);
		exit(1);
	}

	fseek(fin, 0, SEEK_END);
	long int size = ftell(fin);

	if(size == 0){
		fclose(fin);
		fprintf(stderr, "error: %s is an empty file\n", source);
		exit(3);
	}
	else {
		fseek(fin, 0, SEEK_SET);
		uint8_t check_num = 0;
		uint8_t num = 1;
		uint16_t c;

		fread(&check_num, sizeof(check_num), 1, fin);
		fread(&c, sizeof(c), 1, fin);

		fseek(fin, c*(sizeof(uint8_t) + sizeof(uint32_t)) + sizeof(uint32_t), SEEK_CUR);
		fread(&num, sizeof(num), 1, fin);
		fclose(fin);

		if(num != check_num) {
			fprintf(stderr, "error: %s is not a hcomp archive\n", source);
			exit(2);
		}
	}
}



/* Write chars and their frequencies to compressed file so that the tree can
 * be rebuilt during decompression.
 * This is called before the compressed bitstring is written
 */
size_t write_header_info(const char* target, uint32_t* table, uint32_t uncomp_bytes) {
	FILE* fout = fopen(target, "ab");

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



/* Read frequency table from compressed file for rebuilding the huffman tree */
size_t parse_header_info(const char* source, uint32_t *table){
	FILE* fin = fopen(source, "rb");
	if(!fin) {
		fprintf(stderr,"error: %s not found\n", source);
		exit(1);
	}

	printf("reading %s...\n", source);
	size_t bytes_read = 0;
	uint8_t ascii_char;
	uint32_t frequency;
	uint16_t c;

	fseek(fin, sizeof(uint8_t), SEEK_CUR);
	fread(&c, sizeof(c), 1, fin);
	bytes_read += sizeof(c) + sizeof(uint8_t);

	for(int i = 0; i < c; i++) {
		fread(&ascii_char, sizeof(ascii_char), 1, fin);
		fread(&frequency, sizeof(frequency), 1, fin);
		table[ascii_char] = frequency;
		bytes_read += sizeof(frequency) + sizeof(ascii_char);
	}
	fclose(fin);
	return bytes_read;
}



/* Build tree using nodes from the list of pending nodes.
 * This is done till there is only one node left in the pending list (i.e a single tree)
 */
t_node *build_tree(l_node **t) {
	l_node *w_list = *t;
	t_node *root = w_list->subtree;
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



/* Traverse the tree to generate huffman code and store it in a string
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



/* Write the source file to target using corresponding huffman codes instead of ascii codes.
 * Start by reading the source file again, fill up current_buff(an unsigned char) with huffman code
 * and write it to the target file.
 */
size_t compress(const char *source, const char *target, uint32_t n_bytes, char code[][256]) {
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



/* Traverse the tree while parsing huffman codes, and as a leaf node is reached, 
 * write the corresponding char to target file.
 * As 8 bits are read at a time, bit manipulations are used to differentiate 1s from 0s.
 * The additional arguement *b is just used for counting the no. of bytes read
 */
size_t decompress(const char *source, const char *target, t_node* root, size_t *b) {
	FILE *fin = fopen(source, "rb");
	FILE *fout = fopen(target, "wb");

	/* First reading #leaf_nodes (2 bytes), and the skipping the #leaf_nodes entries of the frequency table 
	 * as they are already read, and the tree is ready
	 * Then, reading the original(uncompressed) file size so that the extra padding of 0s doesn't 
	 * lead to writing extra bytes after decompression
	 * (original file size was written to file in write_header_info())
	 */
	uint16_t c = 0; /* #leaf_nodes */
	uint32_t uncomp_bytes = 0;
	fseek(fin, sizeof(uint8_t), SEEK_SET);
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

	printf("decoding huffman codes and writing %s...\n", target);
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