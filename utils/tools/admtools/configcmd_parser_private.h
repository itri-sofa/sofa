/*
 * Copyright (c) 2015-2020 Industrial Technology Research Institute.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 *
 *
 *
 *
 *
 *  
 */

#ifndef CONFIGCMD_PARSER_PRIVATE_H_
#define CONFIGCMD_PARSER_PRIVATE_H_

#define CMD_CFGFN_PREFIX    "cfgfn="

int8_t *cfgcmds_str[] = {
        "--updatefile",
        "--update_append_file" 
};

#define DEF_CFG_FN  "/usr/sofa/config/sofa_config.xml"

#endif /* CONFIGCMD_PARSER_PRIVATE_H_ */
