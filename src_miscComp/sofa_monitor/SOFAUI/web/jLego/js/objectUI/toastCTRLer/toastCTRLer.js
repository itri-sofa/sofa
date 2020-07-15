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
jLego.objectUI.toastCTRLer=function(){
    var myID;
    var myClass;
    var myLanguage;
    var myConsts;

    var mainElement;
    
    this.initialize();
}

jLego.objectUI.toastCTRLer.prototype.initialize=function(){
    this.myID = jLego.objectUI.id.toastCTRLer;
    this.myClass = jLego.objectUI.cls.toastCTRLer;
    this.myConsts = jLego.objectUI.constants.toastCTRLer;
    this.myLanguage=jLego.objectUI.lang.toastCTRLer;
}

jLego.objectUI.toastCTRLer.prototype.add=function(){
    this.mainElement = 
        jLego.basicUI.addDiv(document.body, {id: jLego.func.getRandomString(), class: this.myClass.TOASTCTRLER_MAIN_FRAME});
}

jLego.objectUI.toastCTRLer.prototype.addWarning=function(option){
    if(option==null) return;  
    var cardClass = this.myClass.TOASTCTRLER_CARD_WARNING;
    this.addCard(cardClass, jLego.func.getImgPath({category: 'toastCTRLer', name: 'warning', type: 'png'}), option);
}

jLego.objectUI.toastCTRLer.prototype.addError=function(option){
    if(option==null) return;
    var cardClass = this.myClass.TOASTCTRLER_CARD_ERROR;
    this.addCard(cardClass, jLego.func.getImgPath({category: 'toastCTRLer', name: 'error', type: 'png'}), option);
}

jLego.objectUI.toastCTRLer.prototype.addInfo=function(option){
    if(option==null) return;
    var cardClass = this.myClass.TOASTCTRLER_CARD_INFO;
    this.addCard(cardClass, jLego.func.getImgPath({category: 'toastCTRLer', name: 'info', type: 'png'}), option);
}

jLego.objectUI.toastCTRLer.prototype.addSuccess=function(option){
    if(option==null) return;
    var cardClass = this.myClass.TOASTCTRLER_CARD_SUCCESS;
    this.addCard(cardClass, jLego.func.getImgPath({category: 'toastCTRLer', name: 'success', type: 'png'}), option);
}

jLego.objectUI.toastCTRLer.prototype.addCard = function(cardClass, cardIcon, option){
    var outterFrame =
        jLego.basicUI.addDiv(this.mainElement, {id: jLego.func.getRandomString(), class: this.myClass.TOASTCTRLER_CARD + " " + cardClass}); 
    var title =
        jLego.basicUI.addDiv(outterFrame, {id: jLego.func.getRandomString(), class: this.myClass.TOASTCTRLER_CARD_TITLE});
    $(title).text(option.title);
    var contentFrame =
        jLego.basicUI.addDiv(outterFrame, {id: jLego.func.getRandomString(), class: this.myClass.TOASTCTRLER_CARD_CONTENT_FRAME});
    var icon =
        jLego.basicUI.addImg(contentFrame, {id: jLego.func.getRandomString(), class: this.myClass.TOASTCTRLER_CARD_ICON, src: cardIcon});
    var content =
        jLego.basicUI.addDiv(contentFrame, {id: jLego.func.getRandomString(), class: this.myClass.TOASTCTRLER_CARD_CONTENT});
    $(content).text(option.content);
    
    var fadeOutTime = option.fadeOutTime? option.fadeOutTime: 3000;
    var timer = window.setTimeout(function(){
        $(outterFrame).fadeOut(500);
    }, fadeOutTime);
}

