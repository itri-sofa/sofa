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

dashboardPage.prototype.addGroupListPanel = function(target, Jdata){
    this.groupFrame = 
            jLego.basicUI.addDiv(target, {id: jLego.func.getRandomString(), class: this.myClass.NODELIST_MAIN_FRAME});
    $(this.groupFrame).height($(target).height() - $(this.statusFrame).height());
    $(this.groupFrame).css('overflow', 'hidden');
    this.groupTabCTRLer = new jLego.objectUI.tabPage();
    this.groupTabCTRLer.add(this.groupFrame, {mode: 'no-border', permanentPage: true});
    var targetTabPage = this.groupTabCTRLer.getPermanentPage();
    $(targetTabPage).css('overflow-y', 'auto');
    var panelList = [];
    var callbackObject ={
        arg:{
            parent: this,
            panelList: panelList
        },
        callback: function(arg){
            var parent = arg.parent;
            for(var i=0; i<arg.panelList.length; i++){
                $(arg.panelList[i]).show();
            }
        }
    }
    this.groupTabCTRLer.addTabElement(this.myLanguage.GROUP_TAB_ALL, callbackObject);
    //Group 
    for(var i=0; i<Jdata.groups.length; i++){
        var newPanel = this.addGroupPanel(targetTabPage, {
                                                title: this.myLanguage.GROUP_TAB_GROUP + (i+1) + " :",
                                                groupID: i,
                                                Jdata: Jdata.groups[i]
                                          });
        panelList[panelList.length] = newPanel;
        var callbackObject ={
            arg:{
                parent: this,
                myPanel: newPanel,
                panelList: panelList
            },
            callback: function(arg){
                var parent = arg.parent;
                for(var i=0; i<arg.panelList.length; i++){
                    if(arg.panelList[i]===arg.myPanel) $(arg.panelList[i]).show();
                    else $(arg.panelList[i]).hide();
                }
            }
        }
        this.groupTabCTRLer.addTabElement(this.myLanguage.GROUP_TAB_GROUP + (i+1), callbackObject);
    }
    //Spare
    var newPanel = this.addSparePanel(targetTabPage, {
                                                title: this.myLanguage.GROUP_TAB_SPARE + " :",
                                                Jdata: Jdata.spared
                                          })
    panelList[panelList.length] = newPanel;
    var callbackObject ={
        arg:{
            parent: this,
            myPanel: newPanel,
            panelList: panelList
        },
        callback: function(arg){
            var parent = arg.parent;
            for(var i=0; i<arg.panelList.length; i++){
                if(arg.panelList[i]===arg.myPanel) $(arg.panelList[i]).show();
                else $(arg.panelList[i]).hide();
            }
        }
    }
    this.groupTabCTRLer.addTabElement(this.myLanguage.GROUP_TAB_SPARE, callbackObject);
    $(this.groupTabCTRLer.getTab(0)).click();
    
    this.panelObjectList = panelList;
}

dashboardPage.prototype.updateGroupListPanel = function(Jdata){
    for(var i=0; i<this.panelObjectList.length; i++){
        var currentPanel = this.panelObjectList[i];
        if($(currentPanel).data('type')=="group"){
            var groupData = $(currentPanel).data('groupData');
            this.updateGroupPanel(currentPanel, {groupData: groupData, 
                                                 Jdata: Jdata.groups[groupData.groupID]});
        }
        else if($(currentPanel).data('type')=="spare"){
            var spareData = $(currentPanel).data('spareData');
            this.updateSparePanel(currentPanel, {spareData: spareData, 
                                                 Jdata: Jdata.spared});
            
        }
    }
}

