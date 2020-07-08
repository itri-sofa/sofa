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

dashboardPage = function(){
    var myID;
    var myClass;
    var myConsts;
    var myLanguage;
    var parentElement;
    var mainElement; 
    //System Status Part
    var statusFrame;
    var systemStatusObject;
    var panelObjectList;
    
    //Group Part
    var groupFrame;
    var groupTabCTRLer;
    var groupTableCTRLerList;
    
    var nodeCardCTRLer;
    var buttonCTRLer;
    
    var infoFrame;
    var autoUpdateCTRLer;
    var popoutPanelCTRLer;
    
    this.initialize();
}

dashboardPage.prototype.initialize=function(){
    this.myID=webView.id.dashboardPage;
    this.myClass=webView.cls.dashboardPage;
    this.myConsts=webView.constants.dashboardPage;
    this.myLanguage=webView.lang.dashboardPage;
    
    this.buttonCTRLer = new jLego.objectUI.buttonCTRLer();
    this.nodeCardCTRLer = new jLego.objectUI.nodeCard();
    this.groupTableCTRLerList = [];
}

dashboardPage.prototype.add=function(target, option){
    this.parentElement = target;
    this.mainElement = 
           jLego.basicUI.addDiv(target, {id: jLego.func.getRandomString(), class: this.myClass.MAIN_FRAME}); 
    webView.constants.dashboardPage.tempTarget = this.mainElement;
    webView.connection.getSystemInfo(function(Jdata){
        if(Jdata.errCode==0){
            webView.dashboardPage.addStatusPanel(webView.constants.dashboardPage.tempTarget, Jdata);
            webView.dashboardPage.addGroupListPanel(webView.constants.dashboardPage.tempTarget, Jdata);
            webView.dashboardPage.resize();
            webView.dashboardPage.startUpdateMonitor(false);
        }
    });
}

dashboardPage.prototype.startUpdateMonitor=function(doUpdate){
    if(doUpdate==null) var doUpdate=false;
    if(doUpdate==true)  this.updateDashboard();
    if(this.autoUpdateCTRLer!=null) this.stopUpdateMonitor();
    this.autoUpdateCTRLer = setInterval(function(){
        webView.dashboardPage.updateDashboard();
    }, webView.common.myConsts.UPDATE_TIME_INTERVAL);
    
}

dashboardPage.prototype.updateDashboard=function(){
    webView.connection.getSystemInfo(function(Jdata){
        if(Jdata.errCode==0){
            webView.dashboardPage.updateStatusPanel(Jdata);
            webView.dashboardPage.updateGroupListPanel(Jdata);
        }
    });
}

dashboardPage.prototype.stopUpdateMonitor=function(){
    window.clearInterval(this.autoUpdateCTRLer);
}

dashboardPage.prototype.resize=function(){
    var newGroupFrameHeight = $(this.parentElement).height() - $(this.statusFrame).height();
    $(this.groupFrame).height(newGroupFrameHeight);
    if(this.groupTabCTRLer!=null) this.groupTabCTRLer.resize();
    
    for(var i=0; i<this.groupTableCTRLerList.length; i++){
        this.groupTableCTRLerList[i].resize();
    }
    
    if(this.popoutPanelCTRLer!=null){
        this.popoutPanelCTRLer.resize();
        webView.popupPage.resize();
    }
    this.buttonCTRLer.resize();
}  

