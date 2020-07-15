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

jLego.objectUI.optionCTRLer=function(){
    var myID;
    var myClass;
    var myConsts;
    
    var parentElement;
    var mainElement;
    
    var optionList;
    
    this.initialize();
}

jLego.objectUI.optionCTRLer.prototype.initialize=function(){
    this.myID=jLego.objectUI.id.optionCTRLer;
    this.myClass=jLego.objectUI.cls.optionCTRLer;
    this.myConsts=jLego.objectUI.constants.optionCTRLer;
    
    this.optionList = [];
}

jLego.objectUI.optionCTRLer.prototype.add=function(target){
    if(target==null)    return null;
    
    this.parentElement = target;
    this.mainElement = 
            jLego.basicUI.addDiv(target, {id: jLego.func.getRandomString(), class: this.myClass.OPTIONCTRLER_BACKGROUND});
}

jLego.objectUI.optionCTRLer.prototype.addSelection=function(option){
    var optionIndex  = this.optionList.length;
    
    this.optionList[optionIndex] = 
            jLego.basicUI.addDiv(this.mainElement, {id: jLego.func.getRandomString(), class: this.myClass.OPTIONCTRLER_SELECTION_FRAME});
    var title = 
            jLego.basicUI.addDiv(this.optionList[optionIndex], {id: jLego.func.getRandomString(), class: this.myClass.OPTIONCTRLER_SELECTION_TITLE});
    $(title).text(option.title);
    var select = 
            jLego.basicUI.addSelectOption(this.optionList[optionIndex], {id: jLego.func.getRandomString(), class: this.myClass.OPTIONCTRLER_SELECTION, nameList: option.nameList, valueList: option.valueList});
    if(option.valueList!=null)    $(this.optionList[optionIndex]).data('value', option.valueList[0]);
    else $(this.optionList[optionIndex]).data('value', null);
    if(option.defaultValue!=null){
        var selectOption=select.options;
        for(var i=0; i<selectOption.length; i++){
            if(selectOption[i].value==option.defaultValue){
                selectOption.selectedIndex=i;
                $(this.optionList[optionIndex]).data('value', selectOption[i].value);
            }
        }
    }
    $(select).data('relatedObject', this.optionList[optionIndex]);
    $(select).on('change', function(){
        $($(this).data('relatedObject')).data('value', $(this).val());
    });
    $(this.optionList[optionIndex]).data('select', select);
    return this.optionList[optionIndex];
}
jLego.objectUI.optionCTRLer.prototype.getSelection=function(object){
    return $(object).data('select');
}
jLego.objectUI.optionCTRLer.prototype.addMultipleChoice=function(option){
    
}

