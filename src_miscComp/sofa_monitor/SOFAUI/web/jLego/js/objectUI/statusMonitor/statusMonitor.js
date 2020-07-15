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

jLego.objectUI.statusMonitor=function(){
    var myID;
    var myClass;
    var myConsts;
    
    var monitorList;
    var warningTimer;
    var warningIcon;
    
    this.initialize();
}

jLego.objectUI.statusMonitor.prototype.initialize=function(){
    this.myID=jLego.objectUI.id.statusMonitor;
    this.myClass=jLego.objectUI.cls.statusMonitor;
    this.myConsts=jLego.objectUI.constants.statusMonitor;
    this.myLanguage = jLego.objectUI.lang.statusMonitor;
    this.monitorList=[];
}

jLego.objectUI.statusMonitor.prototype.setThreshold = function(option){
    if(option==null)    return null;
    if(option.spaceThreshold==null) return null;
    if(option.spaceThreshold.warn!=null)    this.myConsts.BAR_WARN_PERCENTAGE = option.spaceThreshold.warn;
    if(option.spaceThreshold.alert!=null)    this.myConsts.BAR_ERROR_PERCENTAGE = option.spaceThreshold.alert;
}

jLego.objectUI.statusMonitor.prototype.showWarning = function(option){
    if(option==null)    option={title: "Something is wrong."};
    if(this.warningIcon!=null){
        $(this.warningIcon).remove();
        this.warningIcon=null;
        clearTimeout(this.warningTimer);
    }
    var statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'warning', type: 'png'});
    this.warningIcon =
            jLego.basicUI.addImg(document.body, {id: "warningIcon", class: this.myClass.STATUSMONITOR_ICONWARNING, src: statusImage});
    $(this.warningIcon).attr('title', option.title);
    $(this.warningIcon).tooltip();
    this.warningTimer = setTimeout(function(){
        $(jLego.statusMonitor.warningIcon).remove();
        jLego.statusMonitor.warningIcon=null;
        clearTimeout(jLego.statusMonitor.warningTimer);
    }, 10000);
}
jLego.objectUI.statusMonitor.prototype.addIconMonitor=function(target, option){
    //check option
    if(target==null) return null;
    
    if(option==null) var option={title: null, initStatus: 'close'};
    else{
        if(option.title==null)   option.title=null;
        if(option.initStatus==null)   option.initStatus='close';
    }
    //draw monitor
    var defIndex = this.monitorList.length;
    var defID = this.myID.STATUSMONITOR_MONITORFRAME + defIndex;
    var statusImage, status, statusText, statusClass;
    if(option.initStatus=="ok"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'ok', type: 'png'});
        status = 'ok';
        statusText = this.myLanguage.OK;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_OK;
    }
    else if(option.initStatus=="degraded"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'degraded', type: 'png'});
        status = 'degraded';
        statusText = this.myLanguage.DEGRADED;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_DEGRADED;
    }
    else if(option.initStatus=="init"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'init', type: 'png'});
        status = 'init';
        statusText = this.myLanguage.INIT;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_INIT;
    }
    else if(option.initStatus=="close"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'close', type: 'png'});
        status = 'close';
        statusText = this.myLanguage.CLOSE;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
    }
    else if(option.initStatus=="removed"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'removed', type: 'png'});
        status = 'removed';
        statusText = this.myLanguage.REMOVED;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
    }
    else if(option.initStatus=="read-only"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'readOnly', type: 'png'});
        status = 'read-only';
        statusText = this.myLanguage.READONLY;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
    }
    else if(option.initStatus=="failed"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'failed', type: 'png'});
        status = 'failed';
        statusText = this.myLanguage.FAILED;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_FAILED;
    }
    else if(option.initStatus=="removed"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'removed', type: 'png'});
        status = 'removed';
        statusText = this.myLanguage.CLOSE;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
    }
    else if(option.initStatus=="rebuilding"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'init', type: 'png'});
        status = 'rebuilding';
        statusText = this.myLanguage.REBUILDING;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_INIT;
    }
    else{
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'unknown', type: 'png'});
        status = 'unknown';
        statusText = this.myLanguage.UNKNOWN;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
    }
    var mainElement = 
            jLego.basicUI.addDiv(target, {id: defID, class: this.myClass.STATUSMONITOR_ICONMONITOR_FRAME});
    var iconElement =
            jLego.basicUI.addImg(mainElement, {id: defID + "_image", class: this.myClass.STATUSMONITOR_ICONMONITOR_ICON, src: statusImage});
    if(status == 'rebuilding') $(iconElement).addClass(this.myClass.STATUSMONITOR_SPIN);
    else $(iconElement).removeClass(this.myClass.STATUSMONITOR_SPIN);
    var titleElement =
            jLego.basicUI.addDiv(mainElement, {id: defID + "_title", class: this.myClass.STATUSMONITOR_ICONMONITOR_TITLE});
    $(titleElement).text(option.title);
    
    var statusTextElement =
            jLego.basicUI.addDiv(mainElement, {id: defID + "_statusText", class: statusClass});
    $(statusTextElement).text(statusText);
    
    this.monitorList[defIndex] = mainElement;
    $(this.monitorList[defIndex]).data('status', status);
    $(this.monitorList[defIndex]).data('iconElement', iconElement);
    $(this.monitorList[defIndex]).data('statusTextElement', statusTextElement);
    $(this.monitorList[defIndex]).data('status', status);
    
    return mainElement;
}

