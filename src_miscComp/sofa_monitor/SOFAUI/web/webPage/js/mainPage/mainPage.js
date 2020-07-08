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

mainPage = function(){
    var myID;
    var myClass;
    var myConsts;
    var myLanguage;
    var parentElement;
    var mainElement;
    
    var WasDrawnDashboard;
    var WasDrawnPerformance;
    var WasDrawnSystemSetting;
    
    var selectDashboard;
    var selectPerformance;
    var selectSystemSetting;
    
    var iopsDetectTimer;
    var startupTimer;
    
    var buttonCTRLer;
    var popoutPanelCTRLer;
    
    this.initialize();
}

mainPage.prototype.initialize=function(){
    this.myID=webView.id.mainPage;
    this.myClass=webView.cls.mainPage;
    this.myConsts=webView.constants.mainPage;
    this.myLanguage=webView.lang.mainPage;

    this.WasDrawnDashboard = false;
    this.WasDrawnPerformance = false;
    this.WasDrawnSystemSetting = false;
    this.selectDashboard = false;
    this.selectPerformance = false;
    this.selectSystemSetting = false;
    
    this.buttonCTRLer = new jLego.objectUI.buttonCTRLer();
    webView.variables.currentFreeSpace = "--";
}

mainPage.prototype.add=function(target, option){
    this.parentElement = target;
    this.mainElement = 
            jLego.basicUI.addDiv(target, {id: this.myID.MAIN_FRAME, class: this.myClass.MAIN_FRAME});

    this.resize();
    //this.setMonitorThreshold();
    jLego.toastCTRLer.add();
    this.checkAndInit();
    
}

mainPage.prototype.checkAndInit=function(){
    webView.connection.isSOFARunning(function(Jdata){
        if(Jdata.errCode==0){
            if(Jdata.isRunning==1 || Jdata.isRunning=="1" || Jdata.isRunning=="True" || Jdata.isRunning=="true" || Jdata.isRunning=="TRUE"){
                webView.mainPage.DrawMainPage();
                webView.mainPage.prepareSystemDetection();
            }
            else{
                webView.mainPage.showStartUpAlarmDialog();
            }
        }
        else{
            jLego.toastCTRLer.addError({title: "Error", content: "Cannot access SOFA..."});
        }
    });
}

mainPage.prototype.showStartUpAlarmDialog = function(){
    if(!this.popoutPanelCTRLer) this.popoutPanelCTRLer = new jLego.objectUI.popoutPanel();
    this.popoutPanelCTRLer.add(document.body, {hasFootFrame: true, 
                                               title: this.myLanguage.POPUP_WARNING_TITLE,
                                               minHeightInPixel: 200,
                                               minWidthInPixel: 300,
                                               maxHeightInPixel: 200,
                                               maxWidthInPixel: 600,
                                               });
    var contentFrame = this.popoutPanelCTRLer.getContentFrame();  
    var description = 
            jLego.basicUI.addDiv(contentFrame, {id: jLego.func.getRandomString(), class: ''});
    $(description).text(this.myLanguage.WARNING_SOFA_STARTUP);
    $(description).css('margin-left', '10px');
    $(description).css('margin-top', '10px');
    $(description).css('text-align', 'left');
    var footerFrame = this.popoutPanelCTRLer.getFootFrame();
    var okButton =
        this.buttonCTRLer.addFreeSingleButton(footerFrame, {type: 'positive', float: 'right', top: 4, right: 5, title: this.myLanguage.OK, iconURL: jLego.func.getImgPath({category: 'buttonIcon/white', name: 'success', type: 'png'})});
    $(okButton).data('parent', this);
    $(okButton).click(function(){
        var parent = $(this).data('parent');
        parent.popoutPanelCTRLer.close();
        parent.startupSOFA();

    });
    var cancelButton =
        this.buttonCTRLer.addFreeSingleButton(footerFrame, {type: 'negtive', float: 'right', top: 4, right: 5, title: this.myLanguage.CANCEL, iconURL: jLego.func.getImgPath({category: 'buttonIcon/white', name: 'error', type: 'png'})});
    $(cancelButton).data('parent', this);
    $(cancelButton).click(function(){
        var parent = $(this).data('parent');
        parent.popoutPanelCTRLer.close();
    });
}

