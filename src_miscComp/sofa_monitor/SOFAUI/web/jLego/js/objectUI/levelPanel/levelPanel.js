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

jLego.objectUI.levelPanel=function(){
    var myID;
    var myClass;
    var myConsts;
    var tableBaseID;
    
    var parentElement;
    var mainElement;
    
    var pageList;
    var pageElement;
    var infoElement;
    var infoPage;
    
    var currentPageIndex;
    var minWidthList;
    
    var minPreserveWidth; 
    
    this.initialize();
}

jLego.objectUI.levelPanel.prototype.initialize=function(){
    this.myID=jLego.objectUI.id.levelPanel;
    this.myClass=jLego.objectUI.cls.levelPanel;
    this.myConsts=jLego.objectUI.constants.levelPanel;
    this.pageList = [];
    this.minWidthList=[];
    this.pageElement=null;
    this.infoElement=null;
    
    this.minPreserveWidth = 55;
}

jLego.objectUI.levelPanel.prototype.add=function(target, option){
    //check option
    if(target==null) return null;
    
    if(option==null){
        return null;
    }
    else{
        if(option.pageTitleList==null)   return null;
    }
    if(option.minWidthList==null){
        for(var i=0; i<option.pageTitleList.length; i++) this.minWidthList[i] = 300;
    }
    else this.minWidthList = option.minWidthList;
    //draw
    this.parentElement = target;
    this.mainElement =
            jLego.basicUI.addDiv(target, {id: jLego.func.getRandomString(), class: this.myClass.OUTTER_FRAME});
    this.infoElement = 
            jLego.basicUI.addDiv(this.mainElement, {id: jLego.func.getRandomString(), class: this.myClass.OUTTER_INFO_FRAME});
    var titleElement =
            jLego.basicUI.addDiv(this.infoElement, {id: jLego.func.getRandomString(), class: this.myClass.PAGE_TITLE});  
    $(titleElement).text(option.infoTitle);
    this.infoPage =
            jLego.basicUI.addDiv(this.infoElement, {id: jLego.func.getRandomString(), class: this.myClass.PAGE});  
    $(this.infoPage).width("100%");
    $(this.infoPage).height($(this.infoElement).height() - $(titleElement).height());
    $(this.infoPage).css('top', $(titleElement).height()+'px');
    $(this.infoPage).css('overflow-y', 'auto');
    
    this.pageElement = 
            jLego.basicUI.addDiv(this.mainElement, {id: jLego.func.getRandomString(), class: this.myClass.OUTTER_PAGE_FRAME});
    
    $(this.pageElement).width($(this.mainElement).width() - $(this.infoElement).width() - 2);
    
    for(var i=0; i<option.pageTitleList.length; i++){
        this.pageList[i] = 
           jLego.basicUI.addDiv(this.pageElement, {id: jLego.func.getRandomString(), class: this.myClass.PAGE});   
        if(i==0){
            var reserveWidth = (option.pageTitleList.length - 1) * this.minPreserveWidth;
            $(this.pageList[i]).width($(this.pageElement).width() - reserveWidth);
            $(this.pageList[i]).css('right', reserveWidth+'px');
        }
        else{
            $(this.pageList[i]).width(this.minPreserveWidth);
            var reserveWidth = (option.pageTitleList.length - 1 - i) * this.minPreserveWidth;
            $(this.pageList[i]).css('right', reserveWidth+'px');
            $(this.pageList[i]).addClass(this.myClass.PAGE_SHADOW);   
        }
        var titleElement =
            jLego.basicUI.addDiv(this.pageList[i], {id: jLego.func.getRandomString(), class: this.myClass.PAGE_TITLE});   
        $(titleElement).text(option.pageTitleList[i]);
        var contentElement = 
            jLego.basicUI.addDiv(this.pageList[i], {id: jLego.func.getRandomString(), class: this.myClass.PAGE_ELEMENT}); 
        $(contentElement).height($(this.pageList[i]).height() - $(titleElement).height());
        $(this.pageList[i]).data('titleElement', titleElement);
        $(this.pageList[i]).data('contentElement', contentElement);
    }
    this.currentPageIndex=0;
    
    
    /*$(this.infoElement).data('parent', this);
    $(this.infoElement).data('currentPage', 0);
    $(this.infoElement).click(function(){
       var parent = $(this).data('parent'); 
       var currentPage = $(this).data('currentPage'); 
       var newPage = (currentPage+1) % 3;
       $(this).data('currentPage', newPage);
       parent.showLevel(newPage);
    });*/
}
jLego.objectUI.levelPanel.prototype.getContentPageOfLevel=function(level){
    return $(this.pageList[level]).data('contentElement');
}

jLego.objectUI.levelPanel.prototype.getInfoPage=function(){
    return this.infoPage;
}

