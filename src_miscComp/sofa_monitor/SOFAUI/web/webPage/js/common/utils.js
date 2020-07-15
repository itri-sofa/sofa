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

utils = function(){
    
}

utils.prototype.initialize=function(){
    
}

utils.prototype.storageSpaceToStorageList=function(targetStorageSpace){
    var storageSpace = parseFloat(targetStorageSpace);
    var storageList = (storageSpace * 1024) / 64;
    return Math.ceil(storageList);
}

utils.prototype.storageListToStorageSpace=function(targetStorageList){
    var storageList = parseInt(targetStorageList);
    var storageSpace = (storageList * 64) / 1024;
    storageSpace = parseInt(storageSpace * 100) / 100;
    return storageSpace;
}
