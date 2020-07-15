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

jLego.objectUI.tabPageOrg=function(){
    var myID;
    var myClass;
    var myConsts;
    var randomID;
    var mode;
    var parentElement;
    var mainElement;
    var tabFrame;
    var pageFrame;
    var tabList;
    var tabPageList;
    var selectTabIndex; 
    
    var permanentPage;
    
    this.initialize();
}

jLego.objectUI.tabPageOrg.prototype.initialize=function(){
    this.myID=jLego.objectUI.id.tabPageOrg;
    this.myClass=jLego.objectUI.cls.tabPageOrg;
    this.myConsts=jLego.objectUI.constants.tabPageOrg;
    this.randomID = "_" + jLego.func.getRandomString() + "_";
    this.tabList=[];
    this.tabPageList=[];
    this.selectTabIndex=-1;
}

jLego.objectUI.tabPageOrg.prototype.add=function(target, option){
    //check option
    if(target==null) return null;
    if(option==null) var option={mode: 'page'};
    else{
        if(option.mode==null)   option.mode = 'page';
        else if(option.mode!='no-border')   option.mode = 'page';
    }
    this.parentElement = target;
    //create
    this.mainElement = 
            jLego.basicUI.addDiv(target, {id: this.myID.TABPAGE_MAIN_FRAME + this.randomID, class: this.myClass.TABPAGE_MAIN_FRAME});
    this.tabFrame = 
            jLego.basicUI.addDiv(this.mainElement, {id: this.myID.TABPAGE_TAB_FRAME + this.randomID, class: this.myClass.TABPAGE_TAB_FRAME});
    if(option.mode=='no-border'){
        this.mode = 'no-border';
        this.pageFrame = 
            jLego.basicUI.addDiv(this.mainElement, {id: this.myID.TABPAGE_PAGE_FRAME + this.randomID, class: this.myClass.TABPAGE_PAGE_FRAME_NOBORDER});
    }
    else{
        this.mode = 'page';
        this.pageFrame = 
            jLego.basicUI.addDiv(this.mainElement, {id: this.myID.TABPAGE_PAGE_FRAME + this.randomID, class: this.myClass.TABPAGE_PAGE_FRAME});
    }
    
    if(option.permanentPage==true){
        this.permanentPage = 
            jLego.basicUI.addDiv(this.pageFrame, {id: jLego.func.getRandomString(), class: this.myClass.TABPAGE_PAGE});
    }
    
    this.resize();
    
    return this.mainElement;
}

jLego.objectUI.tabPageOrg.prototype.addTabElement=function(name, callbackObject){
    var tabIndex = this.tabList.length;
    var newTabID = this.myID.TABPAGE_TAB + this.randomID + tabIndex;
    var newTabPageID = this.myID.TABPAGE_PAGE + this.randomID + tabIndex;
    this.tabList[tabIndex] = 
            jLego.basicUI.addDiv(this.tabFrame, {id: newTabID, class: this.myClass.TABPAGE_TAB_OFF});
    $(this.tabList[tabIndex]).text(name);
    this.tabPageList[tabIndex] = 
            jLego.basicUI.addDiv(this.pageFrame, {id: newTabPageID, class: this.myClass.TABPAGE_PAGE});
    $(this.tabPageList[tabIndex]).hide();
    
    if(tabIndex==0){
        if(this.mode=='no-border') $(this.tabList[tabIndex]).css('left', '30px');
        else $(this.tabList[tabIndex]).css('left', '0px');
    }
    else{
        var marginLeft = $(this.tabList[tabIndex-1]).width() + parseInt($(this.tabList[tabIndex-1]).css('left'))  + parseInt($(this.tabList[tabIndex-1]).css('padding-left'))  + parseInt($(this.tabList[tabIndex-1]).css('padding-right'));
        $(this.tabList[tabIndex]).css('left', marginLeft + 'px');
    }
    $(this.tabList[tabIndex]).data('relatedPage', this.tabPageList[tabIndex]);
    $(this.tabList[tabIndex]).data('tabIndex', tabIndex);
    $(this.tabList[tabIndex]).data('parent', this);
    if(callbackObject!=null){
        $(this.tabList[tabIndex]).data('onClickArg', callbackObject.arg);
        $(this.tabList[tabIndex]).data('onClickCallback', callbackObject.callback);
    }
    
    //set tab on click
    $(this.tabList[tabIndex]).click(function(){
        var parent = $(this).data('parent');
        var tabIndex = $(this).data('tabIndex');
        var relatedPage = $(this).data('relatedPage');
        if($(this).attr('class').toString().indexOf(parent.myClass.TABPAGE_TAB_OFF)>-1){
            //hide previous tab
            if(parent.selectTabIndex!=-1){
                $(parent.tabList[parent.selectTabIndex]).attr('class', parent.myClass.TABPAGE_TAB_OFF);
                $(parent.tabPageList[parent.selectTabIndex]).hide();
            }
            //show clicked tab
            parent.selectTabIndex = tabIndex;
            $(this).attr('class', parent.myClass.TABPAGE_TAB_ON);
            if(parent.mode=='page'){
                if(tabIndex==0) $(parent.pageFrame).attr('class', parent.myClass.TABPAGE_PAGE_FRAME_FIRSTTAB);
                else $(parent.pageFrame).attr('class', parent.myClass.TABPAGE_PAGE_FRAME);
            }

            if(parent.permanentPage==null) $(relatedPage).show();
            var callback = $(this).data('onClickCallback');
            var arg = $(this).data('onClickArg');
            
            if(callback!=null){
                callback(arg);
            }
        }
    });
}

jLego.objectUI.tabPageOrg.prototype.getTab=function(index){
    if(index >= this.tabList.length)    return null;
    else return this.tabList[index];
}

jLego.objectUI.tabPageOrg.prototype.getTabPage=function(index){
    if(index >= this.tabPageList.length)    return null;
    else return this.tabPageList[index];
}

jLego.objectUI.tabPageOrg.prototype.getPermanentPage=function(){
    return this.permanentPage;
}
jLego.objectUI.tabPageOrg.prototype.getSelectedTab=function(){
    return this.selectTabIndex;
}

jLego.objectUI.tabPageOrg.prototype.resize=function(){
    $(this.tabFrame).width($(this.parentElement).width()-2);
    $(this.pageFrame).width($(this.parentElement).width()-2);
    $(this.pageFrame).height($(this.parentElement).height() - $(this.tabFrame).height() -2);
}

jLego.objectUI.tabPageOrg.prototype.resizeHandler=function(){
    this.resize();
}