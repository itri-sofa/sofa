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

jLego.objectUI.stepPage=function(){
    var myID;
    var myClass;
    var myConsts;
    var randomID;
    var parentElement;
    var mainElement;
    var tabFrame;
    var pageFrame;
    var tabList;
    var tabPageList;
    var selectTabIndex; 
    this.initialize();
}

jLego.objectUI.stepPage.prototype.initialize=function(){
    this.myID=jLego.objectUI.id.stepPage;
    this.myClass=jLego.objectUI.cls.stepPage;
    this.myConsts=jLego.objectUI.constants.stepPage;
    this.randomID = "_" + jLego.func.getRandomString() + "_";
    
    this.tabList=[];
    this.tabPageList=[];
    this.selectTabIndex=-1;
}

jLego.objectUI.stepPage.prototype.add=function(target){
    //check option
    if(target==null) return null;

    this.parentElement = target;
    //create
    this.mainElement = 
            jLego.basicUI.addDiv(target, {id: this.myID.STEPPAGE_MAIN_FRAME + this.randomID, class: this.myClass.STEPPAGE_MAIN_FRAME});
    this.tabFrame = 
            jLego.basicUI.addDiv(this.mainElement, {id: this.myID.STEPPAGE_TAB_FRAME + this.randomID, class: this.myClass.STEPPAGE_TAB_FRAME});
    this.pageFrame = 
            jLego.basicUI.addDiv(this.mainElement, {id: this.myID.STEPPAGE_PAGE_FRAME + this.randomID, class: this.myClass.STEPPAGE_PAGE_FRAME});

    this.resize();
    
    return this.mainElement;
}

jLego.objectUI.stepPage.prototype.addStepElement=function(name, option, callbackObject){
    if(option==0)   var option = {type: 'middle'};
    
    var tabIndex = this.tabList.length;
    var newTabID = this.myID.STEPPAGE_TAB + this.randomID + tabIndex;
    var newTabPageID = this.myID.STEPPAGE_TAB + this.randomID + tabIndex;
    //add tab
    if(option.type=='start'){
        this.tabList[tabIndex] = 
            jLego.basicUI.addDiv(this.tabFrame, {id: newTabID, class: this.myClass.STEPPAGE_TAB_OFF_START});
    }
    else if (option.type=='end'){
        this.tabList[tabIndex] = 
            jLego.basicUI.addDiv(this.tabFrame, {id: newTabID, class: this.myClass.STEPPAGE_TAB_OFF_END});
            $(this.tabList[tabIndex]).css('margin-left', '-7px');
    }
    else{
        this.tabList[tabIndex] = 
            jLego.basicUI.addDiv(this.tabFrame, {id: newTabID, class: this.myClass.STEPPAGE_TAB_OFF});
        $(this.tabList[tabIndex]).css('margin-left', '-7px');
    }
    
    $(this.tabList[tabIndex]).text(name);
    
    //adjust z-index
    for(var i=tabIndex; i>=0; i--){
        $(this.tabList[i]).css('z-index', (1 + (tabIndex-i)));
    }
    
    //add page
    this.tabPageList[tabIndex] = 
            jLego.basicUI.addDiv(this.pageFrame, {id: newTabPageID, class: this.myClass.STEPPAGE_PAGE});
    $(this.tabPageList[tabIndex]).hide();
    
    $(this.tabList[tabIndex]).data('relatedPage', this.tabPageList[tabIndex]);
    $(this.tabList[tabIndex]).data('tabIndex', tabIndex);
    $(this.tabList[tabIndex]).data('parent', this);
    $(this.tabList[tabIndex]).data('name', name);
    if(callbackObject!=null){
        $(this.tabList[tabIndex]).data('onClickArg', callbackObject.arg);
        $(this.tabList[tabIndex]).data('onClickCallback', callbackObject.callback);
    }
    //set tab on click
    $(this.tabList[tabIndex]).click(function(){
        var parent = $(this).data('parent');
        var tabIndex = $(this).data('tabIndex');
        var relatedPage = $(this).data('relatedPage');
        if($(this).data('clickable')=='enable'){
            //hide previous tab
            if(parent.selectTabIndex!=-1){
                parent.enableTab(parent.selectTabIndex);
                $(parent.tabPageList[parent.selectTabIndex]).hide();
            }
            //show clicked tab
            parent.selectTabIndex = tabIndex;
            parent.clickedTab(parent.selectTabIndex);
            $(relatedPage).show();
            
            
            var callback = $(this).data('onClickCallback');
            var arg = $(this).data('onClickArg');
            
            if(callback!=null){
                callback(arg);
            }
        }
    });
}
jLego.objectUI.stepPage.prototype.setStepTabCallback = function(tabIndex, callbackObject){
    if(tabIndex >= this.tabList.length || tabIndex<0)    return null;
    if(callbackObject!=null){
        $(this.tabList[tabIndex]).data('onClickArg', callbackObject.arg);
        $(this.tabList[tabIndex]).data('onClickCallback', callbackObject.callback);
    }
    $(this.tabList[tabIndex]).off('click');
    $(this.tabList[tabIndex]).click(function(){
        var parent = $(this).data('parent');
        var tabIndex = $(this).data('tabIndex');
        var relatedPage = $(this).data('relatedPage');
        if($(this).data('clickable')=='enable'){
            //hide previous tab
            if(parent.selectTabIndex!=-1){
                parent.enableTab(parent.selectTabIndex);
                $(parent.tabPageList[parent.selectTabIndex]).hide();
            }
            //show clicked tab
            parent.selectTabIndex = tabIndex;
            parent.clickedTab(parent.selectTabIndex);
            $(relatedPage).show();
            
            
            var callback = $(this).data('onClickCallback');
            var arg = $(this).data('onClickArg');
            
            if(callback!=null){
                callback(arg);
            }
        }
    });
}