mainPage.prototype.startupSOFA=function(){
    webView.variables.waitingSOFAReadyLoading = new jLego.objectUI.nowLoading();
    webView.variables.waitingSOFAReadyLoading.add(document.body, {loadingText: this.myLanguage.AUTO_STARTUP_SOFA});
    webView.connection.startupSOFA(function(Jdata){
        if(Jdata.errCode==0){
            webView.mainPage.waitSOFAReady();
        }
        else{
            jLego.toastCTRLer.addError({title: webView.mainPage.myLanguage.FAILED, content: webView.mainPage.myLanguage.CANNOT_STARTUP_SOFA});
            webView.variables.waitingSOFAReadyLoading.close();
        }
    });
    
}
   
mainPage.prototype.waitSOFAReady=function(){
    this.startupTimer = setInterval(function(){
        webView.connection.isSOFARunning(function(Jdata){
            if(Jdata.errCode==0){
                if(Jdata.isRunning==1 || Jdata.isRunning=="1" || Jdata.isRunning=="True" || Jdata.isRunning=="true" || Jdata.isRunning=="TRUE"){
                    clearInterval(webView.mainPage.startupTimer);
                    webView.mainPage.DrawMainPage();
                    webView.mainPage.prepareSystemDetection();
                    webView.variables.waitingSOFAReadyLoading.close();
                }
            } 
            else{
                jLego.toastCTRLer.addError({title: "Error", content: "Cannot access SOFA..."});
            }
        });
    }, 5000);
} 
mainPage.prototype.DrawMainPage=function(){
    jLego.tabPageOrg.add(this.mainElement, {mode: 'no-border'});
    
    //add dashboard tab & page
    var callbackObject ={
        arg:{
            parent: this,
        },
        callback: function(arg){
            var parent = arg.parent;
            if(parent.selectPerformance == true){
                 webView.performancePage.stopUpdateLineChart();
            }
            parent.selectDashboard = true;
            parent.selectPerformance = false;
            parent.selectSystemSetting = false;
            var targetTabPage = jLego.tabPageOrg.getTabPage(0);
            parent.drawDashboard(targetTabPage);
            /*webView.variables.enterDashboardPageLoading = new jLego.objectUI.nowLoading();
            webView.variables.enterDashboardPageLoading.add(document.body);
            var targetTabPage = jLego.tabPageOrg.getTabPage(0);
            parent.stopAutoUpdating();
            parent.selectDashboard = true;
            parent.drawDashboard(targetTabPage);*/
        }
    }
    jLego.tabPageOrg.addTabElement(this.myLanguage.TAB_DASHBOARD, callbackObject);
    
    //add performance tab & page
    var callbackObject ={
        arg:{
            parent: this,
        },
        callback: function(arg){
            var parent = arg.parent;
            parent.selectDashboard = false;
            parent.selectPerformance = true;
            parent.selectSystemSetting = false;
            var targetTabPage = jLego.tabPageOrg.getTabPage(1);
            parent.drawPerformance(targetTabPage);
            //parent.drawSystemSetting(targetTabPage);
            /*webView.variables.enterDashboardPageLoading = new jLego.objectUI.nowLoading();
            webView.variables.enterDashboardPageLoading.add(document.body);
            var targetTabPage = jLego.tabPageOrg.getTabPage(0);
            parent.stopAutoUpdating();
            parent.selectDashboard = true;
            parent.drawDashboard(targetTabPage);*/
        }
    }
    jLego.tabPageOrg.addTabElement(this.myLanguage.TAB_PERFORMANCE, callbackObject);
    
    //add system setting tab & page
    var callbackObject ={
        arg:{
            parent: this,
        },
        callback: function(arg){
            var parent = arg.parent;
            if(parent.selectPerformance == true){
                 webView.performancePage.stopUpdateLineChart();
            }
            parent.selectDashboard = false;
            parent.selectPerformance = false;
            parent.selectSystemSetting = true;
            var targetTabPage = jLego.tabPageOrg.getTabPage(2);
            parent.drawSystemSetting(targetTabPage);
            /*webView.variables.enterDashboardPageLoading = new jLego.objectUI.nowLoading();
            webView.variables.enterDashboardPageLoading.add(document.body);
            var targetTabPage = jLego.tabPageOrg.getTabPage(0);
            parent.stopAutoUpdating();
            parent.selectDashboard = true;
            parent.drawDashboard(targetTabPage);*/
        }
    }
    jLego.tabPageOrg.addTabElement(this.myLanguage.TAB_SYSTEM_SETTING, callbackObject);
    
    this.drawLanguageButton();
    if(webView.initPageIndex == webView.constants.PAGE_DASHBOARD){
        $(jLego.tabPageOrg.getTab(0)).click();
    }
    else{
        $(jLego.tabPageOrg.getTab(0)).click();
    }
}

