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

dashboardPage.prototype.addStatusPanel=function(target, Jdata){
    var statusPanel = 
            jLego.basicUI.addDiv(target, {id: jLego.func.getRandomString(), class: this.myClass.PANEL_FRAME});
    this.statusFrame = 
          jLego.basicUI.addDiv(statusPanel, {id: jLego.func.getRandomString(), class: this.myClass.STATUS_MAIN_FRAME});  
    //Add title frame
    var titleFrame = 
            jLego.basicUI.addDiv(this.statusFrame, {id: this.myID.TITLE_FRAME, class: this.myClass.TITLE_FRAME});
    var titleElement = 
            jLego.basicUI.addDiv(titleFrame, {id: this.myID.TITLE_ELEMENT, class: this.myClass.TITLE_ELEMENT});
    $(titleElement).text(this.myLanguage.TITLE);
    //Add status frame
    var statusFrame = 
            jLego.basicUI.addDiv(this.statusFrame, {id: this.myID.STATUS_FRAME, class: this.myClass.STATUS_FRAME});
    //System health
    var systemStatus = 
            jLego.statusMonitor.addIconMonitor(statusFrame, {title: this.myLanguage.SYSTEM_STATUS_TITLE, initStatus: Jdata.status});
    //vertical line
    var vLineFrame = 
            jLego.basicUI.addDiv(statusFrame, {id: jLego.func.getRandomString(), class: this.myClass.STATUS_VLINE_FRAME});
    //Group health
    var groupStatusList = [];
    for(var i=0; i<Jdata.groups.length; i++){
        var newGroupStatus = 
            jLego.statusMonitor.addIconMonitor(statusFrame, {title: this.myLanguage.GROUP_STATUS_TITLE + (i+1), initStatus: Jdata.groups[i].status});
        groupStatusList[groupStatusList.length] = newGroupStatus;
    }
    //Spare health
    var spareStatus = 
            jLego.statusMonitor.addIconMonitor(statusFrame, {title: this.myLanguage.SPARE_STATUS_TITLE, initStatus: Jdata.sparedStatus});
    
    //Space usage
    var spaceUsageTitle = webView.common.myLanguage.SPACE_USAGE_SHORT_STRING;
    spaceUsageTitle = spaceUsageTitle.replace('\!usage', jLego.func.getSpaceFromByteToTargetUnit(Jdata.usedSpace, jLego.func.getBestUnit(Jdata.physicalCapacity)));
    spaceUsageTitle = spaceUsageTitle.replace('\!uUnit', jLego.func.getBestUnit(Jdata.physicalCapacity));
    spaceUsageTitle = spaceUsageTitle.replace('\!quota', jLego.func.getSpaceFromByteToBestUnit(Jdata.physicalCapacity));
    spaceUsageTitle = spaceUsageTitle.replace('\!qUnit', jLego.func.getBestUnit(Jdata.physicalCapacity));
    var spaceUsagePercent = Jdata.usedSpace/Jdata.capacity;
    var percentage = Math.round(spaceUsagePercent * 10000) / 100;
    spaceUsageTitle = spaceUsageTitle.replace('\!percent', percentage);
    var spaceUsage = 
            jLego.statusMonitor.addPieMonitor(statusFrame, {title: this.myLanguage.SPACE_USAGE_TITLE, maxValue: Jdata.physicalCapacity, currentValue: Jdata.usedSpace, subTitle: spaceUsageTitle});
    
    //GC info
    //var percentage = Math.round(Jdata.reservedRate * 1000) / 100;
    //var gcReservedRateTitle = this.myLanguage.GC_INFO_CONTENT + percentage + "%";
    //var gcRate = 
    //        jLego.statusMonitor.addPieMonitor(statusFrame, {title: this.myLanguage.GC_INFO_TITLE, maxValue: 1, currentValue: Jdata.reservedRate,  subTitle: gcReservedRateTitle});
    
    //Physical capacity
    var bestUnit = jLego.func.getBestUnit(Jdata.physicalCapacity);
    var bestValue = jLego.func.getSpaceFromByteToTargetUnit(Jdata.physicalCapacity, bestUnit);
    var physicalCapacity =
            jLego.statusMonitor.addBigRoundMonitor(statusFrame, {title: this.myLanguage.PHYSICAL_CAPACITY_TITLE, currentValue: bestValue, currentUnit: bestUnit,  subTitle: this.myLanguage.PHYSICAL_CAPACITY_SUBTITLE});
    
    //Logical capacity
    var bestUnit = jLego.func.getBestUnit(Jdata.capacity);
    var bestValue = jLego.func.getSpaceFromByteToTargetUnit(Jdata.capacity, bestUnit);
    var logicalCapacity =
            jLego.statusMonitor.addBigRoundMonitor(statusFrame, {title: this.myLanguage.CAPACITY_TITLE, currentValue: bestValue, currentUnit: bestUnit,  subTitle: this.myLanguage.CAPACITY_SUBTITLE});


    //add line frame
    var lineFrame = 
            jLego.basicUI.addDiv(this.statusFrame, {id: jLego.func.getRandomString(), class: this.myClass.STATUS_LINE_FRAME});
    
    this.systemStatusObject={
        systemStatus: systemStatus,
        groupStatusList: groupStatusList,
        spareStatus: spareStatus,
        spaceUsage: spaceUsage,
        //gcRate: gcRate,
        physicalCapacity: physicalCapacity,
        logicalCapacity: logicalCapacity
    }
}

