/*
 * Copyright 2016, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * ex_linkedlist.c - test of linkedlist example
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pmemobj_list.h"
#include "unittest.h"

#define ELEMENT_NO	10
#define PRINT_RES(res, struct_name) do {\
	if (res == 0) {\
		UT_OUT("Outcome for " #struct_name " is correct!");\
	} else {\
		UT_ERR("Outcome for " #struct_name\
				" does not match expected result!!!");\
	}\
} while (0)

POBJ_LAYOUT_BEGIN(list);
POBJ_LAYOUT_ROOT(list, struct base);
POBJ_LAYOUT_TOID(list, struct tqueuehead);
POBJ_LAYOUT_TOID(list, struct slisthead);
POBJ_LAYOUT_TOID(list, struct tqnode);
POBJ_LAYOUT_TOID(list, struct snode);
POBJ_LAYOUT_END(list);

POBJ_TAILQ_HEAD(tqueuehead, struct tqnode);
struct tqnode {
	int data;
	POBJ_TAILQ_ENTRY(struct tqnode) tnd;
};

POBJ_SLIST_HEAD(slisthead, struct snode);
struct snode {
	int data;
	POBJ_SLIST_ENTRY(struct snode) snd;
};

struct base {
	TOID(struct tqueuehead) tqueue;
	TOID(struct slisthead) slist;
};

const int expectedResTQ[] = { 111, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 222 };
const int expectedResSL[] = { 111, 8, 222, 6, 5, 4, 3, 2, 1, 0, 333 };

/*
 * init_tqueue -- initialize tail queue
 */
static void
init_tqueue(PMEMobjpool *pop, TOID(struct tqueuehead) head)
{
	if (!POBJ_TAILQ_EMPTY(head))
		return;

	TOID(struct tqnode) node;
	TOID(struct tqnode) middleNode;
	TOID(struct tqnode) node888;
	TOID(struct tqnode) tempNode;
	int i = 0;
	TX_BEGIN(pop) {
		POBJ_TAILQ_INIT(head);
		for (i = 0; i < ELEMENT_NO; ++i) {
			node = TX_NEW(struct tqnode);
			D_RW(node)->data = i;
			if (0 == i) {
				middleNode = node;
			}

			POBJ_TAILQ_INSERT_HEAD(head, node, tnd);
			node = TX_NEW(struct tqnode);
			D_RW(node)->data = i;
			POBJ_TAILQ_INSERT_TAIL(head, node, tnd);
		}
		UT_OUT("POBJ_TAILQ_INSERT_HEAD, "
				"POBJ_TAILQ_INSERT_TAIL");

		node = TX_NEW(struct tqnode);
		D_RW(node)->data = 666;
		POBJ_TAILQ_INSERT_AFTER(middleNode, node, tnd);

		middleNode = POBJ_TAILQ_NEXT(middleNode, tnd);

		node = TX_NEW(struct tqnode);
		D_RW(node)->data = 888;
		node888 = node;
		POBJ_TAILQ_INSERT_BEFORE(middleNode, node, tnd);
		node = TX_NEW(struct tqnode);
		D_RW(node)->data = 555;
		POBJ_TAILQ_INSERT_BEFORE(middleNode, node, tnd);

		node = TX_NEW(struct tqnode);
		D_RW(node)->data = 111;
		tempNode = POBJ_TAILQ_FIRST(head);
		POBJ_TAILQ_INSERT_BEFORE(tempNode, node, tnd);
		node = TX_NEW(struct tqnode);
		D_RW(node)->data = 222;
		tempNode = POBJ_TAILQ_LAST(head);
		POBJ_TAILQ_INSERT_AFTER(tempNode, node, tnd);

		UT_OUT("POBJ_TAILQ_INSERT_BEFORE, "
				"POBJ_TAILQ_INSERT_AFTER");

		tempNode = middleNode;
		middleNode = POBJ_TAILQ_PREV(tempNode, tnd);
		POBJ_TAILQ_MOVE_ELEMENT_TAIL(head, middleNode, tnd);
		POBJ_TAILQ_MOVE_ELEMENT_HEAD(head, tempNode, tnd);

		UT_OUT("POBJ_TAILQ_MOVE_ELEMENT_HEAD, "
				"POBJ_TAILQ_MOVE_ELEMENT_TAIL");

		tempNode = POBJ_TAILQ_FIRST(head);
		POBJ_TAILQ_REMOVE(head, tempNode, tnd);
		tempNode = POBJ_TAILQ_LAST(head);
		POBJ_TAILQ_REMOVE(head, tempNode, tnd);
		POBJ_TAILQ_REMOVE(head, node888, tnd);
		UT_OUT("POBJ_TAILQ_REMOVE");
	} TX_ONABORT {
		abort();
	} TX_END
}