mainPage.prototype.drawLanguageButton=function(){
    var targetLanguageCode = webView.mainPage.myLanguage.AVAILABLE_LANGUAGE_CODE[0];
    var targetLanguageText = webView.mainPage.myLanguage.AVAILABLE_LANGUAGE_TEXT[0];
    for(var i=0; i<webView.mainPage.myLanguage.AVAILABLE_LANGUAGE_CODE.length; i++){
        if(webView.currentLanguage == webView.mainPage.myLanguage.AVAILABLE_LANGUAGE_CODE[i]){
            targetLanguageCode=webView.mainPage.myLanguage.AVAILABLE_LANGUAGE_CODE[i];
            targetLanguageText=webView.mainPage.myLanguage.AVAILABLE_LANGUAGE_TEXT[i];
        }
    }
    var languageFrame = 
            jLego.basicUI.addDiv(this.mainElement, {id: jLego.func.getRandomString(), class: this.myClass.LANGUAGE_FRAME});
    var languageText = 
            jLego.basicUI.addDiv(languageFrame, {id: jLego.func.getRandomString(), class: this.myClass.LANGUAGE_TEXT});
    $(languageText).text(targetLanguageText);
    var languageFlag = 
            jLego.basicUI.addImg(languageFrame, {id: jLego.func.getRandomString(), class: this.myClass.LANGUAGE_FLAG, src: jLego.func.getImgPath({category: 'nationalFlagUsing', name: targetLanguageCode, type: 'png'})});
    
    
    $(languageFrame).data('parent', this);
    $(languageFrame).click(function(){
        var parent = $(this).data('parent');
        parent.drawLanguageOptions();
    })
}
mainPage.prototype.drawLanguageOptions=function(){
    if(!this.popoutPanelCTRLer) this.popoutPanelCTRLer = new jLego.objectUI.popoutPanel();
    webView.mainPage.enableBlurMainElement();
    this.popoutPanelCTRLer.add(document.body, {hasFootFrame: true, 
                                               title: this.myLanguage.SELECT_LANGUAGE,
                                               minHeightInPercent: 20,
                                               minWidthInPixel: 300,
                                               maxHeightInPercent: 60,
                                               maxWidthInPixel: 600,
                                               });
    var contentFrame = this.popoutPanelCTRLer.getContentFrame(); 
    for(var i=0; i<webView.mainPage.myLanguage.AVAILABLE_LANGUAGE_CODE.length; i++){
        var isCurrentLanguage = false;
        if(webView.currentLanguage == webView.mainPage.myLanguage.AVAILABLE_LANGUAGE_CODE[i]){
            isCurrentLanguage = true;
        }
        if(isCurrentLanguage){
            var languageFrame = 
                jLego.basicUI.addDiv(contentFrame, {id: jLego.func.getRandomString(), class: this.myClass.LANGUAGE_OPTION_FRAME_SELECTED});
        }
        else{
            var languageFrame = 
                jLego.basicUI.addDiv(contentFrame, {id: jLego.func.getRandomString(), class: this.myClass.LANGUAGE_OPTION_FRAME});
                $(languageFrame).data('langaugeCode', webView.mainPage.myLanguage.AVAILABLE_LANGUAGE_CODE[i]);
                $(languageFrame).click(function(){
                   jLego.func.createCookie("sofaLanguage", $(this).data('langaugeCode'));
                   location.reload();
                });
                
        }
        var languageFlag = 
                jLego.basicUI.addImg(languageFrame, {id: jLego.func.getRandomString(), class: this.myClass.LANGUAGE_OPTION_FLAG, src: jLego.func.getImgPath({category: 'nationalFlagUsing', name: webView.mainPage.myLanguage.AVAILABLE_LANGUAGE_CODE[i], type: 'png'})});
        var languageText = 
                jLego.basicUI.addDiv(languageFrame, {id: jLego.func.getRandomString(), class: this.myClass.LANGUAGE_OPTION_TEXT});
        $(languageText).text(webView.mainPage.myLanguage.AVAILABLE_LANGUAGE_TEXT[i]);
        
    }
    this.popoutPanelCTRLer.setCloseCallback({
        arg: {},
        callback: function(){
            webView.mainPage.disableBlurMainElement();
        }
    });
}

