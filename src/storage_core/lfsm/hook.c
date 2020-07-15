/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#include <linux/types.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include "config/lfsm_setting.h"
#include "config/lfsm_feature_setting.h"
#include "config/lfsm_cfg.h"
#include "common/common.h"
#include "common/rmutex.h"
#include "io_manager/mqbiobox.h"
#include "io_manager/biocbh.h"
#include "nextprepare.h"
#include "diskgrp.h"
#include "system_metadata/Dedicated_map.h"
#include "lfsm_core.h"
#include "hook.h"
#include "err_manager.h"

/*
 * Description: init hook for debug purpose
 * TODO: bad and not easy understanding, remove it
 */
int32_t hook_init(shook_t * hook)
{
    if (IS_ERR_OR_NULL(hook)) {
        LOGERR("hook is null when init\n");
        return -ENOMEM;
    }

    memset(hook, 0, sizeof(shook_t));

    return 0;
}