jLego.objectUI.statusMonitor.prototype.updateIconMonitor=function(monitor, targetStatus, targetStatusText){
    var statusImage, status, statusText, statusClass;
    var iconElement = $(monitor).data('iconElement');
    var statusTextElement = $(monitor).data('statusTextElement');
    if(targetStatus=="ok"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'ok', type: 'png'});
        status = 'ok';
        statusText = this.myLanguage.OK;
        if(targetStatusText!=null) statusText = targetStatusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_OK;
    }
    else if(targetStatus=="degraded"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'degraded', type: 'png'});
        status = 'degraded';
        statusText = this.myLanguage.DEGRADED;
        if(targetStatusText!=null) statusText = targetStatusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_DEGRADED;
    }
    else if(targetStatus=="init"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'init', type: 'png'});
        status = 'init';
        statusText = this.myLanguage.INIT;
        if(targetStatusText!=null) statusText = targetStatusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_INIT;
    }
    else if(targetStatus=="close"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'close', type: 'png'});
        status = 'close';
        statusText = this.myLanguage.CLOSE;
        if(targetStatusText!=null) statusText = targetStatusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
    }
    else if(targetStatus=="removed"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'removed', type: 'png'});
        status = 'removed';
        statusText = this.myLanguage.CLOSE;
        if(targetStatusText!=null) statusText = targetStatusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
    }
    else if(targetStatus=="read-only"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'readOnly', type: 'png'});
        status = 'read-only';
        statusText = this.myLanguage.READONLY;
        if(targetStatusText!=null) statusText = targetStatusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
    }
    else if(targetStatus=="failed"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'failed', type: 'png'});
        status = 'failed';
        statusText = this.myLanguage.FAILED;
        if(targetStatusText!=null) statusText = targetStatusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_FAILED;
    }
    else if(targetStatus=="removed"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'removed', type: 'png'});
        status = 'removed';
        statusText = this.myLanguage.CLOSE;
        if(targetStatusText!=null) statusText = targetStatusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
    }
    else if(targetStatus=="rebuilding"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'init', type: 'png'});
        status = 'rebuilding';
        statusText = this.myLanguage.REBUILDING;
        if(targetStatusText!=null) statusText = targetStatusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_INIT;
    }
    else if(targetStatus=="unknown"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'unknown', type: 'png'});
        status = 'unknown';
        statusText = this.myLanguage.UNKNOWN;
        if(targetStatusText!=null) statusText = targetStatusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
    }
    else return null;
    $(iconElement).attr('src', statusImage);
    if(status == 'rebuilding') $(iconElement).addClass(this.myClass.STATUSMONITOR_SPIN);
    else $(iconElement).removeClass(this.myClass.STATUSMONITOR_SPIN);
    $(statusTextElement).attr('class', statusClass);
    $(statusTextElement).text(statusText);
}

jLego.objectUI.statusMonitor.prototype.addPieMonitor=function(target, option){
    //check option
    if(target==null) return null;
    
    if(option==null) var option={title: null, subtitle: null, maxValue: 100, currentValue: 0, subTitle: '0 / 100 TB (0%)'};
    else{
        if(option.title==null)   option.title=null;
        if(option.subtitle==null)   option.subtitle=null;
        if(option.maxValue==null)   option.maxValue=100;
        if(option.currentValue==null)   option.currentValue=0;
        if(option.subTitle==null)   option.subTitle='0 / 100 TB (0%)';
    }
    //draw monitor
    var defIndex = this.monitorList.length;
    var defID = this.myID.STATUSMONITOR_MONITORFRAME + defIndex;
    var statusText, statusClass;
    
    statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_PIE_STATUSTEXT;
    statusText = option.subTitle;
    
    var mainElement = 
            jLego.basicUI.addDiv(target, {id: defID, class: this.myClass.STATUSMONITOR_ICONMONITOR_FRAME});
    var pieBackgroundElement = 
            jLego.basicUI.addDiv(mainElement, {id: defID + "_pieBackground", class: this.myClass.STATUSMONITOR_PIEBACKGROUND_FRAME});
    var pieElement = 
            jLego.basicUI.addCanvas(pieBackgroundElement);
    $(pieElement).width($(pieBackgroundElement).width());
    $(pieElement).height($(pieBackgroundElement).height());
    var pieData = [
        {
                value: option.currentValue,
                color:"#006CCF"
        },
        {
                value: (option.maxValue - option.currentValue),
                color: "rgba(0,0,0,0)"
        }
    ];
    var ctx=pieElement.getContext("2d");
    var pie = new Chart(ctx).Pie(pieData);

    var titleElement =
            jLego.basicUI.addDiv(mainElement, {id: defID + "_title", class: this.myClass.STATUSMONITOR_ICONMONITOR_TITLE});
    $(titleElement).text(option.title);
    var statusTextElement =
            jLego.basicUI.addDiv(mainElement, {id: defID + "_statusText", class: statusClass});
    $(statusTextElement).text(statusText);
    
    this.monitorList[defIndex] = mainElement;
    $(this.monitorList[defIndex]).data('pieElement', pieElement);
    $(this.monitorList[defIndex]).data('pie', pie);
    $(this.monitorList[defIndex]).data('maxValue', option.maxValue);
    $(this.monitorList[defIndex]).data('currentValue', option.currentValue);
    $(this.monitorList[defIndex]).data('statusTextElement', statusTextElement);
    return mainElement;
}

