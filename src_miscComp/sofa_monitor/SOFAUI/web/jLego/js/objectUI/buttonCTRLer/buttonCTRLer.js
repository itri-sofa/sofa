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

jLego.objectUI.buttonCTRLer=function(){
    var myID;
    var myClass;
    var myConsts;
    
    var singleButtonList;
    var arrayButtonList;

    var parentFrame;
    var listButton;
    var myID;
    this.initialize();
}

jLego.objectUI.buttonCTRLer.prototype.initialize=function(){
    this.myID=jLego.objectUI.id.buttonCTRLer;
    this.myClass=jLego.objectUI.cls.buttonCTRLer;
    this.myConsts=jLego.objectUI.constants.buttonCTRLer;
    
    this.myID = jLego.func.getRandomString();
    
    this.singleButtonList=[];
    this.arrayButtonList=[];
    
    //$(this.listButton).hide();
}

jLego.objectUI.buttonCTRLer.prototype.addSingleButton=function(target, option, callbackObject){
    //check option
    if(target==null) return null;
    if(option==null)    var option={title: ''};
    else{
        if(option.title==null)  option.title='';
        if(option.type==null)  option.type='positive';
    }
    var buttonClass;
    if(option.type=='positive') buttonClass = this.myClass.SINGLE_BUTTON_FRAME;
    else if(option.type=='noraml') buttonClass = this.myClass.SINGLE_BUTTON_FRAME_NORMAL;
    else buttonClass = this.myClass.SINGLE_BUTTON_FRAME;
    var button =
            jLego.basicUI.addDiv(target, {id: jLego.func.getRandomString(), class: buttonClass});
    $(button).text(option.title);
    
    var buttonIndex = this.singleButtonList.length;
    this.singleButtonList[buttonIndex] = button;
    
    $(this.singleButtonList[buttonIndex]).data('type', 'singleButton');
    
    if(callbackObject!=null){
        $(this.singleButtonList[buttonIndex]).data('onClickArg', callbackObject.arg);
        $(this.singleButtonList[buttonIndex]).data('onClickCallback', callbackObject.callback);
    }
    $(this.singleButtonList[buttonIndex]).click(function(){
        var callback = $(this).data('onClickCallback');
        var arg = $(this).data('onClickArg');

        if(callback!=null){
            callback(arg);
        }
    });
    
    return button;
}

jLego.objectUI.buttonCTRLer.prototype.addFreeSingleButton=function(target, option, callbackObject){
    //check option
    if(target==null) return null;
    if(option==null)    var option={title: '', top: 0, left: 0, icon: null};
    else{
        if(option.title==null)  option.title='';
        if(option.type==null)  option.type='normal';
        if(option.top==null)  option.top=0;
        if(option.left==null && option.right==null)  option.left=0;
        else if(option.right!=null) option.left=null;
    }
    var buttonClass;
    if(option.type=='positive') buttonClass = this.myClass.SINGLE_FREEBUTTON_FRAME + " buttonCTRLer_positiveColor";
    else if(option.type=='info') buttonClass = this.myClass.SINGLE_FREEBUTTON_FRAME + " buttonCTRLer_infoColor";
    else if(option.type=='negtive') buttonClass = this.myClass.SINGLE_FREEBUTTON_FRAME + " buttonCTRLer_negtiveColor";
    else if(option.type=='noraml') buttonClass = this.myClass.SINGLE_FREEBUTTON_FRAME_NORMAL  + " buttonCTRLer_normalColor";
    else buttonClass = this.myClass.SINGLE_FREEBUTTON_FRAME_NORMAL  + " buttonCTRLer_normalColor";
    var button =
            jLego.basicUI.addDiv(target, {id: jLego.func.getRandomString(), class: buttonClass});
    $(button).text(option.title);
    if(!option.float){
        $(button).css('top', option.top + 'px');
        if(option.right!=null) $(button).css('right', option.right + 'px');
        else $(button).css('left', option.left + 'px');
    }
    else{
        $(button).css('position', 'relative');
        $(button).css('float', option.float);
        $(button).css('margin-top', option.top + 'px');
        if(option.right!=null) $(button).css('margin-right', option.right + 'px');
        else $(button).css('margin-left', option.left + 'px');
    }
    if(option.style=='small') $(button).addClass(this.myClass.SINGLE_FREEBUTTON_FRAME_SMALL);
    if(option.icon!=null){
        var icon = 
                jLego.basicUI.addImg(button, {id: jLego.func.getRandomString(), class: this.myClass.SINGLE_FREEBUTTON_ICON, src: jLego.func.getImgPath({category: 'buttonIcon', name: option.icon, type: 'png'})});
        $(button).css('text-indent', "20px");
    }
    
    if(option.iconURL!=null){
        var icon = 
                jLego.basicUI.addImg(button, {id: jLego.func.getRandomString(), class: this.myClass.SINGLE_FREEBUTTON_ICON, src: option.iconURL});
        $(button).css('text-indent', "20px");
    }
    
    if(option.width!=null){
        $(button).width(parseInt(option.width));
    }
    var buttonIndex = this.singleButtonList.length;
    this.singleButtonList[buttonIndex] = button;
    
    $(this.singleButtonList[buttonIndex]).data('type', 'singleFreeButton');
    
    if(callbackObject!=null){
        $(this.singleButtonList[buttonIndex]).data('onClickArg', callbackObject.arg);
        $(this.singleButtonList[buttonIndex]).data('onClickCallback', callbackObject.callback);
    }
    $(this.singleButtonList[buttonIndex]).click(function(){
        var callback = $(this).data('onClickCallback');
        var arg = $(this).data('onClickArg');

        if(callback!=null){
            callback(arg);
        }
    });
    
    return button;
}

