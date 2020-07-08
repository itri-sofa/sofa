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

settingPage.prototype.showGCReserveAlarmDialog = function(setButton){
    this.popoutPanelCTRLer = new jLego.objectUI.popoutPanel();
    webView.mainPage.enableBlurMainElement();
    this.popoutPanelCTRLer.add(document.body, {hasFootFrame: true, 
                                               title: this.myLanguage.POPUP_WARNING_TITLE,
                                               minHeightInPixel: 200,
                                               minWidthInPixel: 300,
                                               maxHeightInPixel: 200,
                                               maxWidthInPixel: 600,
                                               });
    this.popoutPanelCTRLer.setCloseCallback({
        arg: {},
        callback: function(){
            webView.mainPage.disableBlurMainElement();
        }
    });
    var contentFrame = this.popoutPanelCTRLer.getContentFrame();  
    var description = 
            jLego.basicUI.addDiv(contentFrame, {id: jLego.func.getRandomString(), class: ''});
    $(description).text(this.myLanguage.WARNING_SOFA_RESTART);
    $(description).css('margin-left', '10px');
    $(description).css('margin-top', '10px');
    $(description).css('text-align', 'left');
    var footerFrame = this.popoutPanelCTRLer.getFootFrame();
    var okButton =
        this.buttonCTRLer.addFreeSingleButton(footerFrame, {type: 'positive', float: 'right', top: 4, right: 5, title: this.myLanguage.OK, iconURL: jLego.func.getImgPath({category: 'buttonIcon/white', name: 'success', type: 'png'})});
    $(okButton).data('parent', this);
    $(okButton).data('reserveSetting', $(setButton).data('reserveSetting'));
    $(okButton).click(function(){
        var parent = $(this).data('parent');
        parent.doGCReserveSetting(this);
        /*parent.popoutPanelCTRLer.close();
        jLego.toastCTRLer.addSuccess({
            title: parent.myLanguage.SETTING_SUCCESS_SET_TITLE,
            content: parent.myLanguage.SETTING_SUCCESS_SET_CONTENT
        });*/
    });
    var cancelButton =
        this.buttonCTRLer.addFreeSingleButton(footerFrame, {type: 'negtive', float: 'right', top: 4, right: 5, title: this.myLanguage.CANCEL, iconURL: jLego.func.getImgPath({category: 'buttonIcon/white', name: 'error', type: 'png'})});
    $(cancelButton).data('parent', this);
    $(cancelButton).click(function(){
        var parent = $(this).data('parent');
        parent.popoutPanelCTRLer.close();
    });
}

settingPage.prototype.doGCReserveSetting=function(setButton){
    var reserveSetting = $(setButton).data('reserveSetting');
    var currentValue = this.optionListCTRLer.getValue(reserveSetting);
    if($.isNumeric(currentValue)){
        webView.variables.waitingSettingLoading = new jLego.objectUI.nowLoading();
        webView.variables.waitingSettingLoading.add(document.body, {loadingText: "設定中.."});
        webView.connection.setGCReserve(currentValue, function(Jdata){
            webView.variables.waitingSettingLoading.close();
            if(Jdata.errCode==0){
                webView.settingPage.previousReserveValue = currentValue;
                webView.settingPage.popoutPanelCTRLer.close();
                
                jLego.toastCTRLer.addSuccess({
                    title: webView.settingPage.myLanguage.SETTING_SUCCESS_SET_TITLE,
                    content: webView.settingPage.myLanguage.SETTING_SUCCESS_SET_CONTENT
                });
                webView.settingPage.prepareToRefreshWebPage();
            }
            else{
                webView.settingPage.optionListCTRLer.setValue(reserveSetting, webView.settingPage.previousReserveValue);
                jLego.toastCTRLer.addError({
                    title: webView.settingPage.myLanguage.SETTING_FAIL_SET_TITLE,
                    content: webView.settingPage.myLanguage.SETTING_FAIL_SET_CONTENT
                });
            }
        });
    }
    else{
        jLego.toastCTRLer.addError({
            title: this.myLanguage.SETTING_FAIL_SET_TITLE,
            content: "輸入格式錯誤！"
        });
    }
}

settingPage.prototype.prepareToRefreshWebPage=function(){
    setTimeout(function(){
        webView.variables.waitingRefreshLoading = new jLego.objectUI.nowLoading();
        webView.variables.waitingRefreshLoading.add(document.body, {loadingText: "重新開啟中.."});
        setTimeout(function(){
            webView.variables.waitingRefreshLoading.close();
            location.reload();
        }, 10000);
    }, 1000);
}