jLego.objectUI.stepPage.prototype.disableTab=function(index, hint){
    if(index >= this.tabList.length)    return null;
    else{
        $(this.tabList[index]).removeClass(this.myClass.STEPPAGE_TAB_CLICKED);
        $(this.tabList[index]).removeClass(this.myClass.STEPPAGE_TAB_ENABLE);
        $(this.tabList[index]).addClass(this.myClass.STEPPAGE_TAB_DISABLE);
        $(this.tabList[index]).data('clickable', 'disable');
        $(this.tabList[index]).text($(this.tabList[index]).data('name'));
    }
    if(hint!=null){
        $(this.tabList[index]).attr('title', hint);
        $(this.tabList[index]).tooltip({tooltipClass: this.myClass.STEPPAGE_TOOLTIP_WARN, disabled: false});
    }
    else{
        $(this.tabList[index]).tooltip({disabled: true});
    }
}
jLego.objectUI.stepPage.prototype.enableTab=function(index){
    if(index >= this.tabList.length)    return null;
    else{
        $(this.tabList[index]).removeClass(this.myClass.STEPPAGE_TAB_CLICKED);
        $(this.tabList[index]).removeClass(this.myClass.STEPPAGE_TAB_DISABLE);
        $(this.tabList[index]).addClass(this.myClass.STEPPAGE_TAB_ENABLE);
        $(this.tabList[index]).data('clickable', 'enable');
        $(this.tabList[index]).text($(this.tabList[index]).data('name'));
    }
    $(this.tabList[index]).tooltip({disabled: true});
    $(this.tabList[index]).tooltip( "option", "tooltipClass", "stepPage_Tooltip_Warn" );
    
}

jLego.objectUI.stepPage.prototype.clickedTab=function(index){
    if(index >= this.tabList.length)    return null;
    else{
        $(this.tabList[index]).removeClass(this.myClass.STEPPAGE_TAB_ENABLE);
        $(this.tabList[index]).removeClass(this.myClass.STEPPAGE_TAB_DISABLE);
        $(this.tabList[index]).addClass(this.myClass.STEPPAGE_TAB_CLICKED);
        $(this.tabList[index]).data('clickable', 'clicked');
        $(this.tabList[index]).text("* " + $(this.tabList[index]).data('name'));
    }
    $(this.tabList[index]).tooltip({disabled: true});;
}

jLego.objectUI.stepPage.prototype.getTabPage=function(index){
    if(index >= this.tabPageList.length)    return null;
    else return this.tabPageList[index];
}

jLego.objectUI.stepPage.prototype.getTab=function(index){
    if(index >= this.tabList.length)    return null;
    else return this.tabList[index];
}

jLego.objectUI.stepPage.prototype.resize=function(){
    $(this.tabFrame).width($(this.parentElement).width()-5);
    $(this.tabFrame).css('margin-left', '5px');
    $(this.pageFrame).width($(this.parentElement).width()-5);
    $(this.pageFrame).css('margin-left', '5px');
    $(this.pageFrame).height($(this.parentElement).height() - $(this.tabFrame).height() - parseInt($(this.tabFrame).css('margin-top')));
}