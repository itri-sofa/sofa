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



jLego.basicUI.addPanelFrame=function(target, option){
    if(target==null)    return false;

    var defID=jLego.func.getRandomString(jLego.constants.randomIDSize), 
        defClass="", 
        positionX=0, 
        positionY=0;

    if(option!=null){
        if(option.id!=null) defID=option.id;
        if(option.class!=null) defClass=option.class;
        if(option.positionX!=null) positionX=option.positionX;
        if(option.positionY!=null) positionY=option.positionY;
    }
    var newElement = $(document.createElement('div'));
    $(newElement).attr('id', defID);
    $(newElement).attr('class', defClass);
    if(positionX=="align-right"){
        $(newElement).css({
            "position":"absolute", 
            "top": positionY,
            "right": 0
        });
    }
    else if(positionX=="align-left"){
        $(newElement).css({
            "position":"absolute", 
            "top": positionY,
            "left": 0
        });
    }
    else{
        $(newElement).css({
            "position":"absolute", 
            "top": positionY,
            "left": positionX
        });
    }
    newElement.appendTo(target);
    return newElement;
}

jLego.basicUI.addForm=function(target, option){
    if(target==null)    return false;

    var defID=jLego.func.getRandomString(jLego.constants.randomIDSize), 
        defClass="",
        defMethod="",
        defAction="",
        acceptCharset="utf-8";
    if(option!=null){
        if(option.id!=null) defID=option.id;
        if(option.class!=null) defClass=option.class;
        if(option.method!=null) defMethod=option.method;
        if(option.action!=null) defAction=option.action;
        if(option.acceptCharset!=null) acceptCharset=option.acceptCharset;
    }
    
    var newElement = $(document.createElement('form'));
    $(newElement).attr('id', defID);
    $(newElement).attr('class', defClass);
    $(newElement).attr("accept-charset", acceptCharset);
    if(defMethod!="")  $(newElement).attr("method", defMethod);
    if(defAction!="")  $(newElement).attr("action", defAction);

    newElement.appendTo(target);
    return newElement;
}

jLego.basicUI.addDiv=function(target, option){
    if(target==null)    return false;

    var defID=jLego.func.getRandomString(jLego.constants.randomIDSize), 
        defClass="";

    if(option!=null){
        if(option.id!=null) defID=option.id;
        if(option.class!=null) defClass=option.class;
    }
    
    var newElement = $(document.createElement('div'));
    $(newElement).attr('id', defID);
    $(newElement).attr('class', defClass);

    newElement.appendTo(target);
    return newElement;
}

jLego.basicUI.addIFrame=function(target, option){
    if(target==null)    return false;

    var defID=jLego.func.getRandomString(jLego.constants.randomIDSize), 
        defClass="",
        defSrc="";

    if(option!=null){
        if(option.id!=null) defID=option.id;
        if(option.class!=null) defClass=option.class;
        if(option.src!=null) defSrc=option.src;
    }
    
    var newElement = $(document.createElement('iframe'));
    $(newElement).attr('id', defID);
    $(newElement).attr('class', defClass);
    $(newElement).attr('src', defSrc);
    
    newElement.appendTo(target);
    return newElement;
}

jLego.basicUI.addCanvas=function(target, option){
    if(target==null)    return false;

    var defID=jLego.func.getRandomString(jLego.constants.randomIDSize);

    if(option!=null){
        if(option.id!=null) defID=option.id;
    }
    
    $(target).append('<canvas id="' +  defID+ '" width="'+$(target).width()+'px" height="'+$(target).height()+'px"><!-- //comment --> </canvas>');

    return document.getElementById(defID);
}

jLego.basicUI.addLabel=function(target, option){
    if(target==null)    return false;

    var defID=jLego.func.getRandomString(jLego.constants.randomIDSize), 
        defClass="",
        defText="";

    if(option!=null){
        if(option.id!=null) defID=option.id;
        if(option.class!=null) defClass=option.class;
        if(option.text!=null) defText=option.text;
    }
    
    var newElement = $(document.createElement('label'));
    $(newElement).attr('id', defID);
    $(newElement).attr('class', defClass);
    $(newElement).html(defText);
    
    newElement.appendTo(target);
    return newElement;
}

jLego.basicUI.addInput=function(target, option){
    if(target==null)    return false;

    var defID=jLego.func.getRandomString(jLego.constants.randomIDSize), 
        defClass="",
        defName="",
        defHint="",
        defType="";

    if(option!=null){
        if(option.id!=null) defID=option.id;
        if(option.class!=null) defClass=option.class;
        if(option.name!=null) defName=option.name;
        if(option.hint!=null) defHint=option.hint;
        if(option.type!=null) defType=option.type;
    }
    
    var newElement = $(document.createElement('input'));
    $(newElement).attr('id', defID);
    $(newElement).attr('class', defClass);
    $(newElement).attr('name', defName);
    $(newElement).attr('placeholder', defHint);
    $(newElement).attr('type', defType);
    
    newElement.appendTo(target);
    return newElement;
}