mainPage.prototype.drawDashboard=function(target){
    if(this.WasDrawnDashboard==false){
        this.WasDrawnDashboard=true;
        webView.dashboardPage.add(target);
        //webView.dashboardPage.startUpdateMonitor();
    }
    else{
        webView.dashboardPage.resize();
        //webView.dashboardPage.startUpdateMonitor(true);
        //webView.variables.enterDashboardPageLoading.close();
    }
}

mainPage.prototype.drawSystemSetting=function(target){
    if(this.WasDrawnSystemSetting==false){
        this.WasDrawnSystemSetting=true;
        webView.settingPage.add(target);
        //webView.dashboardPage.startUpdateMonitor();
    }
    else{
        webView.settingPage.resize();
        //webView.dashboardPage.startUpdateMonitor(true);
        //webView.variables.enterDashboardPageLoading.close();
    }
}

mainPage.prototype.drawPerformance=function(target){
    
    if(this.WasDrawnPerformance==false){
        this.WasDrawnPerformance=true;
        webView.performancePage.add(target);
        //webView.dashboardPage.startUpdateMonitor();
    }
    else{
        webView.performancePage.resumeLineChart();
        webView.performancePage.resize();
        //webView.dashboardPage.startUpdateMonitor(true);
        //webView.variables.enterDashboardPageLoading.close();
    }
}

mainPage.prototype.enableBlurMainElement=function(){
    if(jLego.func.browserIs('chrome') || jLego.func.browserIs('safari') ){
        var pageClass= "." + $(this.mainElement).attr('class');
        $(pageClass).foggy({
            blurRadius: 4,          // In pixels.
            opacity: 0.8,           // Falls back to a filter for IE.
            cssFilterSupport: true  // Use "-webkit-filter" where available.
        });
    }
}
mainPage.prototype.disableBlurMainElement=function(){
    if(jLego.func.browserIs('chrome') || jLego.func.browserIs('safari') ){
        var pageClass= "." + $(this.mainElement).attr('class');
        $(pageClass).foggy(false);
    }
}