settingPage.prototype.doStartGCRatioSetting=function(setButton){
    var gcStartSetting = $(setButton).data('gcStartSetting');
    var currentValue = this.optionListCTRLer.getValue(gcStartSetting);
    if($.isNumeric(currentValue)){
        currentValue *= 10;
        if(currentValue > webView.settingPage.previousStopGCRatioValue){
            jLego.toastCTRLer.addError({
                title: webView.settingPage.myLanguage.SETTING_FAIL_SET_TITLE,
                content: "起動值不可大於關閉值。"
            });
        }
        else{
            webView.connection.setGCRatio(currentValue, webView.settingPage.previousStopGCRatioValue, function(Jdata){
                if(Jdata.errCode==0){
                    webView.settingPage.previousStartGCRatioValue = currentValue;
                    jLego.toastCTRLer.addSuccess({
                        title: webView.settingPage.myLanguage.SETTING_SUCCESS_SET_TITLE,
                        content: webView.settingPage.myLanguage.SETTING_SUCCESS_SET_CONTENT
                    });
                }
                else{
                    webView.settingPage.optionListCTRLer.setValue(gcStartSetting, webView.settingPage.previousStartGCRatioValue);
                    jLego.toastCTRLer.addError({
                        title: webView.settingPage.myLanguage.SETTING_FAIL_SET_TITLE,
                        content: webView.settingPage.myLanguage.SETTING_FAIL_SET_CONTENT
                    });
                }
            });
        }
    }
    else{
        jLego.toastCTRLer.addError({
            title: this.myLanguage.SETTING_FAIL_SET_TITLE,
            content: "輸入格式錯誤！"
        });
    }
}

settingPage.prototype.doStopGCRatioSetting=function(setButton){
    var gcStopSetting = $(setButton).data('gcStopSetting');
    var currentValue = this.optionListCTRLer.getValue(gcStopSetting);
    if($.isNumeric(currentValue)){
        currentValue *= 10;
        if(currentValue < webView.settingPage.previousStartGCRatioValue){
            jLego.toastCTRLer.addError({
                title: webView.settingPage.myLanguage.SETTING_FAIL_SET_TITLE,
                content: "關閉值不可小於起動值。"
            });
        }
        else{
            webView.connection.setGCRatio(webView.settingPage.previousStartGCRatioValue, currentValue, function(Jdata){
                if(Jdata.errCode==0){
                    webView.settingPage.previousStopGCRatioValue = currentValue;
                    jLego.toastCTRLer.addSuccess({
                        title: webView.settingPage.myLanguage.SETTING_SUCCESS_SET_TITLE,
                        content: webView.settingPage.myLanguage.SETTING_SUCCESS_SET_CONTENT
                    });
                }
                else{
                    webView.settingPage.optionListCTRLer.setValue(gcStopSetting, webView.settingPage.previousStopGCRatioValue);
                    jLego.toastCTRLer.addError({
                        title: webView.settingPage.myLanguage.SETTING_FAIL_SET_TITLE,
                        content: webView.settingPage.myLanguage.SETTING_FAIL_SET_CONTENT
                    });
                }
            });
        }
    }
    else{
        jLego.toastCTRLer.addError({
            title: this.myLanguage.SETTING_FAIL_SET_TITLE,
            content: "輸入格式錯誤！"
        });
    }
}

settingPage.prototype.doIOPSThresholdSetting=function(setButton){
    var iopsThresholdSetting = $(setButton).data('iopsThresholdSetting');
    var currentValue = this.optionListCTRLer.getValue(iopsThresholdSetting);
    if($.isNumeric(currentValue)){
        webView.connection.setIOPSThreshold(currentValue, function(Jdata){
            if(Jdata.errCode==0){
                webView.settingPage.previousIOPSThresholdValue = currentValue;
                webView.variables.iopsThreshold = parseFloat(currentValue);
                jLego.toastCTRLer.addSuccess({
                    title: webView.settingPage.myLanguage.SETTING_SUCCESS_SET_TITLE,
                    content: webView.settingPage.myLanguage.SETTING_SUCCESS_SET_CONTENT
                });
            }
            else{
                webView.settingPage.optionListCTRLer.setValue(iopsThresholdSetting, webView.settingPage.previousIOPSThresholdValue);
                jLego.toastCTRLer.addError({
                    title: webView.settingPage.myLanguage.SETTING_FAIL_SET_TITLE,
                    content: webView.settingPage.myLanguage.SETTING_FAIL_SET_CONTENT
                });
            }
        });
    }
    else{
        jLego.toastCTRLer.addError({
            title: this.myLanguage.SETTING_FAIL_SET_TITLE,
            content: "輸入格式錯誤！"
        });
    }
}

settingPage.prototype.doFreeSpaceThresholdSetting=function(setButton){
    var freeSpaceThresholdSetting = $(setButton).data('freeSpaceThresholdSetting');
    var currentValue = this.optionListCTRLer.getValue(freeSpaceThresholdSetting);
    if($.isNumeric(currentValue)){
        var freeListValue = webView.utils.storageSpaceToStorageList(currentValue);
        webView.connection.setFreeListThreshold(freeListValue, function(Jdata){
            if(Jdata.errCode==0){
                webView.settingPage.previousFreeListThresholdValue = currentValue;
                webView.variables.freeListThreshold = freeListValue;
                jLego.toastCTRLer.addSuccess({
                    title: webView.settingPage.myLanguage.SETTING_SUCCESS_SET_TITLE,
                    content: webView.settingPage.myLanguage.SETTING_SUCCESS_SET_CONTENT
                });
            }
            else{
                webView.settingPage.optionListCTRLer.setValue(freeSpaceThresholdSetting, webView.settingPage.previousFreeListThresholdValue);
                jLego.toastCTRLer.addError({
                    title: webView.settingPage.myLanguage.SETTING_FAIL_SET_TITLE,
                    content: webView.settingPage.myLanguage.SETTING_FAIL_SET_CONTENT
                });
            }
        });
    }
    else{
        jLego.toastCTRLer.addError({
            title: this.myLanguage.SETTING_FAIL_SET_TITLE,
            content: "輸入格式錯誤！"
        });
    }
}