dashboardPage.prototype.addGroupPanel = function(target, option){
    var Jdata = option.Jdata;
    var groupPanel = 
        jLego.basicUI.addDiv(target, {id: jLego.func.getRandomString(), class: this.myClass.NODE_FRAME});
    //Title
    var titleFrame =
        jLego.basicUI.addDiv(groupPanel, {id: jLego.func.getRandomString(), class: this.myClass.NODE_TITLE});      
    $(titleFrame).text(option.title);
    //Show/Hide Button
    var showHideButton = 
        jLego.basicUI.addImg(groupPanel, {id: jLego.func.getRandomString(), class: this.myClass.NODE_SHOWHIDE_ICON, src: jLego.func.getImgPath({category: 'buttonIcon', name: 'expand', type: 'png'})});       
    //Node table
    var nodeFrame =
        jLego.basicUI.addDiv(groupPanel, {id: jLego.func.getRandomString(), class: this.myClass.NODE_TABLE_FRAME});    
    var groupTableCTRLer = new jLego.objectUI.nodeTable();
    this.groupTableCTRLerList[this.groupTableCTRLerList.length] = groupTableCTRLer;
    groupTableCTRLer.add(nodeFrame, {
        columnTitleList: this.myLanguage.GROUP_NODE_TABLE_TITLE,
        widthType: this.myLanguage.GROUP_NODE_TABLE_TITLE_WIDTH_TYPE,
        minWidthList: this.myLanguage.GROUP_NODE_TABLE_TITLE_WIDTH,
        rowCount: 1
    });
    var container;
    //Group ID
    container = groupTableCTRLer.getContainer(0, 0);
    var groupID = 
            jLego.basicUI.addDiv(container, {id: jLego.func.getRandomString(), class: ""});
    $(groupID).text(option.groupID);
    //Status
    container = groupTableCTRLer.getContainer(0, 1);
    var statusMonitor = 
        jLego.statusMonitor.addSmallIconMonitor(container, {initStatus: Jdata.status});
    //Physical Capacity
    container = groupTableCTRLer.getContainer(0, 2);
    var physicalCapacity = 
            jLego.basicUI.addDiv(container, {id: jLego.func.getRandomString(), class: ""});
    $(physicalCapacity).text(jLego.func.getSpaceFromByteToBestUnit(Jdata.physicalCapacity) + " " + jLego.func.getBestUnit(Jdata.physicalCapacity));
    //Capacity
    container = groupTableCTRLer.getContainer(0, 3);
    var capacity = 
            jLego.basicUI.addDiv(container, {id: jLego.func.getRandomString(), class: ""});
    $(capacity).text(jLego.func.getSpaceFromByteToBestUnit(Jdata.capacity) + " " + jLego.func.getBestUnit(Jdata.capacity));
    //Space Usage
    container = groupTableCTRLer.getContainer(0, 4);
    var spaceUsageTitle = webView.common.myLanguage.SPACE_USAGE_STRING;
    spaceUsageTitle = spaceUsageTitle.replace('\!usage', jLego.func.getSpaceFromByteToBestUnit(Jdata.usedSpace));
    spaceUsageTitle = spaceUsageTitle.replace('\!uUnit', jLego.func.getBestUnit(Jdata.usedSpace));
    spaceUsageTitle = spaceUsageTitle.replace('\!quota', jLego.func.getSpaceFromByteToBestUnit(Jdata.physicalCapacity));
    spaceUsageTitle = spaceUsageTitle.replace('\!qUnit', jLego.func.getBestUnit(Jdata.physicalCapacity));
    var spaceUsagePercent = Jdata.usedSpace/Jdata.physicalCapacity;
    var percentage = Math.round(spaceUsagePercent * 10000) / 100;
    spaceUsageTitle = spaceUsageTitle.replace('\!percent', percentage);
    var spaceMonitor = 
        jLego.statusMonitor.addBarMonitor(container, {title: spaceUsageTitle, initPercent: percentage});
    //Action
    //Disk Card
    var diskFrame =
        jLego.basicUI.addDiv(groupPanel, {id: jLego.func.getRandomString(), class: this.myClass.NODE_DISK_FRAME});  
    var diskCardList = [];
    for(var i=0; i<Jdata.disks.length; i++){
        var currentDisk = Jdata.disks[i];
        var newDiskCard = this.nodeCardCTRLer.addDiskCard(diskFrame, {
                                                                            width: this.myConsts.NODE_CARD_WIDTH,
                                                                            height: this.myConsts.NODE_CARD_HEIGHT,
                                                                            title: currentDisk.id
                                                                     });
        var callbackObject = {
            arg: {
                parent: this,
                diskUUID: currentDisk.id
            },
            callback: function(arg){
                var parent = arg.parent;
                webView.mainPage.enableBlurMainElement();
                parent.popoutPanelCTRLer = new jLego.objectUI.popoutPanel();
                parent.popoutPanelCTRLer.add(document.body, {hasFootFrame: true, title: parent.myLanguage.SMART_PAGE_TITLE});
                
                webView.variables.tempLoading = new jLego.objectUI.nowLoading();
                
                webView.variables.tempLoading.add(parent.popoutPanelCTRLer.getContentFrame());
                
                webView.connection.getDiskSMART(arg.diskUUID, function(Jdata){
                    webView.popupPage.addSMART(parent.popoutPanelCTRLer, Jdata);
                    webView.variables.tempLoading.close();
                });

                parent.popoutPanelCTRLer.setCloseCallback({
                    arg: {popoutPanelCTRLer: parent.popoutPanelCTRLer},
                    callback: function(arg){
                          webView.mainPage.disableBlurMainElement(); 
                          webView.popupPage.close();
                          arg.popoutPanelCTRLer=null;
                    }
                });
            }
        }
        var diskSpace = jLego.func.getSpaceFromByteToBestUnit(currentDisk.diskSize) + jLego.func.getBestUnit(currentDisk.diskSize);
        var diskStatusMonitor = 
            jLego.statusMonitor.addTwoLineIconMonitor(this.nodeCardCTRLer.getDiskCardFreeFrame(newDiskCard), {initStatus: currentDisk.status, diskSpace: diskSpace});
        var smartButton = 
            this.buttonCTRLer.addSingleButton(this.nodeCardCTRLer.getDiskCardButtonFrame(newDiskCard), {title: this.myLanguage.SMART_PAGE_TITLE, type: 'positive'}, callbackObject);
        $(smartButton).css('display', 'table');
        $(smartButton).css('margin', '2px auto');
        diskCardList[diskCardList.length] = {
            diskCardFrame: newDiskCard,
            diskStatusMonitor: diskStatusMonitor,
            smartButton: smartButton,
        };
    }
    
    //Set Show/Hide Click
    var maxHeight = $(titleFrame).height() + $(nodeFrame).height();
    $(groupPanel).css('height', maxHeight + "px");
    $(showHideButton).data('isShow', false);
    $(showHideButton).data('frameData', {
        groupPanel: groupPanel,
        titleFrame: titleFrame,
        nodeFrame: nodeFrame,
        diskFrame: diskFrame,
        showHideButton: showHideButton
    });
    $(showHideButton).click(function(){
        var isShow = $(this).data('isShow');
        var groupPanel = $(this).data('frameData').groupPanel;
        var titleFrame = $(this).data('frameData').titleFrame;
        var nodeFrame = $(this).data('frameData').nodeFrame;
        var showHideButton = $(this).data('frameData').showHideButton;
        if(isShow){ //hide panel
            var maxHeight = $(titleFrame).height() + $(nodeFrame).height();
            $(showHideButton).attr('src', jLego.func.getImgPath({category: 'buttonIcon', name: 'expand', type: 'png'}));
            $(groupPanel).animate({height: maxHeight}, 200);
        }
        else{ //show panel
            var maxHeight = $(titleFrame).height() + $(nodeFrame).height() + $(diskFrame).height();
            $(showHideButton).attr('src', jLego.func.getImgPath({category: 'buttonIcon', name: 'collapse', type: 'png'}));
            $(groupPanel).animate({height: maxHeight}, 200, function(){
                $(groupPanel).height("auto");
            });
        }
        $(this).data('isShow', !isShow);
    });
    
    var groupData = {
        groupID: option.groupID,
        titleFrame: titleFrame,
        showHideButton: showHideButton,
        groupIDFrame: groupID,
        statusMonitor: statusMonitor,
        physicalCapacityFrame: physicalCapacity,
        capacityFrame: capacity,
        spaceMonitor: spaceMonitor,
        diskCardList: diskCardList
    };
    $(groupPanel).data('type', "group");
    $(groupPanel).data('groupData', groupData);
    return groupPanel;
}

