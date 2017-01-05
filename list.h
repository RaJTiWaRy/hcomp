/* implementation of a list for holding huffman tree nodes(type t_node) as keys
 * having few simple abstractions
 */

#ifndef LIST_H
#define LIST_H

#include <limits.h>  /* for using UINT_MAX */

typedef struct l_node{
	t_node *subtree;
	struct l_node *next;
} l_node;


void append(l_node **list, t_node *t) {
	l_node* new_node = (l_node*)malloc(sizeof(l_node));
	new_node->subtree = t;
	new_node->next = NULL;

	if(*list == NULL) {
		*list = new_node;
	}
	else {
		l_node *current = *list;
		while(current->next != NULL) {
			current = current->next;
		}
		current->next = new_node;
	}
}


/* find minimum frequency */
uint32_t find_min(l_node **list) {
	l_node *current = *list;
	uint32_t min = UINT_MAX;

	while(current != NULL) {
		if(current->subtree->frequency < min) {
			min = current->subtree->frequency;
		}
		current = current->next;
	}
	return min;
}


/* remove first node with frequency x from the list and return its address */
t_node *extract(l_node **list, uint32_t x) {
	l_node *head = *list;
	l_node *current = head;
	l_node *prev = head;
	t_node* sub_tree = NULL; /* this  will be returned */

	while(current && current->subtree->frequency != x) {
		prev = current;
		current = current->next;
	}
	
	if(head == current) {
		head = head->next;
	}
	else {
		prev->next = current->next;
	}

	*list = head;

	sub_tree = (current->subtree);
	free(current);
	return sub_tree;
}

#endif