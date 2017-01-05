# hcomp
A rudimentary compression tool written from scratch

### Overview

__Implementation__ : *Using Huffman coding*
* __Compression__
	* Reads source file one unsigned char (8 bits) at a time
	* Counts frequency of each char and makes a frequency table
	* Builds a Huffman tree
	* Traverses the tree and gets Huffman code for each character
	* Writes the frequency table to the target file (so that the tree can be rebuilt for decompression)
	* Then keeps on filling up the Huffman code in a buffer of 8 bits and when 8 bits are filled, writes it to the target file
* __Decompression__
	* First reads the frequency table from the compressed file
	* Rebuilds the Huffman tree
	* Reads the Huffman code from compressed file and simultaneously traverses the tree
	* If a leaf node is reached at any point, corresponding char is written to the target file

*_Update_ : Now checks whether a file is a valid hcomp archive before running the decompression routines. This prevents some segmentation faults*


### __Building :__
using gcc : ` $ gcc -lm -o hcomp main.c`		
using clang : `$ clang -lm -o hcomp main.c `		

### Screens
Running hcomp on kennedy.xls from [The Canterbury Corpus](http://corpus.canterbury.ac.nz/descriptions/#cantrbry)

![Comp-Decomp](http://i.imgur.com/QWh8vjm.png)

Error Handling
![Other Stuff](http://i.imgur.com/Hf4E3G9.png)

__PS :__
Sometimes the compressed file may turn out to be bigger than the original file, this is because the compressed file contains additional information used for decompression.		
(It is most likely to happen with very small files)