jLego.objectUI.statusMonitor.prototype.updatePieMonitor=function(monitor, option){
    if(option==null)    return null;
    else if(option.currentValue==null || option.maxValue==null || option.subTitle==null) return null;
    //update pie
    var pie = $(monitor).data('pie');
    pie.segments[0].value = option.currentValue;
    pie.segments[1].value = option.maxValue - option.currentValue;
    pie.update();
    //update status text
    var statusTextElement = $(monitor).data('statusTextElement');
    $(statusTextElement).text(option.subTitle);
}

jLego.objectUI.statusMonitor.prototype.addBarMonitor=function(target, option){
    //check option
    if(target==null) return null;
    
    if(option==null) var option={title: null, initPercent: 0};
    else{
        if(option.title==null)   option.title=null;
        if(option.initPercent==null)   option.initPercent=0;
    }
    //draw monitor
    var defIndex = this.monitorList.length;
    var defID = this.myID.STATUSMONITOR_MONITORFRAME + defIndex;
    var statusPercentage = 0, statusClass = this.myClass.STATUSMONITOR_BARMONITOR_BARELEMENT, statusBarColor=this.myConsts.BAR_COLOR_SAFE;

    if(option.initPercent<this.myConsts.BAR_WARN_PERCENTAGE){
        statusBarColor=this.myConsts.BAR_COLOR_SAFE;
        if(option.initPercent<0) statusPercentage = 0;
        else statusPercentage = option.initPercent;
    }
    else if(option.initPercent>=this.myConsts.BAR_WARN_PERCENTAGE && option.initPercent<this.myConsts.BAR_ERROR_PERCENTAGE){
        
        statusPercentage = option.initPercent;
        statusBarColor=this.myConsts.BAR_COLOR_WARN;
    }
    else if(option.initPercent>=this.myConsts.BAR_ERROR_PERCENTAGE){
        statusBarColor=this.myConsts.BAR_COLOR_ERROR;
        if(option.initPercent>100) statusPercentage = 100;
        else statusPercentage = option.initPercent;
    }
    var mainElement = 
            jLego.basicUI.addDiv(target, {id: defID, class: this.myClass.STATUSMONITOR_BARMONITOR_FRAME});
    var barFrame =
            jLego.basicUI.addDiv(mainElement, {id: defID + "_barFrame", class: this.myClass.STATUSMONITOR_BARMONITOR_BARFRAME});
    var barElement =
            jLego.basicUI.addDiv(barFrame, {id: defID + "_barElement", class: this.myClass.STATUSMONITOR_BARMONITOR_BARELEMENT});
    var titleElement =
            jLego.basicUI.addDiv(mainElement, {id: defID + "_title", class: this.myClass.STATUSMONITOR_BARMONITOR_TITLE});
    $(titleElement).text(option.title);
    if(option.barHeight!=null){
         var outterHeight = option.barHeight + 5;
         $(mainElement).height(outterHeight * 2);
         $(barFrame).height(option.barHeight);
         $(barElement).height(option.barHeight);
         $(titleElement).height(outterHeight);
    }
    if(option.barWidth!=null){
         $(mainElement).width(option.barWidth + 20);
         $(barFrame).width(option.barWidth);
         $(barElement).width(option.barWidth);
         $(titleElement).width(option.barWidth);
    }
    if(option.barBorderRadius!=null){
        $(barFrame).css('border-radius', option.barBorderRadius+'px');
         $(barElement).css('border-radius', option.barBorderRadius+'px');
    }
    var newWidth = $(barFrame).width() * statusPercentage/100;
    
    $(barElement).animate({backgroundColor: statusBarColor, width: newWidth, direction: 'right'}, 1500);

    this.monitorList[defIndex] = mainElement;
    $(this.monitorList[defIndex]).data('statusPercentage', statusPercentage);
    $(this.monitorList[defIndex]).data('barFrame', barFrame);
    $(this.monitorList[defIndex]).data('barElement', barElement);
    $(this.monitorList[defIndex]).data('titleElement', titleElement);
    
    return mainElement;
}

jLego.objectUI.statusMonitor.prototype.updateBarMonitor=function(monitor, option){
    if(option==null)    return null;
    else if(option.title==null || option.percentage==null) return null;
    var statusBarColor, statusPercentage;
    if(option.percentage<this.myConsts.BAR_WARN_PERCENTAGE){
        statusBarColor=this.myConsts.BAR_COLOR_SAFE;
        if(option.percentage<0) statusPercentage = 0;
        else statusPercentage = option.percentage;
    }
    else if(option.percentage>=this.myConsts.BAR_WARN_PERCENTAGE && option.percentage<this.myConsts.BAR_ERROR_PERCENTAGE){
        
        statusPercentage = option.percentage;
        statusBarColor=this.myConsts.BAR_COLOR_WARN;
    }
    else if(option.percentage>=this.myConsts.BAR_ERROR_PERCENTAGE){
        statusBarColor=this.myConsts.BAR_COLOR_ERROR;
        if(option.percentage>100) statusPercentage = 100;
        else statusPercentage = option.percentage;
    }
    var barFrame = $(monitor).data('barFrame');
    var barElement = $(monitor).data('barElement');
    var titleElement = $(monitor).data('titleElement');
    var newWidth = $(barFrame).width() * statusPercentage/100;
    
    $(barElement).animate({backgroundColor: statusBarColor, width: newWidth, direction: 'right'}, 1500);
    $(titleElement).text(option.title);
}