jLego.objectUI.optionCTRLer.prototype.addMultipleSelection=function(option){
    var optionIndex  = this.optionList.length;
    
    this.optionList[optionIndex] = 
            jLego.basicUI.addDiv(this.mainElement, {id: jLego.func.getRandomString(), class: this.myClass.OPTIONCTRLER_SELECTION_FRAME});
    var title = 
            jLego.basicUI.addDiv(this.optionList[optionIndex], {id: jLego.func.getRandomString(), class: this.myClass.OPTIONCTRLER_SELECTION_TITLE});
    $(title).text(option.title);
    
    var elementFrame = 
            jLego.basicUI.addDiv(this.optionList[optionIndex], {id: jLego.func.getRandomString(), class: this.myClass.OPTIONCTRLER_MULTISELECTION_FRAME});
    
    $(this.optionList[optionIndex]).data('value', []);
    
    var selectionID = jLego.func.getRandomString() + "_";
    var checkBoxList =[];
    if(option.nameList!=null){
        for(var i=0; i<option.nameList.length; i++){
            $(this.optionList[optionIndex]).data('value')[i]=false;
            var checkBox = 
                    jLego.basicUI.addCheckbox(elementFrame, {id: selectionID +i, 
                                                              class: this.myClass.OPTIONCTRLER_MULTISELECTION_CHECK_BOX,
                                                              name: option.title,
                                                              label: option.nameList[i],
                                                              value: option.valueList[i]
                                                             });
                                                             
             $(checkBox).data('relatedObject', this.optionList[optionIndex]);  
             $(checkBox).data('elementFrame', $(checkBox).data('frame'));
             $(checkBox).data('index', i);
             checkBoxList[checkBoxList.length] = checkBox;
             $(checkBox).on('click', function(){
                 var elementFrame=$(this).data('relatedObject');
                 var index=$(this).data('index');
                 var onCallbackObject=$(this).data('onCallbackObject');
                 if($(this).is(":checked")) $(elementFrame).data('value')[index]=true;
                 else $(elementFrame).data('value')[index]=false;
                 if(onCallbackObject!=null){
                     var callback=onCallbackObject.callback;
                     var arg=onCallbackObject.arg;
                     callback(arg);
                 }
             });
             if(option.defaultValue!=null){
                 for(var j=0; j<option.defaultValue.length; j++){
                     if(option.defaultValue[j].toString() == option.nameList[i].toString())   $(checkBox).click();
                 }
             }
             if(option.unavailableValue!=null){
                 for(var j=0; j<option.unavailableValue.length; j++){
                     if(option.unavailableValue[j].toString().match(option.nameList[i].toString())){
                         $(checkBox).off('click');
                         $(checkBox).data('label').off('click');
                         $(checkBox).data('label').css('opacity', 0.5);
                         $(checkBox).prop("disabled", true);
                     }
                 }
             }
        }
    }
    $(this.optionList[optionIndex]).data('checkBoxkList', checkBoxList);
    return this.optionList[optionIndex];
}
jLego.objectUI.optionCTRLer.prototype.registerOnCheckEvent = function(checkBox, onCallbackObject){
    $(checkBox).data('onCallbackObject', onCallbackObject);
}
jLego.objectUI.optionCTRLer.prototype.getCheckList = function(optionList){
    return $(optionList).data('checkBoxkList');
}
jLego.objectUI.optionCTRLer.prototype.addDayTimeSelection=function(option){
    var optionIndex  = this.optionList.length;
    
    this.optionList[optionIndex] = 
            jLego.basicUI.addDiv(this.mainElement, {id: jLego.func.getRandomString(), class: this.myClass.OPTIONCTRLER_SELECTION_FRAME});
    var title = 
            jLego.basicUI.addDiv(this.optionList[optionIndex], {id: jLego.func.getRandomString(), class: this.myClass.OPTIONCTRLER_SELECTION_TITLE});
    $(title).text(option.title);
    
    $(this.optionList[optionIndex]).data('value', null);
    var inputType, dateFormat, inputHint;
    if(option.inputType!=null) inputType=option.inputType;
    else inputType="text";
    if(option.format!=null) dateFormat=option.format;
    else dateFormat='yy-mm-dd';
    if(option.inputHint!=null) inputHint=option.inputHint;
    else dateFormat='yy-mm-dd';
    var element = 
            jLego.basicUI.addInput(this.optionList[optionIndex], {id: jLego.func.getRandomString(), class: this.myClass.OPTIONCTRLER_DATETIME_INPUT, type: inputType, hint: inputHint});  
    $(element).attr('readonly', true);
    if(option.editable== null || option.editable==true){
        $(element).datetimepicker({dateFormat: dateFormat, timeFormat: 'HH:mm:ss', maxDateTime: option.currentDate, minDateTime: option.minDate});
        $(element).data('option', {dateFormat: dateFormat, timeFormat: 'HH:mm:ss', maxDateTime: option.currentDate, minDateTime: option.minDate});
        $(element).data('lastMaxTime', option.currentDate.getTime());
    }
    else{
        $(element).css('opacity', 0.4);
        $(element).css('cursor', 'not-allowed');
    }
    if(option.defaultValue!=null){
        $(element).val(option.defaultValue);
        $(this.optionList[optionIndex]).data('value', option.defaultValue);
    }
    $(element).data('relatedObject', this.optionList[optionIndex]);
    $(element).on('change', function(){
        $($(this).data('relatedObject')).data('value', $(this).val());
    });
    var image =
            jLego.basicUI.addImg(this.optionList[optionIndex], {id: jLego.func.getRandomString(), class: this.myClass.OPTIONCTRLER_DATETIME_IMAGE, src: jLego.func.getImgPath({category: 'optionCTRLer', name: 'calendar', type: 'png'})});
    var newWidth = $(element).width() - $(image).width() - parseInt($(image).css('margin-left'));
    $(element).width(newWidth);
    $(image).data('relatedObject', element);
    $(image).data('option', $(element).data('option'));
    
    $(image).click(function(){
        var relatedObject = $(this).data('relatedObject');
        $(relatedObject).datetimepicker('show');
    })
    $(this.optionList[optionIndex]).data('datetimepicker', element);
    if(option.autoUpdateMaxDate==true){
        $(element).hover(function(){
            var option = $(this).data('option');
            $(this).data('lastMaxTime', $(this).datepicker( "option", "maxDateTime").getTime());
            var currentDate = new Date();
            currentDate.setTime(parseInt((currentDate.getTime()+1000)*1000)/1000);
            option.maxDateTime = currentDate;
            $(this).datetimepicker('destroy');
            $(this).datetimepicker(option);
        });
        $(image).hover(function(){
             var relatedObject = $(this).data('relatedObject');
            var option = $(this).data('option');
            $(relatedObject).data('lastMaxTime', $(relatedObject).datepicker( "option", "maxDateTime").getTime());
            var currentDate = new Date();
            //option.maxDateTime = currentDate;
            currentDate.setTime(parseInt((currentDate.getTime()+1000)*1000)/1000);
            option.maxDateTime = currentDate;
            $($(this).data('relatedObject')).datetimepicker('destroy');
            $($(this).data('relatedObject')).datetimepicker(option);
        });
    }
    return this.optionList[optionIndex];
}

jLego.objectUI.optionCTRLer.prototype.setDayTimeSelectionToLast=function(element){
    var option = $(element).data('option');
    $(element).data('lastMaxTime', $(element).datepicker( "option", "maxDateTime").getTime());
    var targetDate = new Date();
    targetDate.setTime($(element).datepicker( "option", "maxDateTime").getTime());
    var currentDate = new Date();
    currentDate.setTime(parseInt((currentDate.getTime()+1000)*1000)/1000);
    option.maxDateTime = currentDate;
    $(element).datetimepicker('destroy');
    $(element).datetimepicker(option);
    $(element).datetimepicker('setDate', (targetDate));
}
jLego.objectUI.optionCTRLer.prototype.getDateTimePicker=function(object){
    return $(object).data('datetimepicker');
}
jLego.objectUI.optionCTRLer.prototype.getContentFrame=function(){
    return this.mainElement;
}

jLego.objectUI.optionCTRLer.prototype.getSetValue=function(optionElement){
    if(optionElement==null) return null;
    else return $(optionElement).data('value');
}