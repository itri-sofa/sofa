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

connection = function(){
    
}

connection.prototype.initialize=function(){
    
}

connection.prototype.getSystemInfo=function(onReceiveCallback){
    $.ajax({
        url: webView.url.common.getSystemInfo,
        type: "GET",
        dataType: "json",
        async: true,
        success: function(Jdata) {
              onReceiveCallback(Jdata);
        },
        error: function() {
            jLego.statusMonitor.showWarning({title: "Get System Info Error!"});
            onReceiveCallback({errCode: 1});
        }        
    });
}

connection.prototype.getDiskSMART=function(diskUUID, onReceiveCallback){
    $.ajax({
        url: webView.url.common.getDiskSMART,
        type: "GET",
        dataType: "json",
        async: true,
        data: {
            diskUUID: diskUUID
        },
        success: function(Jdata) {
              onReceiveCallback(Jdata);
        },
        error: function() {
            jLego.statusMonitor.showWarning({title: "Get Disk SMART Error!"});
            onReceiveCallback({errCode: 1});
        }        
    });
}

connection.prototype.getSystemSetting=function(onReceiveCallback){
    $.ajax({
        url: webView.url.common.getSystemProperties,
        type: "GET",
        dataType: "json",
        async: true,
        data: {
        },
        success: function(Jdata) {
              onReceiveCallback(Jdata);
        },
        error: function() {
            jLego.statusMonitor.showWarning({title: "Get System Properties Error!"});
            onReceiveCallback({errCode: 1});
        }        
    });
}

connection.prototype.getIOPSTrend=function(onReceiveCallback){
    $.ajax({
        url: webView.url.common.getIOPSTrend,
        type: "GET",
        dataType: "json",
        async: true,
        data: {
            pointCount: 60
        },
        success: function(Jdata) {
              onReceiveCallback(Jdata);
        },
        error: function() {
            jLego.statusMonitor.showWarning({title: "Get IOPS Error!"});
            onReceiveCallback({errCode: 1});
        }        
    });
}

connection.prototype.getNewIOPSTrend=function(onReceiveCallback){
    $.ajax({
        url: webView.url.common.getIOPSTrend,
        type: "GET",
        dataType: "json",
        async: true,
        data: {
            pointCount: 1
        },
        success: function(Jdata) {
              onReceiveCallback(Jdata);
        },
        error: function() {
            jLego.statusMonitor.showWarning({title: "Get IOPS Error!"});
            onReceiveCallback({errCode: 1});
        }        
    });
}

connection.prototype.isSOFARunning=function(onReceiveCallback){
    $.ajax({
        url: webView.url.common.isSOFARunning,
        type: "GET",
        dataType: "json",
        async: true,
        data: {
        },
        success: function(Jdata) {
              onReceiveCallback(Jdata);
        },
        error: function() {
            jLego.statusMonitor.showWarning({title: "Get SOFA Running Status Error!"});
            onReceiveCallback({errCode: 1});
        }        
    });
}

connection.prototype.startupSOFA=function(onReceiveCallback){
    $.ajax({
        url: webView.url.common.startupSOFA,
        type: "GET",
        dataType: "json",
        async: true,
        data: {
        },
        success: function(Jdata) {
              onReceiveCallback(Jdata);
        },
        error: function() {
            jLego.statusMonitor.showWarning({title: "Startup SOFA Error!"});
            onReceiveCallback({errCode: 1});
        }        
    });
}

connection.prototype.setIOPSThreshold=function(iopsThreshold, onReceiveCallback){
    $.ajax({
        url: webView.url.common.setIOPSThreshold,
        type: "GET",
        dataType: "json",
        async: true,
        data: {
            iopsThreshold: iopsThreshold
        },
        success: function(Jdata) {
              onReceiveCallback(Jdata);
        },
        error: function() {
            jLego.statusMonitor.showWarning({title: "Set IOPS Threshold Error!"});
            onReceiveCallback({errCode: 1});
        }        
    });
}

connection.prototype.setFreeListThreshold=function(freeListThreshold, onReceiveCallback){
    $.ajax({
        url: webView.url.common.setFreeListThreshold,
        type: "GET",
        dataType: "json",
        async: true,
        data: {
            freeListThreshold: freeListThreshold
        },
        success: function(Jdata) {
              onReceiveCallback(Jdata);
        },
        error: function() {
            jLego.statusMonitor.showWarning({title: "Set FreeSpace Threshold Error!"});
            onReceiveCallback({errCode: 1});
        }        
    });
}

connection.prototype.setGCRatio=function(startRatio, stopRatio, onReceiveCallback){
    $.ajax({
        url: webView.url.common.setGCRatio,
        type: "GET",
        dataType: "json",
        async: true,
        data: {
            maxGCRatio: stopRatio,
            minGCRatio: startRatio
        },
        success: function(Jdata) {
              onReceiveCallback(Jdata);
        },
        error: function() {
            jLego.statusMonitor.showWarning({title: "Set GC Ratio Error!"});
            onReceiveCallback({errCode: 1});
        }        
    });
}

connection.prototype.setGCReserve=function(gcReserve, onReceiveCallback){
    $.ajax({
        url: webView.url.common.setGCReserve,
        type: "GET",
        dataType: "json",
        async: true,
        data: {
            gcReserve: gcReserve
        },
        success: function(Jdata) {
              onReceiveCallback(Jdata);
        },
        error: function() {
            jLego.statusMonitor.showWarning({title: "Set GC Reserve Error!"});
            onReceiveCallback({errCode: 1});
        }        
    });
}