jLego.objectUI.statusMonitor.prototype.addArrayIconMonitor=function(target, option){
    //check option
    if(target==null) return null;
    
    if(option==null) var option={title: null, monitorType: ['ok', 'degraded', 'close'], initValueList: [0, 0, 0]};
    else{
        if(option.title==null)   option.title=null;
        if(option.monitorType==null || option.initValueList==null){
            option.monitorType = ['ok', 'degraded', 'close'];
            option.initValueList = [0, 0, 0];
        }
    }
    var monitorCount=0, 
        indexOK=jQuery.inArray('ok', option.monitorType), 
        indexDegraded=jQuery.inArray('degraded', option.monitorType),
        indexClose=jQuery.inArray('close', option.monitorType),
        indexInit=jQuery.inArray('init', option.monitorType);
    if(indexOK!=-1) monitorCount++;
    if(indexDegraded!=-1) monitorCount++;
    if(indexClose!=-1) monitorCount++;
    if(indexInit!=-1) monitorCount++;
    
    //draw monitor
    var defIndex = this.monitorList.length;
    var defID = this.myID.STATUSMONITOR_MONITORFRAME + defIndex;
    var statusImage, status, statusText, statusClass;
    
    var mainElement = 
            jLego.basicUI.addDiv(target, {id: defID, class: this.myClass.STATUSMONITOR_ARRAYMONITOR_FRAME});
    $(mainElement).width($(mainElement).width() * monitorCount);
    
    var iconElementOK = null, iconElementDegraded = null, iconElementClose = null, iconElementInit = null;
    var statusTextElementOK = null, statusTextElementDegraded = null, statusTextElementClose = null, statusTextElementInit = null;
    if(indexOK!=-1){
        iconElementOK =
            jLego.basicUI.addImg(mainElement, {id: defID + "_imageOK", class: this.myClass.STATUSMONITOR_ARRAYMONITOR_ICON, src: jLego.func.getImgPath({category: 'statusSmall', name: 'ok', type: 'png'})});
        $(iconElementOK).attr('title', this.myLanguage.OK);
        $(iconElementOK).tooltip();
        statusTextElementOK =
            jLego.basicUI.addDiv(mainElement, {id: defID + "_statusTextOK", class: this.myClass.STATUSMONITOR_ARRAYMONITOR_TEXT});
        $(statusTextElementOK).css('color', this.myConsts.TEXT_COLOR_OK);
        $(statusTextElementOK).text(option.initValueList[indexOK]);
    }
    if(indexInit!=-1){
        iconElementInit =
            jLego.basicUI.addImg(mainElement, {id: defID + "_imageInit", class: this.myClass.STATUSMONITOR_ARRAYMONITOR_ICON, src: jLego.func.getImgPath({category: 'statusSmall', name: 'init', type: 'png'})});
        $(iconElementInit).attr('title', this.myLanguage.INIT);
        $(iconElementInit).tooltip();
        statusTextElementInit =
            jLego.basicUI.addDiv(mainElement, {id: defID + "_statusTextInit", class: this.myClass.STATUSMONITOR_ARRAYMONITOR_TEXT});
        $(statusTextElementInit).css('color', this.myConsts.TEXT_COLOR_INIT);
        $(statusTextElementInit).text(option.initValueList[indexInit]);
        
        if(parseInt(option.initValueList[indexInit])!=0) $(iconElementInit).addClass(this.myClass.STATUSMONITOR_SPIN);
        else $(iconElementInit).removeClass(this.myClass.STATUSMONITOR_SPIN);
    }
    if(indexDegraded!=-1){
        iconElementDegraded =
            jLego.basicUI.addImg(mainElement, {id: defID + "_imageDegraded", class: this.myClass.STATUSMONITOR_ARRAYMONITOR_ICON, src: jLego.func.getImgPath({category: 'statusSmall', name: 'degraded', type: 'png'})});
        $(iconElementDegraded).attr('title', this.myLanguage.DEGRADED);
        $(iconElementDegraded).tooltip();
        statusTextElementDegraded =
            jLego.basicUI.addDiv(mainElement, {id: defID + "_statusTextDegraded", class: this.myClass.STATUSMONITOR_ARRAYMONITOR_TEXT});
        $(statusTextElementDegraded).css('color', this.myConsts.TEXT_COLOR_DEGRADED);
        $(statusTextElementDegraded).text(option.initValueList[indexDegraded]);
    }
    if(indexClose!=-1){
        iconElementClose =
            jLego.basicUI.addImg(mainElement, {id: defID + "_imageClose", class: this.myClass.STATUSMONITOR_ARRAYMONITOR_ICON, src: jLego.func.getImgPath({category: 'statusSmall', name: 'close', type: 'png'})});
        $(iconElementClose).attr('title', this.myLanguage.CLOSE);
        $(iconElementClose).tooltip();
        statusTextElementClose =
            jLego.basicUI.addDiv(mainElement, {id: defID + "_statusTextClose", class: this.myClass.STATUSMONITOR_ARRAYMONITOR_TEXT});
        $(statusTextElementClose).css('color', this.myConsts.TEXT_COLOR_CLOSE);
        $(statusTextElementClose).text(option.initValueList[indexClose]);
    }
    
    this.monitorList[defIndex] = mainElement;
    
    $(this.monitorList[defIndex]).data('iconElementOK', iconElementOK);
    $(this.monitorList[defIndex]).data('statusTextElementOK', statusTextElementOK);
    $(this.monitorList[defIndex]).data('iconElementDegraded', iconElementDegraded);
    $(this.monitorList[defIndex]).data('statusTextElementDegraded', statusTextElementDegraded);
    $(this.monitorList[defIndex]).data('iconElementClose', iconElementClose);
    $(this.monitorList[defIndex]).data('statusTextElementClose', statusTextElementClose);
    $(this.monitorList[defIndex]).data('iconElementInit', iconElementInit);
    $(this.monitorList[defIndex]).data('statusTextElementInit', statusTextElementInit);
    return mainElement;
}

