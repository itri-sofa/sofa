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

settingPage = function(){
    var myID;
    var myClass;
    var myConsts;
    var myLanguage;
    var parentElement;
    var mainElement; 

    
    //GC Part
    var gcFrame;
    var maxGCRatio;
    var minGCRatio;
    var previousReserveValue;
    var previousStartGCRatioValue;
    var previousStopGCRatioValue;
    var previousIOPSThresholdValue;
    var previousFreeListThresholdValue;
    //Performance Part
    var performanceFrame;
    
    var buttonCTRLer;
    
    var optionListCTRLer;
    var popoutPanelCTRLer;
    
    var freeSpaceOption;
    
    this.initialize();
}

settingPage.prototype.initialize=function(){
    this.myID=webView.id.settingPage;
    this.myClass=webView.cls.settingPage;
    this.myConsts=webView.constants.settingPage;
    this.myLanguage=webView.lang.settingPage;
    
    this.buttonCTRLer = new jLego.objectUI.buttonCTRLer();
    this.optionListCTRLer = new jLego.objectUI.optionListCTRLer();
}

settingPage.prototype.add=function(target, option){
    this.parentElement = target;
    this.mainElement = 
           jLego.basicUI.addDiv(target, {id: jLego.func.getRandomString(), class: this.myClass.MAIN_FRAME}); 
    var titleFrame = 
           jLego.basicUI.addDiv(this.mainElement, {id: jLego.func.getRandomString(), class: this.myClass.MAIN_TITLE_FRAME}); 
    $(titleFrame).text(this.myLanguage.SETTING_TITLE);
    webView.constants.settingPage.tempTarget = this.mainElement;
    webView.connection.getSystemSetting(function(Jdata){
        if(Jdata.errCode==0){
            webView.settingPage.maxGCRatio = Jdata.gc.maxRatio;
            webView.settingPage.minGCRatio = Jdata.gc.minRatio;
            webView.settingPage.previousReserveValue = Jdata.gc.reserve;
            webView.settingPage.previousStartGCRatioValue = Jdata.gc.startRatio;
            webView.settingPage.previousStopGCRatioValue = Jdata.gc.stopRatio;
            webView.settingPage.previousIOPSThresholdValue = Jdata.iops.threshold;
            webView.settingPage.previousFreeListThresholdValue = Jdata.freeList.threshold;
            webView.settingPage.addGCPanel(webView.constants.settingPage.tempTarget, Jdata);
            webView.settingPage.addIOPSPanel(webView.constants.settingPage.tempTarget, Jdata);
            webView.settingPage.resize();
        }
    });
}

