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

jLego.objectUI.popoutPanel=function(){
    var myID;
    var myClass;
    var myConsts;
    
    var showing;
    
    var footFrameMode;
    var mainElement;
    var foregroundElement;
    var titleFrame;
    var contentFrame;
    var footFrame;
    this.initialize();
}

jLego.objectUI.popoutPanel.prototype.initialize=function(){
    this.myID=jLego.objectUI.id.popoutPanel;
    this.myClass=jLego.objectUI.cls.popoutPanel;
    this.myConsts=jLego.objectUI.constants.popoutPanel;
    this.myLanguage = jLego.objectUI.lang.popoutPanel;
    
    this.showing = false;
}

jLego.objectUI.popoutPanel.prototype.add=function(target, option){
    if(option==null)    var option={hasFootFrame: false, title: ''};
    else{
        if(option.hasFootFrame==null)   option.hasFootFrame = false;
        if(option.title==null)   option.title = '';
    }
    this.footFrameMode = option.hasFootFrame;
    this.close();
    this.mainElement = 
            jLego.basicUI.addDiv(target, {id: jLego.func.getRandomString(), class: this.myClass.POPOUTPANEL_BACKGROUND});

    this.foregroundElement = 
            jLego.basicUI.addDiv(this.mainElement, {id: jLego.func.getRandomString(), class: this.myClass.POPOUTPANEL_FOREGROUND});
    this.setupWidthAndHeight(this.foregroundElement, option);
    if(jLego.func.browserIs('chrome')){
        $(this.foregroundElement).css('display', 'block');
    }

    this.titleFrame =
            jLego.basicUI.addDiv(this.foregroundElement, {id: jLego.func.getRandomString(), class: this.myClass.POPOUTPANEL_TITLEFRAME});
    $(this.titleFrame).text(option.title);
    
    var closeButton = 
            jLego.basicUI.addImg(this.titleFrame, {id: jLego.func.getRandomString(), class: this.myClass.POPOUTPANEL_CLOSE, src: jLego.func.getImgPath({category: 'popoutPanel', name: 'close', type: 'png'})});
    $(closeButton).data('parent', this);
    $(closeButton).click(function(){
        var parent = $(this).data('parent');
        parent.showing = false;
        parent.close();
    });
    this.contentFrame =
            jLego.basicUI.addDiv(this.foregroundElement, {id: jLego.func.getRandomString(), class: this.myClass.POPOUTPANEL_CONTENTFRAME});
    if(this.footFrameMode==true){
        this.footFrame =
            jLego.basicUI.addDiv(this.foregroundElement, {id: jLego.func.getRandomString(), class: this.myClass.POPOUTPANEL_FOOTFRAME});
    }
    
    this.showing = true;
    
    this.resize();
    
    $(this.mainElement).data('parent', this);
    $(this.mainElement).data('foregroundElement', this.foregroundElement);
    $(this.mainElement).data('backgroundElement', this.mainElement);
}

jLego.objectUI.popoutPanel.prototype.setupWidthAndHeight=function(target, option){
    //width & height
    if(option.widthInPercent){
        $(target).css('width', option.widthInPercent + "%");
    }
    if(option.heightInPercent){
        $(target).css('height', option.heightInPercent + "%");
    }
    if(option.widthInPixel){
        $(target).css('width', option.widthInPixel + "px");
    }
    if(option.heightInPixel){
        $(target).css('height', option.heightInPixel + "px");
    }
    //max-width & max-height
    if(option.maxWidthInPercent){
        $(target).css('max-width', option.maxWidthInPercent + "%");
    }
    if(option.maxHeightInPercent){
        $(target).css('max-height', option.maxHeightInPercent + "%");
    }
    if(option.maxWidthInPixel){
        $(target).css('max-width', option.maxWidthInPixel + "px");
    }
    if(option.maxHeightInPixel){
        $(target).css('max-height', option.maxHeightInPixel + "px");
    }
    //min-width & min-height
    if(option.minWidthInPercent){
        $(target).css('min-width', option.minWidthInPercent + "%");
    }
    if(option.minHeightInPercent){
        $(target).css('min-height', option.minHeightInPercent + "%");
    }
    if(option.minWidthInPixel){
        $(target).css('min-width', option.minWidthInPixel + "px");
    }
    if(option.minHeightInPixel){
        $(target).css('min-height', option.minHeightInPixel + "px");
    }
}

jLego.objectUI.popoutPanel.prototype.getContentFrame=function(){
    return this.contentFrame;
}
jLego.objectUI.popoutPanel.prototype.getFootFrame=function(){
    return this.footFrame;
}

jLego.objectUI.popoutPanel.prototype.isShow=function(){
    return this.isShown;
}

jLego.objectUI.popoutPanel.prototype.close=function(){
    var callback = $(this.mainElement).data('onClickCallback');
    var arg = $(this.mainElement).data('onClickArg');
    if(callback!=null){
        callback(arg);
    }
    $(this.mainElement).remove();
}
jLego.objectUI.popoutPanel.prototype.setCloseCallback=function(callbackObject){
    if(callbackObject!=null){
        $(this.mainElement).data('onClickArg', callbackObject.arg);
        $(this.mainElement).data('onClickCallback', callbackObject.callback);
    }
}

jLego.objectUI.popoutPanel.prototype.setClickOutsideToClose=function(){
    $(this.mainElement).click(function(){
        var parent = $(this).data('parent');
        var foregroundElement = $(this).data('foregroundElement');
        var backgroundElement = $(this).data('backgroundElement');
        if(foregroundElement!=null){
            if(!$(foregroundElement).is(':hover') && !$(backgroundElement).is(':hover')){
                
            }
            else if($(foregroundElement).is(':hover')){
                
            }
            else{
                parent.close();
            }
        }
    });
}
jLego.objectUI.popoutPanel.prototype.disableClickOutsideToClose=function(){
    $(this.mainElement).off('click');
}

jLego.objectUI.popoutPanel.prototype.resize=function(){
    var newHeight;
    if(this.footFrameMode==true){
        newHeight = $(this.foregroundElement).height() - $(this.titleFrame).height() - $(this.footFrame).height();        
    }
    else{
        newHeight = $(this.foregroundElement).height() - $(this.titleFrame).height(); 
    }
    $(this.contentFrame).height(newHeight);
}