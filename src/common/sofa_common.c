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

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/types.h>
#include "sofa_common_private.h"
#include "sofa_common.h"
#include "sofa_mm.h"
#include "sofa_config.h"

/*
 * Description: main function to initialize sofa common module
 * Input  :
 * Output :
 *     ret: 0: initialize success
 *          non 0: initialize fail
 * Process:
 * NOTE   :
 */
int32_t init_sofa_comm(void)
{
    int32_t ret;

    ret = 0;

    atomic_set(&sofa_rtime_err, 0);
    sofa_loglevel = LOG_INFO;

    init_sofa_mm();
    init_sofa_config();

    return ret;
}

/*
 * Description: function to release sofa common module
 * Input  :
 * Output :
 *     ret: 0: initialize success
 *          non 0: initialize fail
 * Process:
 * NOTE   :
 */
int32_t rel_sofa_comm(void)
{
    int32_t ret;

    ret = 0;

    rel_sofa_config();
    rel_sofa_mm();

    if (atomic_read(&sofa_rtime_err) > 0) {
        /*
         * TODO: Lego: Maybe we should notify user space daemon and log information
         *       at specific log file
         */
        sofa_printk(LOG_ERR, "ERROR has run time err %d\n",
                    atomic_read(&sofa_rtime_err));
        SOFA_WARN_ON(true);
    }

    return ret;
}