jLego.objectUI.statusMonitor.prototype.updateArrayIconMonitor=function(monitor, option){
    if(option==null)    return null;
    else if(option.monitorType==null || option.valueList==null) return null;
    var monitorCount=0, 
        indexOK=jQuery.inArray('ok', option.monitorType), 
        indexDegraded=jQuery.inArray('degraded', option.monitorType),
        indexClose=jQuery.inArray('close', option.monitorType),
        indexInit=jQuery.inArray('init', option.monitorType);
    if(indexOK!=-1) monitorCount++;
    if(indexDegraded!=-1) monitorCount++;
    if(indexClose!=-1) monitorCount++;
    if(indexOK!=-1){
        var statusTextElementOK = $(monitor).data('statusTextElementOK');
        $(statusTextElementOK).text(option.valueList[indexOK]);
    }
    if(indexDegraded!=-1){
        var statusTextElementDegraded = $(monitor).data('statusTextElementDegraded');
        $(statusTextElementDegraded).text(option.valueList[indexDegraded]);
    }
    if(indexInit!=-1){
        var statusTextElementInit = $(monitor).data('statusTextElementInit');
        var iconElementInit = $(monitor).data('iconElementInit');
        $(statusTextElementInit).text(option.valueList[indexInit]);
        if(parseInt(option.valueList[indexInit])!=0) $(iconElementInit).addClass(this.myClass.STATUSMONITOR_SPIN);
        else $(iconElementInit).removeClass(this.myClass.STATUSMONITOR_SPIN);
    }
    if(indexClose!=-1){
        var statusTextElementClose = $(monitor).data('statusTextElementClose');
        $(statusTextElementClose).text(option.valueList[indexClose]);
    }
}

jLego.objectUI.statusMonitor.prototype.addSmallIconMonitor=function(target, option){
    //check option
    if(target==null) return null;
    
    if(option==null) var option={initStatus: 'unknown'};
    else{
        if(option.initStatus==null)   option.initStatus='unknown';
    }
    //draw monitor
    var defIndex = this.monitorList.length;
    var defID = this.myID.STATUSMONITOR_MONITORFRAME + defIndex;
    var statusImage, status, statusText, statusColor, statusClass;
    if(option.initStatus=="ok"){
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'ok', type: 'png'});
        status = 'ok';
        statusText = this.myLanguage.OK;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_OK;
        statusColor = this.myConsts.TEXT_COLOR_OK;
    }
    else if(option.initStatus=="degraded"){
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'degraded', type: 'png'});
        status = 'degraded';
        statusText = this.myLanguage.DEGRADED;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_DEGRADED;
        statusColor = this.myConsts.TEXT_COLOR_DEGRADED;
    }
    else if(option.initStatus=="init"){
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'init', type: 'png'});
        status = 'init';
        statusText = this.myLanguage.INIT;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_INIT;
        statusColor = this.myConsts.TEXT_COLOR_INIT;
    }
    else if(option.initStatus=="close"){
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'close', type: 'png'});
        status = 'close';
        statusText = this.myLanguage.CLOSE;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
        statusColor = this.myConsts.TEXT_COLOR_CLOSE;
    }
    else if(option.initStatus=="removed"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'removed', type: 'png'});
        status = 'removed';
        statusText = this.myLanguage.CLOSE;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
    }
    else if(option.initStatus=="read-only"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'readOnly', type: 'png'});
        status = 'read-only';
        statusText = this.myLanguage.READONLY;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
    }
    else if(option.initStatus=="failed"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'failed', type: 'png'});
        status = 'failed';
        statusText = this.myLanguage.FAILED;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_FAILED;
    }
    else if(option.initStatus=="removed"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'removed', type: 'png'});
        status = 'removed';
        statusText = this.myLanguage.CLOSE;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
    }
    else if(option.initStatus=="rebuilding"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'init', type: 'png'});
        status = 'rebuilding';
        statusText = this.myLanguage.REBUILDING;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_INIT;
    }
    else{
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'unknown', type: 'png'});
        status = 'unknown';
        statusText = this.myLanguage.CLOSE;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
        statusColor = this.myConsts.TEXT_COLOR_CLOSE;
    }
    var mainElement = 
            jLego.basicUI.addDiv(target, {id: defID, class: this.myClass.STATUSMONITOR_SINGLEMONITOR_FRAME});
    var iconElement =
            jLego.basicUI.addImg(mainElement, {id: defID + "_image", class: this.myClass.STATUSMONITOR_ARRAYMONITOR_ICON, src: statusImage});
    if(status == 'rebuilding') $(iconElement).addClass(this.myClass.STATUSMONITOR_SPIN);
    else $(iconElement).removeClass(this.myClass.STATUSMONITOR_SPIN);
    var statusTextElement =
            jLego.basicUI.addDiv(mainElement, {id: defID + "_statusText", class: this.myClass.STATUSMONITOR_SINGLEMONITOR_TEXT});
    $(statusTextElement).text(statusText);
    $(statusTextElement).css('color', statusColor);
    if(option.hideText!=true) $(statusTextElement).show();
    else $(statusTextElement).hide();
    //spin for init status
    if(option.initStatus=="init") $(iconElement).addClass(this.myClass.STATUSMONITOR_SPIN);
    else $(iconElement).removeClass(this.myClass.STATUSMONITOR_SPIN);
    
    this.monitorList[defIndex] = mainElement;
    $(this.monitorList[defIndex]).data('status', status);
    $(this.monitorList[defIndex]).data('iconElement', iconElement);
    $(this.monitorList[defIndex]).data('statusTextElement', statusTextElement);
    $(this.monitorList[defIndex]).data('status', status);
    $(this.monitorList[defIndex]).data('hideText', option.hideText);
    return mainElement;
}