jLego.objectUI.buttonCTRLer.prototype.addFreeSwitch=function(target, option){
    //check option
    if(target==null) return null;
    if(option==null) return null;
    else{
        if(option.nameList==null)  return null;
        if(option.switchSize==null)  option.switchSize=50;
        if(option.top==null)  option.top=0;
        if(option.left==null && option.right==null)  option.left=0;
        else if(option.right!=null) option.left=null;
    }
    //add switch frame
    var switchFrame = 
            jLego.basicUI.addDiv(target, {id: jLego.func.getRandomString(), class: this.myClass.SWITCH_FRAME});
    $(switchFrame).width( (option.switchSize+2) * option.nameList.length);
    $(switchFrame).css('top', option.top + 'px');
    if(option.right!=null) $(switchFrame).css('right', option.right + 'px');
    else $(switchFrame).css('left', option.left + 'px');
    //add switch
    var switchList = [];
    for(var i=0; i<option.nameList.length; i++){
        var switchClass = this.myClass.SWITCH;
        if(i==0) switchClass = this.myClass.SWITCH_FIRST;
        else if(i==option.nameList.length-1) switchClass = this.myClass.SWITCH_LAST;
        switchList[i] = 
            jLego.basicUI.addDiv(switchFrame, {id: jLego.func.getRandomString(), class: switchClass});   
        $(switchList[i]).text(option.nameList[i]);
        $(switchList[i]).width(option.switchSize);
        $(switchList[i]).data('index', i);
        $(switchList[i]).data('parentElement', switchFrame);
        $(switchList[i]).data('parent', this);
        //only one switch
        if(option.nameList.length==1){
            $(switchList[i]).css('border-radius', '2px');
            $(switchList[i]).css('border', '1px solid #006CCF');
        }
        //only two switches
        if(option.nameList.length==2){
            $(switchList[0]).css('border-right', '1px solid #006CCF');
        }
        //set click event
        $(switchList[i]).click(function(){
            var parent = $(this).data('parent');
            var parentElement = $(this).data('parentElement');
            var currentSelectIndex = $(parentElement).data('currentIndex');
            var switchList = $(parentElement).data('switchList');
            var index = $(this).data('index');
            if(currentSelectIndex!=index){
                if(currentSelectIndex!=null){
                    $(switchList[currentSelectIndex]).removeClass(parent.myClass.SWITCH_ON);
                }
                $(switchList[index]).addClass(parent.myClass.SWITCH_ON);
                $(parentElement).data('currentIndex', index);
                var callback = $(this).data('onClickCallback');
                var arg = $(this).data('onClickArg');

                if(callback!=null){
                    callback(arg);
                }
            }
        });
    }
    $(switchFrame).data('currentIndex', null);
    $(switchFrame).data('switchList', switchList);
    
    return switchFrame;
}

jLego.objectUI.buttonCTRLer.prototype.getSwitchOfIndex=function(switchFrame, index){
    var switchList = $(switchFrame).data('switchList');
    return switchList[index];
}
jLego.objectUI.buttonCTRLer.prototype.setSwitchCallback=function(switchElement, callbackObject){
    $(switchElement).data('onClickCallback', callbackObject.callback);
    $(switchElement).data('onClickArg', callbackObject.arg);
}