settingPage.prototype.addGCPanel = function(target, Jdata){
    this.gcFrame = 
            jLego.basicUI.addDiv(target, {id: jLego.func.getRandomString(), class: this.myClass.SETTING_OUTTER_FRAME});
    var titleFrame = 
            jLego.basicUI.addDiv(this.gcFrame, {id: jLego.func.getRandomString(), class: this.myClass.SETTING_TITTLE_FRAME});
    $(titleFrame).text(this.myLanguage.SETTING_GC_SETTING_TITLE);
    //gc reserve
    /*var reserveSetting = this.optionListCTRLer.add(this.gcFrame, {
        hasIcon: true,
        iconURL: jLego.func.getImgPath({category: 'buttonIcon', name: 'recycle', type: 'png'}),
        title: this.myLanguage.SETTING_GC_PRESERVE_TITLE + " (" + Jdata.gc.minRatio +"‰ ~ " +  Jdata.gc.maxRatio + "‰)",
        type: 'input',
        inputType: 'text',
        hideButton: false,
        hideBottomBorder: true,
        defaultValue: Jdata.gc.reserve,
        buttonText: this.myLanguage.SETTING_SETRESTART_BUTTON,
        buttonClass: "warning"
    });*/
    var reserveTextList = ["5%", "10%", "25%", "50%", "75%"];
    var reserveValueList = [50, 100, 250, 500, 750];
    var defaultReserve = reserveValueList[0];
    if(Jdata.gc.reserve){
        if(parseInt(Jdata.gc.reserve) < reserveValueList[0]){
            defaultReserve = reserveValueList[0];
        }
        else if(parseInt(Jdata.gc.reserve) > reserveValueList[reserveValueList.length - 1]){
            defaultReserve = reserveValueList[reserveValueList.length - 1];
        }
        else {
            for(var i=0; i<reserveValueList.length; i++){
                if(parseInt(Jdata.gc.reserve) == reserveValueList[i]){
                    defaultReserve = reserveValueList[i];
                    break;
                }
                else if(parseInt(Jdata.gc.reserve) > reserveValueList[i] && parseInt(Jdata.gc.reserve) < reserveValueList[i + 1] ){
                    defaultReserve = reserveValueList[i];
                    break;
                }
            }
        }
        
    }
    var reserveSetting = this.optionListCTRLer.add(this.gcFrame, {
        hasIcon: true,
        iconURL: jLego.func.getImgPath({category: 'buttonIcon', name: 'recycle', type: 'png'}),
        title: this.myLanguage.SETTING_GC_PRESERVE_TITLE,
        type: 'select',
        hideButton: false,
        hideBottomBorder: true,
        defaultValue: defaultReserve,
        selectionName: reserveTextList,
        selectionValue: reserveValueList,
        buttonText: this.myLanguage.SETTING_SETRESTART_BUTTON,
        buttonClass: "warning"
    });
    var myElement = this.optionListCTRLer.getElement(reserveSetting);
    this.setupInputNumberOnly(myElement, 1000, 0, true);
    var setButton = this.optionListCTRLer.getButton(reserveSetting);
    $(setButton).data('parent', this);
    $(setButton).data('reserveSetting', reserveSetting);
    $(setButton).click(function(){
        var parent = $(this).data('parent');
        parent.showGCReserveAlarmDialog(this);
        /*jLego.toastCTRLer.addInfo({
            title: parent.myLanguage.SETTING_SUCCESS_SET_TITLE,
            content: parent.myLanguage.SETTING_SUCCESS_SET_CONTENT
        });*/
    });
    //gc start
    var gcStartSetting = this.optionListCTRLer.add(this.gcFrame, {
        hasIcon: true,
        iconURL: jLego.func.getImgPath({category: 'buttonIcon', name: 'start', type: 'png'}),
        title: this.myLanguage.SETTING_GC_START_TITLE,
        type: 'input',
        inputType: 'text',
        hideButton: false,
        hideBottomBorder: true,
        defaultValue: Jdata.gc.startRatio/10,
        buttonText: this.myLanguage.SETTING_SET_BUTTON
    });
    var myElement = this.optionListCTRLer.getElement(gcStartSetting);
    this.setupInputNumberOnly(myElement, 100, 0, true);
    var setButton = this.optionListCTRLer.getButton(gcStartSetting);
    $(setButton).data('parent', this);
    $(setButton).data('gcStartSetting', gcStartSetting);
    $(setButton).click(function(){
        var parent = $(this).data('parent');
        parent.doStartGCRatioSetting(this);
        /*jLego.toastCTRLer.addSuccess({
            title: parent.myLanguage.SETTING_SUCCESS_SET_TITLE,
            content: parent.myLanguage.SETTING_SUCCESS_SET_CONTENT
        });*/
    });
    //gc end
    var gcStopSetting = this.optionListCTRLer.add(this.gcFrame, {
        hasIcon: true,
        iconURL: jLego.func.getImgPath({category: 'buttonIcon', name: 'stop', type: 'png'}),
        title: this.myLanguage.SETTING_GC_STOP_TITLE,
        type: 'input',
        inputType: 'text',
        hideButton: false,
        hideBottomBorder: true,
        defaultValue: Jdata.gc.stopRatio/10,
        buttonText: this.myLanguage.SETTING_SET_BUTTON
    });
    var myElement = this.optionListCTRLer.getElement(gcStopSetting);
    this.setupInputNumberOnly(myElement, 100, 0, true);
    var setButton = this.optionListCTRLer.getButton(gcStopSetting);
    $(setButton).data('parent', this);
    $(setButton).data('gcStopSetting', gcStopSetting);
    $(setButton).click(function(){
        var parent = $(this).data('parent');
        parent.doStopGCRatioSetting(this);
        /*jLego.toastCTRLer.addError({
            title: parent.myLanguage.SETTING_FAIL_SET_TITLE,
            content: parent.myLanguage.SETTING_FAIL_SET_CONTENT
        });*/
    });
}