jLego.objectUI.levelPanel.prototype.showLevel=function(level, callback){
    if(level<0 || level >= this.pageList.length) return null;
    //calculate shown size
    var totalWidth = $(this.pageElement).width();
    var availableWidth = totalWidth;
    for(var i=this.pageList.length-1; i>=level+1; i--){
        availableWidth-=this.minPreserveWidth;
    }
    for(var i=level-1; i>=0; i--){
        availableWidth-=this.minWidthList[i];
    }
    //hide following level
    for(var i=this.pageList.length-1; i>=level+1; i--){
        var preserveRight = (this.pageList.length - 1 - i) * this.minPreserveWidth;
        $(this.pageList[i]).animate({width: this.minPreserveWidth, right: preserveRight}, 600);
    }
    //hide previous level
    var marginRight = $(this.pageElement).width();
    for(var i=0; i<level; i++){
        marginRight -= this.minWidthList[i];
        $(this.pageList[i]).animate({width: this.minWidthList[i], right: marginRight}, 600);
    }
    //show current level
    var targetWidth = availableWidth;
    if(availableWidth<this.minWidthList[level]) targetWidth = this.minWidthList[level];
    var preserveRight = (this.pageList.length - 1 - level) * this.minPreserveWidth;
    $(this.pageList[level]).animate({width: targetWidth, right: preserveRight}, 600, null, callback);
    
    this.currentPageIndex = level;
}
jLego.objectUI.levelPanel.prototype.getCurrentLevel=function(){
    return this.currentPageIndex;
}
jLego.objectUI.levelPanel.prototype.resize=function(){
    $(this.pageElement).width($(this.mainElement).width() - $(this.infoElement).width() - 2);
    var newHeight = $(this.mainElement).height() - 2 * parseInt($(this.pageElement).css('margin-top'));
    $(this.pageElement).height(newHeight);
    var level = this.currentPageIndex;
    var totalWidth = $(this.pageElement).width();
    var availableWidth = totalWidth;
    for(var i=this.pageList.length-1; i>=level+1; i--){
        availableWidth-=this.minPreserveWidth;
    }
    for(var i=level-1; i>=0; i--){
        availableWidth-=this.minWidthList[i];
    }
    //hide following level
    for(var i=this.pageList.length-1; i>=level+1; i--){
        var preserveRight = (this.pageList.length - 1 - i) * this.minPreserveWidth;
        $(this.pageList[i]).width(this.minPreserveWidth);
        $(this.pageList[i]).css('right', preserveRight+'px');
        $(this.pageList[i]).height(newHeight);
        var titleElement =
            $(this.pageList[i]).data('titleElement'); 
        var contentElement = 
            $(this.pageList[i]).data('contentElement');
        $(contentElement).height($(this.pageList[i]).height() - $(titleElement).height());

    }
    //hide previous level
    var marginRight = $(this.pageElement).width();
    for(var i=0; i<level; i++){
        marginRight -= this.minWidthList[i];
        $(this.pageList[i]).width(this.minWidthList[i]);
        $(this.pageList[i]).css('right', marginRight+'px');
        $(this.pageList[i]).height(newHeight);
        var titleElement =
            $(this.pageList[i]).data('titleElement'); 
        var contentElement = 
            $(this.pageList[i]).data('contentElement');
        $(contentElement).height($(this.pageList[i]).height() - $(titleElement).height());

    }
    var targetWidth = availableWidth;
    if(availableWidth<this.minWidthList[level]) targetWidth = this.minWidthList[level];
    var preserveRight = (this.pageList.length - 1 - level) * this.minPreserveWidth;
    $(this.pageList[level]).width(targetWidth);
    $(this.pageList[level]).css('right', preserveRight+'px');
    $(this.pageList[level]).height(newHeight);
    $(this.infoElement).height(newHeight);
    var titleElement =
        $(this.pageList[level]).data('titleElement'); 
    var contentElement = 
        $(this.pageList[level]).data('contentElement');
    $(contentElement).height($(this.pageList[level]).height() - $(titleElement).height());

    $(this.infoPage).height($(this.infoElement).height() - $(titleElement).height());
    /*for(var i=0; i<this.pageList.length; i++){ 
        if(i==0){
            var reserveWidth = (this.pageList.length - 1) * this.minPreserveWidth;
            $(this.pageList[i]).width($(this.pageElement).width() - reserveWidth);
            $(this.pageList[i]).css('right', reserveWidth+'px');
        }
        else{
            $(this.pageList[i]).width(this.minPreserveWidth);
            var reserveWidth = (this.pageList.length - 1 - i) * this.minPreserveWidth;
            $(this.pageList[i]).css('right', reserveWidth+'px');
            $(this.pageList[i]).addClass(this.myClass.PAGE_SHADOW);   
        }
        var titleElement =
            $(this.pageList[i]).data('titleElement'); 
        var contentElement = 
            $(this.pageList[i]).data('contentElement');
        $(contentElement).height($(this.pageList[i]).height() - $(titleElement).height());
        $(titleElement).width($(this.pageList[i]).width());
        $(contentElement).width($(this.pageList[i]).width());
    }*/
}
