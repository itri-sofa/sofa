/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#ifndef _MDNODE_H_
#define _MDNODE_H_

enum mdstate { md_empty = -1, md_exist = 0, md_copying = 1 };

typedef struct smetadata_node {
//    int idx;
    sector64 lpn[FPAGE_PER_SEU];
} smd_node_t;

typedef struct smetadata {
    smd_node_t *mdnode;
    atomic_t fg_state;          // mdstate
} smd_t;

struct EUProperty;

extern struct kmem_cache *mdnode_pool_create(void);
extern void mdnode_pool_destroy(struct kmem_cache *pMdpool);
extern int32_t mdnode_alloc(smd_t * pmd, struct kmem_cache *pMdpool);
extern int32_t mdnode_free(smd_t * pmd, struct kmem_cache *pMdpool);
extern void mdnode_set_lpn(smd_node_t * pNode, sector64 lpn, int32_t idx);
extern int32_t mdnode_free_all(lfsm_dev_t * td);
extern void mdnode_init(smd_t * pmd);
extern int32_t mdnode_set_state_copy(smd_t * pmd, wait_queue_head_t * pwq);
extern void mdnode_set_state(smd_t * pmd, int32_t mdstate);
extern int32_t mdnode_item_added(smd_node_t * pNode);
extern int32_t mdnode_hpeu_check_exist(struct HPEUTab *phpeutab,
                                       struct EUProperty **ar_peu,
                                       int32_t id_faildisk);
extern int32_t mdnode_check_exist_all(void);
extern int32_t mdnode_get_metadata(int8_t * buffer, sector64 addr,
                                   int32_t size);

#endif
