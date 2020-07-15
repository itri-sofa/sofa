/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#ifndef CONFIG_LFSM_FEATURE_SETTING_H_
#define CONFIG_LFSM_FEATURE_SETTING_H_

/*
 * Lego-20161005
 * VER = 0: means no need check license
 * VER = 1: means check license expire base os disk disk spin time
 *          => Not a validate method
 * VER = 2: useless
 * VER = 3: when user write data over certain threshold,
 *          sofa will each 4K data's last 32 byte value and un-recoveryable
 */
#define LFSM_RELEASE_VER 0

#define LFSM_CONFLICT_CHECK   1

//#define ENABLE_MEM_TRACKER

//#define HLIST_GC_WHOLE_LOCK_DEBUG   1
#define LFSM_PAGE_SWAP 1

#define LFSM_FLUSH_SUPPORT 0    // 2.6.35:BIO_RW_BARRIER, 3.5.0 REQ_FLUSH

#define LFSM_REDUCE_BIOC_PAGE     1

#define LFSM_ENABLE_TRIM  1
#define TRIM_ALL_FREE   0       //((LFSM_ENABLE_TRIM>0) && 1)
#define LFSM_PREERASE     (1 && LFSM_ENABLE_TRIM>0)

#define DYNAMIC_PPQ_LOCK_KEY 1  //if 0, maximum dev is 24

#define LFSM_REVERSIBLE_ASSERT 1

#endif /* CONFIG_LFSM_FEATURE_SETTING_H_ */
