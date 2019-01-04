/*
 * param_list_store_vmem.c
 *
 *  Created on: Dec 19, 2016
 *      Author: johan
 */

#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <param/param_list.h>
#include "param_list.h"

#include <vmem/vmem.h>

#include <csp/csp.h>

param_t * param_list_from_line(char * line) {

	char name[25] = {};
	unsigned int id, type, mask, node;
	int size;
	int scanned = sscanf(line, "%u,%u,%25[^,],%u,%d,%x%*s", &node, &id, name, &type, &size, &mask);

	if (scanned < 4)
		return NULL;

	printf("Found parameter => %s", line);

	if (size == -1) {
		size = param_typesize(type);
	}

	param_t * param = param_list_find_id(node, id);

	if (param == NULL) {
		param = param_list_create_remote(id, node, type, mask, size, name, strlen(name));
		param_list_add(param);
	}

	return param;

}

static void param_list_from_string(FILE *stream) {

	char line[100] = {};
	while(fgets(line, 100, stream) != NULL) {
		param_list_from_line(line);
	}
}

static void param_list_to_string(FILE * stream, int node_filter, int remote_only) {

	param_t * param;
	param_list_iterator i = {};
	while ((param = param_list_iterate(&i)) != NULL) {

		if ((node_filter >= 0) && (param->node != node_filter))
			continue;

		if ((remote_only) && (param->node == PARAM_LIST_LOCAL))
			continue;

		int node = param->node;
		if (node == PARAM_LIST_LOCAL)
			node = csp_get_address();

		fprintf(stream, "%u,%u,%s,%u,%d,%x\n", node, param->id, param->name, param->type, param->array_size, param->mask);
	}

}

void param_list_store_vmem_save(const vmem_t * vmem) {
	char * hk_list = calloc(vmem->size, 1);

	FILE *stream = fmemopen(hk_list, vmem->size, "w");
	param_list_to_string(stream, -1, 1);
	fclose(stream);

	vmem_memcpy((void *) vmem->vaddr, hk_list, vmem->size);
	//csp_hex_dump("hk list", hk_list, vmem->size);
	free(hk_list);
}

void param_list_store_vmem_load(const vmem_t * vmem) {

	char * hk_list = calloc(vmem->size, 1);
	if (hk_list == NULL)
		return;

	//printf("Reading into %p %u bytes\n", hk_list, vmem->size);
	vmem_memcpy(hk_list, (void *) vmem->vaddr, vmem->size);
	//csp_hex_dump("hk list", hk_list, strlen(hk_list));

	unsigned int len = strlen(hk_list);
	if (len == 0) {
		free(hk_list);
		return;
	}

	//csp_hex_dump("hk list", hk_list, len);
	FILE *stream = fmemopen(hk_list, len, "r");
	param_list_from_string(stream);
	fclose(stream);
	free(hk_list);

}