jLego.objectUI.buttonCTRLer.prototype.addFreeIconButton=function(target, option, callbackObject){
    //check option
    if(target==null) return null;
    if(option==null)    var option={title: null, top: 0, left: 0, icon: null, size: null};
    else{
        if(option.type==null)  option.type='normal';
        if(option.top==null)  option.top=0;
        if(option.left==null && option.right==null)  option.left=0;
        else if(option.right!=null) option.left=null;
    }
    var buttonClass;
    if(option.type=='positive') buttonClass = this.myClass.SINGLE_FREEICONBUTTON_FRAME;
    else if(option.type=='noraml') buttonClass = this.myClass.SINGLE_FREEICONBUTTON_FRAME_NORMAL;
    else buttonClass = this.myClass.SINGLE_FREEICONBUTTON_FRAME_NORMAL;
    
    var iconSrc;
    if(option.icon!=null) iconSrc = jLego.func.getImgPath({category: 'buttonIcon', name: option.icon, type: 'png'});
    else iconSrc = jLego.func.getImgPath({category: 'buttonIcon', name:'default', type: 'png'});
    
    var button = 
                jLego.basicUI.addImg(target, {id: jLego.func.getRandomString(), class: buttonClass, src: iconSrc});
    
    if(option.size!=null){
        $(button).width(parseInt(option.size));
        $(button).height(parseInt(option.size));
    }
    if(option.title!=null){
        $(button).attr('title', option.title);
        $(button).tooltip();
    }
    $(button).css('top', option.top + 'px');
    if(option.right!=null) $(button).css('right', option.right + 'px');
    else $(button).css('left', option.left + 'px');
    
    
    
    var buttonIndex = this.singleButtonList.length;
    this.singleButtonList[buttonIndex] = button;
    
    $(this.singleButtonList[buttonIndex]).data('type', 'singleFreeIconButton');
    
    if(callbackObject!=null){
        $(this.singleButtonList[buttonIndex]).data('onClickArg', callbackObject.arg);
        $(this.singleButtonList[buttonIndex]).data('onClickCallback', callbackObject.callback);
    }
    $(this.singleButtonList[buttonIndex]).click(function(){
        var callback = $(this).data('onClickCallback');
        var arg = $(this).data('onClickArg');

        if(callback!=null){
            callback(arg);
        }
    });
    
    return button;
}

jLego.objectUI.buttonCTRLer.prototype.addArrayButton=function(target, option){
    $(target).addClass(this.myClass.ARRAY_BUTTON_FRAME_PARENT);
    //check option
    if(target==null) return null;
    if(option==null)    var option={nameList: []};
    else{
        if(option.nameList==null)  option.nameList=[];
    }

    var defID = jLego.func.getRandomString();
    var buttonFrame =
            jLego.basicUI.addDiv(target, {id: defID, class: this.myClass.ARRAY_BUTTON_FRAME});
    var widthLimit = $(buttonFrame).width() - 20;
    
    var buttonArray =[];
    var buttonClass;
    var widthUsage = 10;
    var showMore = false;
    var moreButton;
    for(var i=0; i<option.nameList.length; i++){
        if(i==0){
             if (option.nameList.length==1) buttonClass =  this.myClass.ARRAY_BUTTON_FIRST_LAST;
             else buttonClass =  this.myClass.ARRAY_BUTTON_FIRST;
        }
        else if(i!=0 && i==option.nameList.length-1) buttonClass =  this.myClass.ARRAY_BUTTON_LAST;
        else buttonClass = this.myClass.ARRAY_BUTTON;
        
        buttonArray[i] =
            jLego.basicUI.addDiv(buttonFrame, {id: defID + "_" + i, class: buttonClass});
        $(buttonArray[i]).text(option.nameList[i]);
        
        widthUsage += ($(buttonArray[i]).width() + parseInt($(buttonArray[i]).css('padding-left')) + parseInt($(buttonArray[i]).css('padding-right')));
        if(widthUsage > widthLimit){
            $(buttonArray[i]).hide();
            if(showMore==false){
                var availableWidth = widthLimit - (widthUsage-$(buttonArray[i]).width());
                if(availableWidth < this.myConsts.MORE_BUTTON_WIDTH){
                    if(i!=option.nameList.length-1 && i>0){    //need extra detail button
                        $(buttonArray[i-1]).hide();
                    }
                }
            }
            showMore=true;
        }
    }
    moreButton =
        jLego.basicUI.addDiv(buttonFrame, {id: defID + "_more", class: this.myClass.ARRAY_BUTTON_LAST});
    $(moreButton).text("More ▼");
    $(moreButton).data('parent', this);
    $(moreButton).css('margin-left', "4px");
    $(moreButton).data('buttonArray', buttonArray);
    this.setMoreButtonClick(moreButton);
    if(showMore==false){
        $(moreButton).hide();
    }
    var arrayButtonIndex = this.arrayButtonList.length;
    this.arrayButtonList[arrayButtonIndex] = buttonFrame;
    $(this.arrayButtonList[arrayButtonIndex]).data('parentFrame', buttonFrame);
    $(this.arrayButtonList[arrayButtonIndex]).data('buttonArray', buttonArray);
    $(this.arrayButtonList[arrayButtonIndex]).data('moreButton', moreButton);
    return this.arrayButtonList[arrayButtonIndex];
}