dashboardPage.prototype.updateStatusPanel=function(Jdata){
    //System health
    jLego.statusMonitor.updateIconMonitor(this.systemStatusObject.systemStatus, Jdata.status);
    //Group health
    for(var i=0; i<Jdata.groups.length; i++){
        if(this.systemStatusObject.groupStatusList[i]!=null)
            jLego.statusMonitor.updateIconMonitor(this.systemStatusObject.groupStatusList[i], Jdata.groups[i].status);
    }
    //Spare health
    jLego.statusMonitor.updateIconMonitor(this.systemStatusObject.spareStatus, Jdata.sparedStatus);
    //Space usage
    var spaceUsageTitle = webView.common.myLanguage.SPACE_USAGE_SHORT_STRING;
    spaceUsageTitle = spaceUsageTitle.replace('\!usage', jLego.func.getSpaceFromByteToTargetUnit(Jdata.usedSpace, jLego.func.getBestUnit(Jdata.physicalCapacity)));
    spaceUsageTitle = spaceUsageTitle.replace('\!uUnit', jLego.func.getBestUnit(Jdata.physicalCapacity));
    spaceUsageTitle = spaceUsageTitle.replace('\!quota', jLego.func.getSpaceFromByteToBestUnit(Jdata.physicalCapacity));
    spaceUsageTitle = spaceUsageTitle.replace('\!qUnit', jLego.func.getBestUnit(Jdata.physicalCapacity));
    var spaceUsagePercent = Jdata.usedSpace/Jdata.capacity;
    var percentage = Math.round(spaceUsagePercent * 10000) / 100;
    spaceUsageTitle = spaceUsageTitle.replace('\!percent', percentage);
    jLego.statusMonitor.updatePieMonitor(this.systemStatusObject.spaceUsage, {currentValue: Jdata.usedSpace, maxValue: Jdata.physicalCapacity, subTitle: spaceUsageTitle});  
    //GC info
    //var percentage = Math.round(Jdata.reservedRate * 1000) / 100;
    //var gcReservedRateTitle = this.myLanguage.GC_INFO_CONTENT + percentage + "%";
    //jLego.statusMonitor.updatePieMonitor(this.systemStatusObject.gcRate, {currentValue: Jdata.reservedRate, maxValue: 1, subTitle: gcReservedRateTitle});  
    //Physical capacity
    var bestUnit = jLego.func.getBestUnit(Jdata.physicalCapacity);
    var bestValue = jLego.func.getSpaceFromByteToTargetUnit(Jdata.physicalCapacity, bestUnit);
    jLego.statusMonitor.updateBigRoundMonitor(this.systemStatusObject.physicalCapacity, {currentValue: bestValue, currentUnit: bestUnit});  
    //Logical capacity
    var bestUnit = jLego.func.getBestUnit(Jdata.capacity);
    var bestValue = jLego.func.getSpaceFromByteToTargetUnit(Jdata.capacity, bestUnit);
    jLego.statusMonitor.updateBigRoundMonitor(this.systemStatusObject.logicalCapacity, {currentValue: bestValue, currentUnit: bestUnit});  
}



