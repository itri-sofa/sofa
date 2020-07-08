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
#ifndef DESTIO_INC_1029831092741028312
#define DESTIO_INC_1029831092741028312

typedef unsigned long long sector64;
//enum edestio_type { r=0, w=1, copyr=2, copyw=3 };

struct sdestio_header
{
	int idx;
	int cn_item;
	sector64 date;	
};

struct sdestio_payload
{
	sector64 type; // r, w, copyr, copyw
	sector64 lpn; // in page
	sector64 pbn; // in sector
	sector64 err;
};

struct sdestio
{
	union{
		struct sdestio_payload payload;
		struct sdestio_header header;
	};
};

#endif