settingPage.prototype.addIOPSPanel = function(target, Jdata){
    this.performanceFrame = 
            jLego.basicUI.addDiv(target, {id: jLego.func.getRandomString(), class: this.myClass.SETTING_OUTTER_FRAME});
    var titleFrame = 
            jLego.basicUI.addDiv(this.performanceFrame, {id: jLego.func.getRandomString(), class: this.myClass.SETTING_TITTLE_FRAME});
    $(titleFrame).text(this.myLanguage.SETTING_PERFORMANCE_SETTING_TITLE);
    var iopsThresholdSetting = this.optionListCTRLer.add(this.performanceFrame, {
        hasIcon: true,
        iconURL: jLego.func.getImgPath({category: 'buttonIcon', name: 'alarm', type: 'png'}),
        title: this.myLanguage.SETTING_IOPS_ThRESHOLD_TITLE,
        type: 'input',
        inputType: 'text',
        hideButton: false,
        hideBottomBorder: true,
        defaultValue: Jdata.iops.threshold,
        buttonText: this.myLanguage.SETTING_SET_BUTTON
    });
    var myElement = this.optionListCTRLer.getElement(iopsThresholdSetting);
    this.setupInputNumberOnly(myElement, 999999, 0, true);
    var setButton = this.optionListCTRLer.getButton(iopsThresholdSetting);
    $(setButton).data('parent', this);
    $(setButton).data('iopsThresholdSetting', iopsThresholdSetting);
    $(setButton).click(function(){
        var parent = $(this).data('parent');
        parent.doIOPSThresholdSetting(this);
        /*jLego.toastCTRLer.addWarning({
            title: parent.myLanguage.SETTING_SUCCESS_SET_TITLE,
            content: parent.myLanguage.SETTING_SUCCESS_SET_CONTENT
        });*/
    });
    var freeSpace = webView.utils.storageListToStorageSpace(Jdata.freeList.threshold);
    var freeSpaceThresholdSetting = this.optionListCTRLer.add(this.performanceFrame, {
        hasIcon: true,
        iconURL: jLego.func.getImgPath({category: 'buttonIcon', name: 'preserve', type: 'png'}),
        title: this.myLanguage.SETTING_FREESPACE_THRESHOLD_TITLE,
        type: 'input',
        inputType: 'text',
        hideButton: false,
        hideBottomBorder: true,
        defaultValue: freeSpace,
        buttonText: this.myLanguage.SETTING_SET_BUTTON,
        subContent: this.myLanguage.FREESPACE + this.myLanguage.COLON + webView.variables.currentFreeSpace + " GB"
    });
    this.freeSpaceOption = freeSpaceThresholdSetting;
    var myElement = this.optionListCTRLer.getElement(freeSpaceThresholdSetting);
    this.setupInputNumberOnly(myElement, 999999, 0);
    var setButton = this.optionListCTRLer.getButton(freeSpaceThresholdSetting);
    $(setButton).data('parent', this);
    $(setButton).data('freeSpaceThresholdSetting', freeSpaceThresholdSetting);
    $(setButton).click(function(){
        var parent = $(this).data('parent');
        parent.doFreeSpaceThresholdSetting(this);
        /*jLego.toastCTRLer.addWarning({
            title: parent.myLanguage.SETTING_SUCCESS_SET_TITLE,
            content: parent.myLanguage.SETTING_SUCCESS_SET_CONTENT
        });*/
    });
}

settingPage.prototype.setupInputNumberOnly=function(element, max, min, disableFloat){
    $(element).data('max', max);
    $(element).data('min', min);
    $(element).data('currentValue', $(element).val());
    $(element).data('disableFloat', disableFloat);
    $(element).keydown(function (e) {
        // Allow: backspace, delete, tab, escape, enter and .
        if($(this).data('disableFloat')){
            if(e.keyCode == 110){
                e.preventDefault();
                return;
            }
        }
        if ($.inArray(e.keyCode, [46, 8, 9, 27, 13, 110, 190]) !== -1 ||
             // Allow: Ctrl+A, Command+A
            (e.keyCode === 65 && (e.ctrlKey === true || e.metaKey === true)) || 
             // Allow: home, end, left, right, down, up
            (e.keyCode >= 35 && e.keyCode <= 40)) {
                 // let it happen, don't do anything
                 return;
        }
        // Ensure that it is a number and stop the keypress
        
        if ((e.shiftKey || (e.keyCode < 48 || e.keyCode > 57)) && (e.keyCode < 96 || e.keyCode > 105)) {
            e.preventDefault();
        }
    });
    $(element).keyup(function() {
      var max = $(this).data('max');
      var min = $(this).data('min');
      
      if(!$.isNumeric($(this).val())){
            if($(this).val()!=""){
                $(this).val($(this).data('currentValue'));
            }
      }
      else{
          if ($(this).val() > max){
                $(this).val(max);
          }
          else if ($(this).val() < min){
            $(this).val(min);
          } 
          $(this).data('currentValue', $(this).val());
      }
    }); 
}
settingPage.prototype.updateFreeSpace=function(){
    if(this.freeSpaceOption){
        this.optionListCTRLer.updateSubContent(this.freeSpaceOption, this.myLanguage.FREESPACE + this.myLanguage.COLON + webView.variables.currentFreeSpace + " GB");
    }
}  
settingPage.prototype.resize=function(){

    this.buttonCTRLer.resize();
}  