jLego.objectUI.statusMonitor.prototype.getSmallIconMonitorStatus=function(monitor){
    return $(monitor).data('status');
}
jLego.objectUI.statusMonitor.prototype.updateSmallIconMonitor=function(monitor, targetStatus, targetStatusText){
    var statusImage, status, statusText, statusClass, statusColor;
    var iconElement = $(monitor).data('iconElement');
    var statusTextElement = $(monitor).data('statusTextElement');
    var hideText = $(monitor).data('hideText');
    if(targetStatus=="ok"){
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'ok', type: 'png'});
        status = 'ok';
        statusText = this.myLanguage.OK;
        if(targetStatusText!=null) statusText = targetStatusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_OK;
        statusColor = this.myConsts.TEXT_COLOR_OK;
    }
    else if(targetStatus=="degraded"){
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'degraded', type: 'png'});
        status = 'degraded';
        statusText = this.myLanguage.DEGRADED;
        if(targetStatusText!=null) statusText = targetStatusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_DEGRADED;
        statusColor = this.myConsts.TEXT_COLOR_DEGRADED;
    }
    else if(targetStatus=="init"){
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'init', type: 'png'});
        status = 'init';
        statusText = this.myLanguage.INIT;
        if(targetStatusText!=null) statusText = targetStatusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_INIT;
        statusColor = this.myConsts.TEXT_COLOR_INIT;
    }
    else if(targetStatus=="close"){
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'close', type: 'png'});
        status = 'close';
        statusText = this.myLanguage.CLOSE;
        if(targetStatusText!=null) statusText = targetStatusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
        statusColor = this.myConsts.TEXT_COLOR_CLOSE;
    }
    else if(option.initStatus=="removed"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'removed', type: 'png'});
        status = 'removed';
        statusText = this.myLanguage.CLOSE;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
    }
    else if(option.initStatus=="read-only"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'readOnly', type: 'png'});
        status = 'read-only';
        statusText = this.myLanguage.READONLY;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
    }
    else if(option.initStatus=="failed"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'failed', type: 'png'});
        status = 'failed';
        statusText = this.myLanguage.FAILED;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_FAILED;
    }
    else if(option.initStatus=="removed"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'removed', type: 'png'});
        status = 'removed';
        statusText = this.myLanguage.CLOSE;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
    }
    else if(option.initStatus=="rebuilding"){
        statusImage = jLego.func.getImgPath({category: 'statusLarge', name: 'init', type: 'png'});
        status = 'rebuilding';
        statusText = this.myLanguage.REBUILDING;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_INIT;
    }
    else if(targetStatus=="unknown"){
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'unknown', type: 'png'});
        status = 'unknown';
        statusText = this.myLanguage.UNKNOWN;
        if(targetStatusText!=null) statusText = targetStatusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
        statusColor = this.myConsts.TEXT_COLOR_CLOSE;
    }
    else return null;
    $(iconElement).attr('src', statusImage);
    if(status == 'rebuilding') $(iconElement).addClass(this.myClass.STATUSMONITOR_SPIN);
    else $(iconElement).removeClass(this.myClass.STATUSMONITOR_SPIN);
    $(statusTextElement).css('color', statusColor);
    $(statusTextElement).text(statusText);
    
    if(targetStatus=="init") $(iconElement).addClass(this.myClass.STATUSMONITOR_SPIN);
    else $(iconElement).removeClass(this.myClass.STATUSMONITOR_SPIN);
    
    $(monitor).data('status', targetStatus);
}

