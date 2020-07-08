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

jLego.objectUI.searchCTRLer=function(){
    var myID;
    var myClass;
    var myConsts;
    
    var mainElement;
    var category;
    var inputBox;
    
    this.initialize();
}

jLego.objectUI.searchCTRLer.prototype.initialize=function(){
    this.myID=jLego.objectUI.id.searchCTRLer;
    this.myClass=jLego.objectUI.cls.searchCTRLer;
    this.myConsts=jLego.objectUI.constants.searchCTRLer;
}

jLego.objectUI.searchCTRLer.prototype.add=function(target, option, callbackObject){
    //check option
    if(target==null) return null;
    if(option==null)    var option={nameList: [], valueList: [], top: 0, left: 0};
    else{
        if(option.nameList==null)  option.nameList=[];
        if(option.valueList==null)  option.valueList=[];
        if(option.top==null)  option.top=0;
        if(option.left==null && option.right==null)  option.left=0;
        else if(option.right!=null) option.left=null;
    }
    this.mainElement =
            jLego.basicUI.addDiv(target, {id: jLego.func.getRandomString(), class: this.myClass.SEARCH_FRAME});

    $(this.mainElement).css('top', option.top + 'px');
    if(option.right!=null) $(this.mainElement).css('right', option.right + 'px');
    else $(this.mainElement).css('left', option.left + 'px');
    
    this.category = 
            jLego.basicUI.addSelectOption(this.mainElement, {id: jLego.func.getRandomString(), class: this.myClass.SEARCH_OPTION, nameList: option.nameList, valueList: option.valueList});
    $(this.category).data('relatedObject', option.valueList[0]);
    if(callbackObject!=null){
        $(this.category).data('onClickCallback', callbackObject.callback);
        $(this.category).data('onClickArg', callbackObject.arg);
    }
    
    var searchFrame =
            jLego.basicUI.addDiv(this.mainElement, {id: jLego.func.getRandomString(), class: this.myClass.SEARCH_BOX});
    this.inputBox =
            jLego.basicUI.addInput(searchFrame, {id: jLego.func.getRandomString(), class: this.myClass.SEARCH_BOX_ELEMENT, type: 'text', name: ''});  
    if(callbackObject!=null){
        $(this.inputBox).data('onClickCallback', callbackObject.callback);
        $(this.inputBox).data('onClickArg', callbackObject.arg);
    }
    var searchIcon =
            jLego.basicUI.addImg(searchFrame, {id: jLego.func.getRandomString(), class: this.myClass.SEARCH_BOX_ICON, src: jLego.func.getImgPath({category: 'searchCTRLer', name: 'search', type: 'png'})});
    
    $(this.category).on('change', function(){
        $($(this).data('relatedObject')).data('value', $(this).val());
        var callback = $(this).data('onClickCallback');
        var arg = $(this).data('onClickArg');
        if(callback!=null){
            callback(arg);
        }
    });
    $(this.inputBox).on('keyup', function(){
        $(this).data('value', $(this).val());
        var callback = $(this).data('onClickCallback');
        var arg = $(this).data('onClickArg');
        if(callback!=null){
            callback(arg);
        }
    });
    return this.mainElement;
}

jLego.objectUI.searchCTRLer.prototype.getSelectOpiton=function(){
    return $(this.category).val();
}
jLego.objectUI.searchCTRLer.prototype.setSelectOpiton=function(key){
    $(this.category).val(key);
}
jLego.objectUI.searchCTRLer.prototype.getInputContent=function(){
    return $(this.inputBox).val();
}
jLego.objectUI.searchCTRLer.prototype.setInputContent=function(content){
    $(this.inputBox).val(content);
    $(this.inputBox).trigger('keyup');
}
jLego.objectUI.searchCTRLer.prototype.show=function(){
    $(this.mainElement).show();
}
jLego.objectUI.searchCTRLer.prototype.hide=function(){
    $(this.mainElement).hide();
}