dashboardPage.prototype.updateGroupPanel = function(groupPanel, option){
    var groupData = option.groupData;
    var Jdata = option.Jdata;
    //
    jLego.statusMonitor.updateSmallIconMonitor(groupData.statusMonitor, Jdata.status);
    //
    $(groupData.physicalCapacityFrame).text(jLego.func.getSpaceFromByteToBestUnit(Jdata.physicalCapacity) + " " + jLego.func.getBestUnit(Jdata.physicalCapacity));
    //
    $(groupData.capacityFrame).text(jLego.func.getSpaceFromByteToBestUnit(Jdata.capacity) + " " + jLego.func.getBestUnit(Jdata.capacity));
    //
    var spaceUsageTitle = webView.common.myLanguage.SPACE_USAGE_STRING;
    spaceUsageTitle = spaceUsageTitle.replace('\!usage', jLego.func.getSpaceFromByteToBestUnit(Jdata.usedSpace));
    spaceUsageTitle = spaceUsageTitle.replace('\!uUnit', jLego.func.getBestUnit(Jdata.usedSpace));
    spaceUsageTitle = spaceUsageTitle.replace('\!quota', jLego.func.getSpaceFromByteToBestUnit(Jdata.physicalCapacity));
    spaceUsageTitle = spaceUsageTitle.replace('\!qUnit', jLego.func.getBestUnit(Jdata.physicalCapacity));
    var spaceUsagePercent = Jdata.usedSpace/Jdata.physicalCapacity;
    var percentage = Math.round(spaceUsagePercent * 10000) / 100;
    spaceUsageTitle = spaceUsageTitle.replace('\!percent', percentage);
    jLego.statusMonitor.updateBarMonitor(groupData.spaceMonitor, {title: spaceUsageTitle, initPercent: percentage});
    //
    for(var i=0; i<Jdata.disks.length; i++){
        var currentDisk = Jdata.disks[i];
        if(groupData.diskCardList[i]!=null){
            var currentDiskCard = groupData.diskCardList[i];
            jLego.statusMonitor.updateTwoLineIconMonitor(currentDiskCard.diskStatusMonitor, currentDisk.status);
        }
    }
}
dashboardPage.prototype.addSparePanel = function(target, option){
    var Jdata = option.Jdata;
    var sparePanel = 
        jLego.basicUI.addDiv(target, {id: jLego.func.getRandomString(), class: this.myClass.NODE_FRAME});
    //Title
    var titleFrame =
        jLego.basicUI.addDiv(sparePanel, {id: jLego.func.getRandomString(), class: this.myClass.NODE_TITLE});      
    $(titleFrame).text(option.title);
    //Disk Frame
    var diskFrame =
        jLego.basicUI.addDiv(sparePanel, {id: jLego.func.getRandomString(), class: this.myClass.NODE_DISK_FRAME});  
    var diskCardList = [];
    for(var i=0; i<Jdata.length; i++){
        var currentDisk = Jdata[i];
        var newDiskCard = this.nodeCardCTRLer.addDiskCard(diskFrame, {
                                                                            width: this.myConsts.NODE_CARD_WIDTH,
                                                                            height: this.myConsts.NODE_CARD_HEIGHT,
                                                                            title: currentDisk.id
                                                                     });
        var callbackObject = {
            arg: {
                parent: this,
                diskUUID: currentDisk.id
            },
            callback: function(arg){
                var parent = arg.parent;
                webView.mainPage.enableBlurMainElement();
                parent.popoutPanelCTRLer = new jLego.objectUI.popoutPanel();
                parent.popoutPanelCTRLer.add(document.body, {hasFootFrame: true, title: parent.myLanguage.SMART_PAGE_TITLE});
                
                webView.variables.tempLoading = new jLego.objectUI.nowLoading();
                
                webView.variables.tempLoading.add(parent.popoutPanelCTRLer.getContentFrame());
                
                webView.connection.getDiskSMART(arg.diskUUID, function(Jdata){
                    webView.popupPage.addSMART(parent.popoutPanelCTRLer, Jdata);
                    webView.variables.tempLoading.close();
                });

                parent.popoutPanelCTRLer.setCloseCallback({
                    arg: {popoutPanelCTRLer: parent.popoutPanelCTRLer},
                    callback: function(arg){
                          webView.mainPage.disableBlurMainElement(); 
                          webView.popupPage.close();
                          arg.popoutPanelCTRLer=null;
                    }
                });
            }
        }
        var diskSpace = jLego.func.getSpaceFromByteToBestUnit(currentDisk.diskSize) + jLego.func.getBestUnit(currentDisk.diskSize);
        var diskStatusMonitor = 
            jLego.statusMonitor.addTwoLineIconMonitor(this.nodeCardCTRLer.getDiskCardFreeFrame(newDiskCard), {initStatus: currentDisk.status});
        var smartButton = 
            this.buttonCTRLer.addSingleButton(this.nodeCardCTRLer.getDiskCardButtonFrame(newDiskCard), {title: "S.M.A.R.T", type: 'positive'}, callbackObject);
        $(smartButton).css('display', 'table');
        $(smartButton).css('margin', '2px auto');
        diskCardList[diskCardList.length] = {
            diskCardFrame: newDiskCard,
            diskStatusMonitor: diskStatusMonitor,
            smartButton: smartButton,
        };
    }
    
    if(Jdata.length==0) $(sparePanel).css('opacity', "0.5");
    
    var spareData = {
        titleFrame: titleFrame,
        diskCardList: diskCardList
    };
    $(sparePanel).data('type', "spare");
    $(sparePanel).data('spareData', spareData);
    return sparePanel;
}

dashboardPage.prototype.updateSparePanel = function(sparePanel, option){
    var spareData = option.spareData;
    var Jdata = option.Jdata;

    for(var i=0; i<Jdata.length; i++){
        var currentDisk = Jdata[i];
        if(spareData.diskCardList[i]!=null){
            var currentDiskCard = spareData.diskCardList[i];
            jLego.statusMonitor.updateTwoLineIconMonitor(currentDiskCard.diskStatusMonitor, currentDisk.status);
        }
    }
}