mainPage.prototype.prepareSystemDetection = function(){
    webView.variables.iopsThreshold = 9999999;
    webView.variables.freeListThreshold = 9999999;
    webView.connection.getSystemSetting(function(Jdata){
        if(Jdata.errCode==0){
            if(Jdata.iops.threshold)
                webView.variables.iopsThreshold = parseFloat(Jdata.iops.threshold);
            if(Jdata.freeList.threshold)
                webView.variables.freeListThreshold = parseFloat(Jdata.freeList.threshold);
            webView.mainPage.startDetect();
        }
    });
}
mainPage.prototype.startDetect = function(){
    webView.variables.iopsLowCount = 0;
    webView.variables.freeSpaceLowCount = 0;
    this.iopsDetectTimer = setInterval(function(){
        webView.connection.getNewIOPSTrend(function(Jdata){
            if(Jdata.errCode==0){
                var currentData = Jdata.data[Jdata.data.length - 1];
                webView.variables.currentFreeSpace = webView.utils.storageListToStorageSpace(currentData.freelist);
                if(currentData.iops < webView.variables.iopsThreshold && currentData.iops != 0){
                    webView.variables.iopsLowCount++;
                    if(webView.variables.iopsLowCount >= 2){
                        jLego.toastCTRLer.addWarning({
                            title: webView.mainPage.myLanguage.WARNING_OOPS_TITLE,
                            content: webView.mainPage.myLanguage.IOPS_TOO_LOW_CONTENT_1 + currentData.iops +  webView.mainPage.myLanguage.IOPS_TOO_LOW_CONTENT_2
                            +  webView.variables.iopsThreshold +  webView.mainPage.myLanguage.IOPS_TOO_LOW_CONTENT_3
                        });
                        //webView.variables.iopsLowCount = 0;
                    }
                }
                else{
                    webView.variables.iopsLowCount = 0;
                }
                if(currentData.freelist < webView.variables.freeListThreshold){
                    webView.variables.freeSpaceLowCount++;
                    if(webView.variables.freeSpaceLowCount >= 5){
                        jLego.toastCTRLer.addWarning({
                            title: webView.mainPage.myLanguage.WARNING_OOPS_TITLE,
                            content: webView.mainPage.myLanguage.FREESPACE_TOO_LOW_CONTENT_1 + webView.variables.currentFreeSpace +  webView.mainPage.myLanguage.FREESPACE_TOO_LOW_CONTENT_2
                            +   webView.utils.storageListToStorageSpace(webView.variables.freeListThreshold)+  webView.mainPage.myLanguage.FREESPACE_TOO_LOW_CONTENT_3
                        });
                        //webView.variables.iopsLowCount = 0;
                    }
                }
                else{
                    webView.variables.freeSpaceLowCount = 0;
                }
                //update free space in setting page
                if($(jLego.tabPageOrg.getTabPage(2)).is(":visible")){
                    webView.settingPage.updateFreeSpace();
                }
            }else{
                jLego.toastCTRLer.addError({
                    title: webView.mainPage.myLanguage.FAILED,
                    content: webView.mainPage.myLanguage.CANNOT_GET_IOPS
                });
            }
        });
    }, 2000);
}

mainPage.prototype.resize=function(){
        var marginLeft = parseInt($(this.mainElement).css('left'));
        var marginTop = parseInt($(this.mainElement).css('top'));
        $(this.mainElement).width($(this.parentElement).width() - (2 * marginLeft));
        $(this.mainElement).height($(this.parentElement).height() - (2 * marginTop));
        
}

mainPage.prototype.resizeHandler=function(){
    jLego.variables.webpageCTRLer = this;
    if(!jLego.func.isMobile()){
        window.onresize = function() {
            //resize main page
            jLego.variables.webpageCTRLer.resize();

            jLego.tabPageOrg.resize();
            if($(jLego.tabPageOrg.getTabPage(0)).is(":visible")){
                webView.dashboardPage.resize();
            }
            else if($(jLego.tabPageOrg.getTabPage(1)).is(":visible")){
                webView.performancePage.resize();
            }
            else if($(jLego.tabPageOrg.getTabPage(2)).is(":visible")){
                webView.settingPage.resize();
            }
            
            //resize objectUI
            jLego.resize();
            if(jLego.variables.webpageCTRLer.popoutPanelCTRLer!=null) jLego.variables.webpageCTRLer.popoutPanelCTRLer.resize();
            if(this.buttonCTRLer) this.buttonCTRLer.resize();
        }
    }
    else{
        window.addEventListener("orientationchange", function() {
            //resize main page
            jLego.variables.webpageCTRLer.resize();

            jLego.tabPageOrg.resize();
            if($(jLego.tabPageOrg.getTabPage(0)).is(":visible")){
                webView.dashboardPage.resize();
            }
            else if($(jLego.tabPageOrg.getTabPage(1)).is(":visible")){
                webView.performancePage.resize();
            }
            else if($(jLego.tabPageOrg.getTabPage(1)).is(":visible")){
                webView.settingPage.resize();
            }
            
            //resize objectUI
            jLego.resize();
            if(jLego.variables.webpageCTRLer.popoutPanelCTRLer!=null) jLego.variables.webpageCTRLer.popoutPanelCTRLer.resize();
            this.buttonCTRLer.resize();
        }, false);
    }
}