# hcomp
A rudimentary compression tool written from scratch

### Overview

__Implementation__ : *Using huffman coding*
* __Compression__
	* Reads source file one unsigned char (8 bits) at a time
	* Counts frequency of each char and makes a frequency table
	* Builds a huffman tree
	* Traverses the tree and gets huffman code for each character
	* Writes the frequency table to the target file (so that the tree can be rebuilt for decompression)
	* Then keeps on filling up the huffman code in a buffer of 8 bits and when 8 bits are filled, writes it to the target file
* __Decompression__
	* First reads the frequency table from the compressed file
	* Rebuilds the tree
	* Reads the huffman code from compressed file and simultaneously traverses the tree
	* If a leaf node is reached at any point, corresponding char is written to the target file

*_Update_ : Now verifies file before running the decompression routines. This ensures hcomp decompresses only hcomp archives (this is a heuristic and can fail in some rare cases)*


### __Building :__
using gcc : ` $ gcc -lm -o hcomp main.c`		
using clang : `$ clang -lm -o hcomp main.c `		

### Screens
Running hcomp on kennedy.xls and sum from [The Canterbury Corpus](http://corpus.canterbury.ac.nz/descriptions/#cantrbry) and also showing the running time
<img src="http://i.imgur.com/WMsEwdz.pngwidth" width="600">

Error Handling and other commands		
<img src="http://i.imgur.com/58QV7C7.png" width="600">


[//]: # (imgur post link : http://imgur.com/a/EZArw)

__PS :__
Sometimes the compressed file may turn out to be bigger than the original file, this is because the compressed file contains additional information used for decompression.		
(It is most likely to happen with very small files)