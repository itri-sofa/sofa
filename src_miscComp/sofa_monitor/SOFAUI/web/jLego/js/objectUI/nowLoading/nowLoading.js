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

jLego.objectUI.nowLoading=function(){
    var myID;
    var myClass;
    var myConsts;
    
    this.mainElement;
    this.loadingIcon;
    
    this.totalRotateStep;
    this.rotateStep;
    this.timer;
    
    this.initialize();
}

jLego.objectUI.nowLoading.prototype.initialize=function(){
    this.myID=jLego.objectUI.id.nowLoading;
    this.myClass=jLego.objectUI.cls.nowLoading;
    this.myConsts=jLego.objectUI.constants.nowLoading;
    this.myLanguage = jLego.objectUI.lang.nowLoading;
    
    this.totalRotateStep=10;
    this.rotateStep=0;
}

jLego.objectUI.nowLoading.prototype.add=function(target, option){
    if(jLego.func.isInIframe()){
        //alert(window.self.parent.document)
        //var target = window.self.parent.document;
    }
    this.mainElement = 
            jLego.basicUI.addDiv(target, {id: jLego.func.getRandomString(), class: this.myClass.LOADING_BACKGROUND});
    var foregroundElement = 
            jLego.basicUI.addDiv(this.mainElement, {id: jLego.func.getRandomString(), class: this.myClass.LOADING_FOREGROUND});
    this.loadingIcon = 
            jLego.basicUI.addImg(foregroundElement, {id: jLego.func.getRandomString(), class: this.myClass.LOADING_ICON, src: jLego.func.getImgPath({category: 'nowLoading', name: 'loading', type: 'png'})});
    var loadingText = 
            jLego.basicUI.addDiv(foregroundElement, {id: jLego.func.getRandomString(), class: this.myClass.LOADING_TEXT});
    if(option==null){
        $(loadingText).text(this.myLanguage.LOADING);
    }
    else{
        if(option.loadingText!=null){
            $(loadingText).text(option.loadingText);
        }
        else{
            $(loadingText).text(this.myLanguage.LOADING);
        }
    }
    
    jLego.variables.tempOjbect = this;
    this.timer = setInterval(function(){
        var parent = jLego.variables.tempOjbect;
        parent.rotating();
    }, 100);
}

jLego.objectUI.nowLoading.prototype.rotating=function(){
    var singleStepAngle = 360/this.totalRotateStep;
    var newRotateStep = parseInt((this.rotateStep+1) % this.totalRotateStep);
    this.rotateStep = newRotateStep;
    var angle = singleStepAngle * newRotateStep;
     $(this.loadingIcon).css({
        "-webkit-transform": "rotate("+angle+"deg)",
        "-moz-transform": "rotate("+angle+"deg)",
        "transform": "rotate("+angle+"deg)"});
}

jLego.objectUI.nowLoading.prototype.close=function(){
    $(this.mainElement).remove();
    window.clearInterval(this.timer);
}