jLego.basicUI.addCheckbox=function(target, option){
    if(target==null)    return false;

    var defID=jLego.func.getRandomString(jLego.constants.randomIDSize), 
        defClass="",
        defName="",
        defLabel="",
        defValue="",
        defType="checkbox";

    if(option!=null){
        if(option.id!=null) defID=option.id;
        if(option.name!=null) defName=option.name;
        if(option.class!=null) defClass=option.class;
        if(option.label!=null) defLabel=option.label;
        if(option.value!=null) defValue=option.value;
    }
    var frame = jLego.basicUI.addDiv(target, {id: defID+"_frame", class: defClass});
    var newElement = $(document.createElement('input'));
    
    var newElement = $(document.createElement('input'));
    $(newElement).attr('id', defID);
    $(newElement).attr('name', defName);
    $(newElement).attr('value', defValue);
    $(newElement).attr('type', defType);
    $(newElement).css('float', "left");
    newElement.appendTo(frame);
    
    var label = jLego.basicUI.addDiv(frame, {id: defID+"_label"});
    $(label).css('display', 'table-cell');
    $(label).text(defLabel);
    $(label).css('float', "left");
    $(label).css('margin-right', "3px");
    $(label).click(function(){
        $(newElement).click();
    });
    $(newElement).data('label', label);
    $(newElement).data('frame', frame);
    return newElement;
}

jLego.basicUI.addButton=function(target, option){
    if(target==null)    return false;

    var defID=jLego.func.getRandomString(jLego.constants.randomIDSize), 
        defClass="",
        defName="",
        defLabel="";

    if(option!=null){
        if(option.id!=null) defID=option.id;
        if(option.class!=null) defClass=option.class;
        if(option.name!=null) defName=option.name;
        if(option.label!=null) defLabel=option.label;
    }
    
    var newElement = $(document.createElement('button'));
    $(newElement).attr('id', defID);
    $(newElement).attr('class', defClass);
    $(newElement).attr('name', defName);
    $(newElement).text(defLabel);
    
    newElement.appendTo(target);
    return newElement;
}

jLego.basicUI.addImg=function(target, option){
    if(target==null)    return false;

    var defID=jLego.func.getRandomString(jLego.constants.randomIDSize), 
        defClass="",
        defSrc="";

    if(option!=null){
        if(option.id!=null) defID=option.id;
        if(option.class!=null) defClass=option.class;
        if(option.src!=null) defSrc=option.src;
    }
    
    var newElement = $(document.createElement('img'));
    $(newElement).attr('id', defID);
    $(newElement).attr('class', defClass);
    $(newElement).attr('src', defSrc);
    
    newElement.appendTo(target);
    return newElement;
}

jLego.basicUI.addSelectOption=function(target, option){
    if(target==null)    return false;

    var defID=jLego.func.getRandomString(jLego.constants.randomIDSize), 
        defClass="",
        nameList=[],
        valueList=[];

    if(option!=null){
        if(option.id!=null) defID=option.id;
        if(option.class!=null) defClass=option.class;
        if(option.nameList!=null) nameList=option.nameList;
        if(option.valueList!=null) valueList=option.valueList;
    }
    
    var newElement = $(document.createElement('select'));
    $(newElement).attr('id', defID);
    $(newElement).attr('class', defClass);
    for(var i=0;i<nameList.length; i++){
        if(i===0)   $(newElement).append(new Option(nameList[i], valueList[i], "true", "true"));
        else $(newElement).append(new Option(nameList[i], valueList[i]));
    }
    
    newElement.appendTo(target);
    return newElement;
}

jLego.basicUI.addTable=function(target, option){
    if(target==null)    return false;

    var defID=jLego.func.getRandomString(jLego.constants.randomIDSize), 
        defClass="";

    if(option!=null){
        if(option.id!=null) defID=option.id;
        if(option.class!=null) defClass=option.class;
    }
    
    var newElement = $(document.createElement('table'));
    $(newElement).attr('id', defID);
    $(newElement).attr('class', defClass);
    
    newElement.appendTo(target);
    return newElement;
}

jLego.basicUI.addTableTr=function(target){
    if(target==null)    return false;

    var newElement = $(document.createElement('tr'));
    newElement.appendTo(target);
    return newElement;
}

jLego.basicUI.addTableTh=function(target, option){
    if(target==null)    return false;
    
    var content="";
    
    if(option!=null){
        if(option.content!=null) content=option.content;
    }
    var newElement = $(document.createElement('th'));
    $(newElement).text(content);
    newElement.appendTo(target);
    return newElement;
}

jLego.basicUI.addTableTd=function(target, option){
    if(target==null)    return false;
    
    var content="";
    
    if(option!=null){
        if(option.content!=null) content=option.content;
    }
    var newElement = $(document.createElement('td'));
    $(newElement).text(content);
    newElement.appendTo(target);
    return newElement;
}

jLego.basicUI.addTableDivTd=function(target, option){
    if(target==null)    return false;
    
    var content="";
    
    if(option!=null){
        if(option.content!=null) content=option.content;
    }
    var newElement = $(document.createElement('td'));
    content.appendTo(newElement);
    newElement.appendTo(target);
    return newElement;
}

jLego.basicUI.addParagraph=function(target, option){
    if(target==null)    return false;
    
    var content="";
    
    if(option!=null){
        if(option.content!=null) content=option.content;
    }
    var newElement = $(document.createElement('p'));
    newElement.appendTo(target);
    $(newElement).text(content);
    return newElement;
}