/*
 * init_slist -- initialize SLIST
 */
static void
init_slist(PMEMobjpool *pop, TOID(struct slisthead) head)
{
	if (!POBJ_SLIST_EMPTY(head))
		return;

	TOID(struct snode) node;
	TOID(struct snode) tempNode;
	int i = 0;
	TX_BEGIN(pop) {
		POBJ_SLIST_INIT(head);
		for (i = 0; i < ELEMENT_NO; ++i) {
			node = TX_NEW(struct snode);
			D_RW(node)->data = i;
			POBJ_SLIST_INSERT_HEAD(head, node, snd);
		}
		UT_OUT("POBJ_SLIST_INSERT_HEAD");

		tempNode = POBJ_SLIST_FIRST(head);
		node = TX_NEW(struct snode);
		D_RW(node)->data = 111;
		POBJ_SLIST_INSERT_AFTER(tempNode, node, snd);

		tempNode = POBJ_SLIST_NEXT(node, snd);
		node = TX_NEW(struct snode);
		D_RW(node)->data = 222;
		POBJ_SLIST_INSERT_AFTER(tempNode, node, snd);

		UT_OUT("POBJ_SLIST_INSERT_AFTER");

		tempNode = POBJ_SLIST_NEXT(node, snd);
		POBJ_SLIST_REMOVE_FREE(head, tempNode, snd);
		UT_OUT("POBJ_SLIST_REMOVE_FREE "
				"Remove intermediate element");

		POBJ_SLIST_REMOVE_HEAD(head, snd);
		UT_OUT("POBJ_SLIST_REMOVE_HEAD");

		TOID(struct snode) element = POBJ_SLIST_FIRST(head);
		while (!TOID_IS_NULL(D_RO(element)->snd.pe_next)) {
			element = D_RO(element)->snd.pe_next;
		}
		node = TX_NEW(struct snode);
		D_RW(node)->data = 333;
		POBJ_SLIST_INSERT_AFTER(element, node, snd);

		element = node;
		node = TX_NEW(struct snode);
		D_RW(node)->data = 123;
		POBJ_SLIST_INSERT_AFTER(element, node, snd);

		tempNode = POBJ_SLIST_NEXT(node, snd);
		POBJ_SLIST_REMOVE_FREE(head, node, snd);
		UT_OUT("POBJ_SLIST_INSERT_AFTER, "
			"POBJ_SLIST_REMOVE_FREE "
			"Insert to tail and remove from tail");

	} TX_ONABORT {
		abort();
	} TX_END
}

int
main(int argc, char *argv[])
{
	unsigned int res = 0;
	PMEMobjpool *pop;
	const char *path;

	START(argc, argv, "ex_linkedlist");

	if (argc != 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}
	path = argv[1];

	if (access(path, F_OK) != 0) {
		if ((pop = pmemobj_create(path, POBJ_LAYOUT_NAME(list),
			PMEMOBJ_MIN_POOL, 0666)) == NULL) {
			UT_FATAL("!pmemobj_create: %s", path);
		}
	} else {
		if ((pop = pmemobj_open(path,
				POBJ_LAYOUT_NAME(list))) == NULL) {
			UT_FATAL("!pmemobj_open: %s", path);
		}
	}

	TOID(struct base) base = POBJ_ROOT(pop, struct base);
	TOID(struct tqueuehead) tqhead = D_RO(base)->tqueue;
	TOID(struct slisthead) slhead = D_RO(base)->slist;
	TX_BEGIN(pop) {
		tqhead = TX_NEW(struct tqueuehead);
		slhead = TX_NEW(struct slisthead);
	} TX_ONABORT {
		abort();
	} TX_END

	init_tqueue(pop, tqhead);
	init_slist(pop, slhead);

	int i = 0;
	TOID(struct tqnode) tqelement;
	POBJ_TAILQ_FOREACH(tqelement, tqhead, tnd) {
		if (D_RO(tqelement)->data != expectedResTQ[i]) {
			res = 1;
			break;
		}
		i++;
	}
	PRINT_RES(res, tail queue);

	i = 0;
	res = 0;
	TOID(struct snode) slelement;
	POBJ_SLIST_FOREACH(slelement, slhead, snd) {
		if (D_RO(slelement)->data != expectedResSL[i]) {
			res = 1;
			break;
		}
		i++;
	}
	PRINT_RES(res, singly linked list);
	pmemobj_close(pop);

	DONE(NULL);
}