jLego.objectUI.statusMonitor.prototype.addBigRoundMonitor=function(target, option){
    //check option
    if(target==null) return null;
    
    if(option==null) var option={title: null, subtitle: null, currentValue: 0};
    else{
        if(option.title==null)   option.title=null;
        if(option.subtitle==null)   option.subtitle=null;
        if(option.currentValue==null)   option.currentValue=0;
    }
    //draw monitor
    var defIndex = this.monitorList.length;
    var defID = this.myID.STATUSMONITOR_MONITORFRAME + defIndex;
    var statusText, statusClass;
    
    statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_PIE_STATUSTEXT;
    statusText = option.subTitle;
    
    var mainElement = 
            jLego.basicUI.addDiv(target, {id: defID, class: this.myClass.STATUSMONITOR_ICONMONITOR_FRAME});
    var roundBackgroundElement = 
            jLego.basicUI.addDiv(mainElement, {id: defID + "_roundBackground", class: this.myClass.STATUSMONITOR_PIEBACKGROUND_FRAME});
    
    var valueElement =
            jLego.basicUI.addDiv(roundBackgroundElement, {id: defID + "_value", class: this.myClass.STATUSMONITOR_BIGROUND_VALUE_FRAME});
    $(valueElement).text(option.currentValue);
    if(option.currentValue.toString().length > 5)  $(valueElement).css('font-size', "24px");
    else $(valueElement).css('font-size', "32px");
    var unitElement =
            jLego.basicUI.addDiv(roundBackgroundElement, {id: defID + "_value", class: this.myClass.STATUSMONITOR_BIGROUND_UNIT_FRAME});
    $(unitElement).text(option.currentUnit);

    var titleElement =
            jLego.basicUI.addDiv(mainElement, {id: defID + "_title", class: this.myClass.STATUSMONITOR_ICONMONITOR_TITLE});
    $(titleElement).text(option.title);
    var statusTextElement =
            jLego.basicUI.addDiv(mainElement, {id: defID + "_statusText", class: this.myClass.STATUSMONITOR_ICONMONITOR_TITLE});
    $(statusTextElement).text(statusText);
    
    this.monitorList[defIndex] = mainElement;
    $(this.monitorList[defIndex]).data('valueElement', valueElement);
    $(this.monitorList[defIndex]).data('unitElement', unitElement);
    return mainElement;
}

jLego.objectUI.statusMonitor.prototype.updateBigRoundMonitor=function(monitor, option){
    if(option==null)    return null;
    else if(option.currentValue==null || option.currentUnit==null) return null;
    //update
    var valueElement = $(monitor).data('valueElement');
    var unitElement = $(monitor).data('unitElement');
    $(valueElement).text(option.currentValue);
    if(option.currentValue.toString().length > 5)  $(valueElement).css('font-size', "24px");
    else $(valueElement).css('font-size', "32px");
    $(unitElement).text(option.currentUnit);
}

jLego.objectUI.statusMonitor.prototype.addTwoLineIconMonitor=function(target, option){
    //check option
    if(target==null) return null;
    
    if(option==null) var option={initStatus: 'unknown'};
    else{
        if(option.initStatus==null)   option.initStatus='unknown';
    }
    //draw monitor
    var defIndex = this.monitorList.length;
    var defID = this.myID.STATUSMONITOR_MONITORFRAME + defIndex;
    var statusImage, status, statusText, statusColor, statusClass;
    if(option.initStatus=="ok"){
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'ok', type: 'png'});
        status = 'ok';
        statusText = this.myLanguage.OK;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_OK;
        statusColor = this.myConsts.TEXT_COLOR_OK;
    }
    else if(option.initStatus=="degraded"){
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'degraded', type: 'png'});
        status = 'degraded';
        statusText = this.myLanguage.DEGRADED;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_DEGRADED;
        statusColor = this.myConsts.TEXT_COLOR_DEGRADED;
    }
    else if(option.initStatus=="init"){
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'init', type: 'png'});
        status = 'init';
        statusText = this.myLanguage.INIT;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_INIT;
        statusColor = this.myConsts.TEXT_COLOR_INIT;
    }
    else if(option.initStatus=="rebuilding"){
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'init', type: 'png'});
        status = 'rebuilding';
        statusText = this.myLanguage.REBUILDING;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_INIT;
        statusColor = this.myConsts.TEXT_COLOR_INIT;
    }
    else if(option.initStatus=="read-only"){
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'readOnly', type: 'png'});
        status = 'read-only';
        statusText = this.myLanguage.READONLY;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
        statusColor = this.myConsts.TEXT_COLOR_CLOSE;
    }
    else if(option.initStatus=="failed"){
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'failed', type: 'png'});
        status = 'failed';
        statusText = this.myLanguage.FAILED;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_FAILED;
        statusColor = this.myConsts.TEXT_COLOR_FAILED;
    }
    else if(option.initStatus=="removed"){
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'removed', type: 'png'});
        status = 'removed';
        statusText = this.myLanguage.REMOVED;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
        statusColor = this.myConsts.TEXT_COLOR_CLOSE;
    }
    else if(option.initStatus=="close"){
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'close', type: 'png'});
        status = 'close';
        statusText = this.myLanguage.CLOSE;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
        statusColor = this.myConsts.TEXT_COLOR_CLOSE;
    }
    else{
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'unknown', type: 'png'});
        status = 'unknown';
        statusText = this.myLanguage.CLOSE;
        if(option.statusText!=null) statusText = option.statusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
        statusColor = this.myConsts.TEXT_COLOR_CLOSE;
    }
    var mainElement = 
            jLego.basicUI.addDiv(target, {id: defID, class: this.myClass.STATUSMONITOR_TWOLINE_MONITOR_FRAME});
    $(mainElement).height($(target).height());
    var iconElement =
            jLego.basicUI.addImg(mainElement, {id: defID + "_image", class: this.myClass.STATUSMONITOR_ARRAYMONITOR_ICON, src: statusImage});
    $(iconElement).css("position", "absolute");
    $(iconElement).width($(target).height() - 16);
    $(iconElement).height($(target).height() - 16);
    $(iconElement).css("left", "8px");
    $(iconElement).css("top", "0px");
    var statusTextElement =
            jLego.basicUI.addDiv(mainElement, {id: defID + "_statusText", class: this.myClass.STATUSMONITOR_TWOLINE_MONITOR_TEXT});
    $(statusTextElement).text(statusText);
    $(statusTextElement).css('color', statusColor);
    $(statusTextElement).height($(target).height() / 2);
    $(statusTextElement).css("line-height", parseInt($(target).height() / 2) + "px");
    $(statusTextElement).css("position", "absolute");
    $(statusTextElement).css("top", "4px");
    $(statusTextElement).css("left", ($(iconElement).width() + 20) + "px");
    var secondLineTextElement =
            jLego.basicUI.addDiv(mainElement, {id: defID + "_secondLineText", class: this.myClass.STATUSMONITOR_TWOLINE_MONITOR_TEXT});
    $(secondLineTextElement).text(option.diskSpace);
    $(secondLineTextElement).height($(target).height() / 2);
    $(secondLineTextElement).css("font-size", "14px");
    $(secondLineTextElement).css("line-height", parseInt($(target).height() / 2) + "px");
    $(secondLineTextElement).css("position", "absolute");
    $(secondLineTextElement).css("top", parseInt($(target).height() / 2 -4) + "px");
    $(secondLineTextElement).css("left", ($(iconElement).width() + 20) + "px");
    //spin for init status
    if(option.initStatus=="init") $(iconElement).addClass(this.myClass.STATUSMONITOR_SPIN);
    else $(iconElement).removeClass(this.myClass.STATUSMONITOR_SPIN);
    
    this.monitorList[defIndex] = mainElement;
    $(this.monitorList[defIndex]).data('status', status);
    $(this.monitorList[defIndex]).data('iconElement', iconElement);
    $(this.monitorList[defIndex]).data('statusTextElement', statusTextElement);
    $(this.monitorList[defIndex]).data('status', status);
    $(this.monitorList[defIndex]).data('hideText', option.hideText);
    return mainElement;
}