jLego.objectUI.buttonCTRLer.prototype.setArrayButtonClick=function(arrayButtonElement, index, callbackObject){
    var buttonArray = $(arrayButtonElement).data('buttonArray');
    if(index >= buttonArray.length) return null;
    if(callbackObject!=null){
        $(buttonArray[index]).data('onClickArg', callbackObject.arg);
        $(buttonArray[index]).data('onClickCallback', callbackObject.callback);
    }
    $(buttonArray[index]).click(function(){
        var callback = $(this).data('onClickCallback');
        var arg = $(this).data('onClickArg');
        if(callback!=null){
            callback(arg);
        }
    });
}

jLego.objectUI.buttonCTRLer.prototype.setMoreButtonClick=function(moreButton){
    $(moreButton).click(function(event){
        
        var parent = $(this).data('parent');
        var buttonArray = $(this).data('buttonArray');
        
        var pageX = event.pageX - $(parent.parentFrame).offset().left  + $(parent.parentFrame).scrollLeft();
        var pageY = event.pageY - $(parent.parentFrame).offset().top + $(parent.parentFrame).scrollTop();

        if($(parent.listButton).is(":visible")){
            $(parent.listButton).slideUp('fast');
        }
        else{
            $(parent.listButton).html('');
            for(var i=0; i<buttonArray.length; i++){
                if(!$(buttonArray[i]).is(":visible")){
                    var buttonClass = parent.myClass.LIST_BUTTON;
                    if(i==buttonArray.length-1) buttonClass=parent.myClass.LIST_BUTTON_LAST;
                    var newButton =
                        jLego.basicUI.addDiv(parent.listButton, {id: jLego.func.getRandomString(), class: buttonClass});    
                    $(newButton).text($(buttonArray[i]).text());
                    $(newButton).data('relatedButtonArray', buttonArray[i]);
                    $(newButton).data('listButton', parent.listButton);
                    $(newButton).click(function(){
                        var myButtonArray = $(this).data('relatedButtonArray');
                        var callback = $(myButtonArray).data('onClickCallback');
                        var arg = $(myButtonArray).data('onClickArg');
                        var listButton = $(this).data('listButton');
                        if(callback!=null){
                            $(listButton).slideUp('fast');
                            callback(arg);
                        }
                    });
                }
            }
            var newLeft = pageX - $(parent.listButton).width()/1.5;
            var newTop = pageY + 20;
            $(parent.listButton).css('left', newLeft+"px");
            $(parent.listButton).css('top', newTop+"px");
            $(parent.listButton).slideDown('fast');
        }
    });
}
jLego.objectUI.buttonCTRLer.prototype.resizeArrayButton=function(arrayButton){
    var target = $(arrayButton).data('parentFrame');
    var widthLimit = $(target).width() - 20;
    var buttonArray = $(arrayButton).data('buttonArray');
    var moreButton = $(arrayButton).data('moreButton');
    var widthUsage = 10;
    var showMore = false;
    for(var i=0; i<buttonArray.length; i++){
        widthUsage += ($(buttonArray[i]).width() + parseInt($(buttonArray[i]).css('padding-left')) + parseInt($(buttonArray[i]).css('padding-right')));
        if(widthUsage > widthLimit){
            $(buttonArray[i]).hide();
            if(showMore==false){
                var availableWidth = widthLimit - (widthUsage-$(buttonArray[i]).width());
                if(availableWidth < this.myConsts.MORE_BUTTON_WIDTH){
                    if(moreButton!=null && i>0){    //need extra detail button
                        $(buttonArray[i-1]).hide();
                    }
                }
            }
            showMore=true;
        }
        else{
            $(buttonArray[i]).show();
        }
    }
    if(showMore==false) $(moreButton).hide();
    else if(showMore==true){
        if(!$(buttonArray[0]).is(":visible")) $(moreButton).attr('class', this.myClass.ARRAY_BUTTON_FIRST_LAST);
        else $(moreButton).attr('class', this.myClass.ARRAY_BUTTON_LAST);
        $(moreButton).show();
    }
    
}

jLego.objectUI.buttonCTRLer.prototype.prepareListButton=function(parentFrame){
    this.parentFrame = parentFrame;
    if(this.listButton==null){
        var newListButton =
            jLego.basicUI.addDiv(this.parentFrame, {id: jLego.func.getRandomString(), class: this.myClass.LIST_BUTTON_FRAME});
        this.listButton = newListButton;
        $(this.listButton).hide();
    }
    return this.listButton;
}

jLego.objectUI.buttonCTRLer.prototype.resize=function(){
    for(var i=0; i<this.arrayButtonList.length; i++){
        this.resizeArrayButton(this.arrayButtonList[i]);
        $(this.listButton).hide();
    }
}