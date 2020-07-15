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

jLego.objectUI.nodeCard=function(){
    var myID;
    var myClass;
    var myConsts;
    
    var cardList;
    this.initialize();
}

jLego.objectUI.nodeCard.prototype.initialize=function(){
    this.myID=jLego.objectUI.id.nodeCard;
    this.myClass=jLego.objectUI.cls.nodeCard;
    this.myConsts=jLego.objectUI.constants.nodeCard;
    this.cardList = [];
}

jLego.objectUI.nodeCard.prototype.addDiskCard=function(target, option){
    //check option
    if(target==null) return null;
    if(option==null) return null;
    else{
        if(option.title==null)   return null;
        else if(option.width==null)   return null;
        else if(option.height==null)   return null;
    }
    var defIndex = this.cardList.length;
    var defID = this.myID.STATUSMONITOR_MONITORFRAME + defIndex;
    var mainElement = 
            jLego.basicUI.addDiv(target, {id: defID, class: this.myClass.NODECARD_DISKCARD_FRAME});
    $(mainElement).height(option.height);
    $(mainElement).width(option.width);
    var titleElement =
            jLego.basicUI.addDiv(mainElement, {id: defID + "_title", class: this.myClass.NODECARD_DISKCARD_TITLE});
    $(titleElement).text(option.title);
    var freeElement =
            jLego.basicUI.addDiv(mainElement, {id: defID + "_freeFrame", class: this.myClass.NODECARD_DISKCARD_FREEELEMENT});
    var buttonElement = 
            jLego.basicUI.addDiv(mainElement, {id: defID + "_buttonFrame", class: this.myClass.NODECARD_DISKCARD_BUTTONELEMENT});
    
    $(freeElement).height($(mainElement).height() - $(titleElement).height() - $(buttonElement).height());
    
    $(mainElement).data('titleElement', titleElement);
    $(mainElement).data('freeElement', freeElement);
    $(mainElement).data('buttonElement', buttonElement);
    
    this.cardList[this.cardList.length] = mainElement;
    return mainElement;
}

jLego.objectUI.nodeCard.prototype.getDiskCardFreeFrame = function(diskCard){
    return $(diskCard).data('freeElement');
}
jLego.objectUI.nodeCard.prototype.getDiskCardButtonFrame = function(diskCard){
    return $(diskCard).data('buttonElement');
}