jLego.objectUI.statusMonitor.prototype.getTwoLineIconMonitorStatus=function(monitor){
    return $(monitor).data('status');
}
jLego.objectUI.statusMonitor.prototype.updateTwoLineIconMonitor=function(monitor, targetStatus, targetStatusText){
    var statusImage, status, statusText, statusClass, statusColor;
    var iconElement = $(monitor).data('iconElement');
    var statusTextElement = $(monitor).data('statusTextElement');
    var hideText = $(monitor).data('hideText');
    if(targetStatus=="ok"){
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'ok', type: 'png'});
        status = 'ok';
        statusText = this.myLanguage.OK;
        if(targetStatusText!=null) statusText = targetStatusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_OK;
        statusColor = this.myConsts.TEXT_COLOR_OK;
    }
    else if(targetStatus=="degraded"){
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'degraded', type: 'png'});
        status = 'degraded';
        statusText = this.myLanguage.DEGRADED;
        if(targetStatusText!=null) statusText = targetStatusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_DEGRADED;
        statusColor = this.myConsts.TEXT_COLOR_DEGRADED;
    }
    else if(targetStatus=="init"){
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'init', type: 'png'});
        status = 'init';
        statusText = this.myLanguage.INIT;
        if(targetStatusText!=null) statusText = targetStatusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_INIT;
        statusColor = this.myConsts.TEXT_COLOR_INIT;
    }
    else if(targetStatus=="read-only"){
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'readOnly', type: 'png'});
        status = 'read-only';
        statusText = this.myLanguage.READONLY;
        if(targetStatusText!=null) statusText = targetStatusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
        statusColor = this.myConsts.TEXT_COLOR_CLOSE;
    }
    else if(targetStatus=="removed"){
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'removed', type: 'png'});
        status = 'removed';
        statusText = this.myLanguage.REMOVED;
        if(targetStatusText!=null) statusText = targetStatusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
        statusColor = this.myConsts.TEXT_COLOR_CLOSE;
    }
    else if(targetStatus=="rebuilding"){
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'rebuilding', type: 'png'});
        status = 'rebuilding';
        statusText = this.myLanguage.REBUILDING;
        if(targetStatusText!=null) statusText = targetStatusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
        statusColor = this.myConsts.TEXT_COLOR_CLOSE;
    }
    else if(targetStatus=="failed"){
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'failed', type: 'png'});
        status = 'failed';
        statusText = this.myLanguage.FAILED;
        if(targetStatusText!=null) statusText = targetStatusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_FAILED;
        statusColor = this.myConsts.TEXT_COLOR_FAILED;
    }
    else if(targetStatus=="close"){
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'close', type: 'png'});
        status = 'close';
        statusText = this.myLanguage.CLOSE;
        if(targetStatusText!=null) statusText = targetStatusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
        statusColor = this.myConsts.TEXT_COLOR_CLOSE;
    }
    else if(targetStatus=="unknown"){
        statusImage = jLego.func.getImgPath({category: 'statusSmall', name: 'unknown', type: 'png'});
        status = 'unknown';
        statusText = this.myLanguage.UNKNOWN;
        if(targetStatusText!=null) statusText = targetStatusText;
        statusClass = this.myClass.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE;
        statusColor = this.myConsts.TEXT_COLOR_CLOSE;
    }
    else return null;
    $(iconElement).attr('src', statusImage);
    $(statusTextElement).css('color', statusColor);
    $(statusTextElement).text(statusText);
    
    if(targetStatus=="init") $(iconElement).addClass(this.myClass.STATUSMONITOR_SPIN);
    else $(iconElement).removeClass(this.myClass.STATUSMONITOR_SPIN);
    
    $(monitor).data('status', targetStatus);
}


jLego.objectUI.statusMonitor.prototype.resize=function(){

}

jLego.objectUI.statusMonitor.prototype.resizeHandler=function(){
    this.resize();
}