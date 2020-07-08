/*
 * Copyright (c) 2015-2025 Industrial Technology Research Institute.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef SOFA_MM_H_
#define SOFA_MM_H_

#ifndef SOFA_USER_SPACE

extern void *sofa_kmalloc(size_t m_size, gfp_t m_flags);
extern void sofa_kfree(void *ptr);

extern void *sofa_vmalloc(size_t m_size);
extern void sofa_vfree(void *ptr);

extern struct page *sofa_alloc_page(gfp_t m_flags);
extern void sofa_free_page(struct page *mypage);

#else

extern void *sofa_malloc(size_t m_size);
extern void sofa_free(void *ptr);

#endif

extern void init_sofa_mm(void);
extern void rel_sofa_mm(void);

#endif /* SOFA_MM_H_ */
