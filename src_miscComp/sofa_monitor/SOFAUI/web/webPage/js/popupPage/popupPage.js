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

popupPage = function(){
    var myID;
    var myClass;
    var myConsts;
    var myLanguage;
    var myVariable;
    
    var panelCTRLer;
    
    var smartTableFrame;
    var smartTableCTRLer;
    var smartDetailTableFrame;
    var smartDetailTableCTRLer;
    
    var buttonCTRLer;
    this.initialize();
}

popupPage.prototype.initialize=function(){
    this.myID=webView.id.popupPage;
    this.myClass=webView.cls.popupPage;
    this.myConsts=webView.constants.popupPage;
    this.myLanguage=webView.lang.popupPage;
    this.myVariable=webView.variables.popupPage;
    
    this.buttonCTRLer = new jLego.objectUI.buttonCTRLer();
}

popupPage.prototype.addSMART=function(panelCTRLer, Jdata){
    this.myVariable.panelCTRLer = panelCTRLer;
    this.smartTableFrame = 
            jLego.basicUI.addDiv(panelCTRLer.getContentFrame(), {id: jLego.func.getRandomString(), class: this.myClass.SMART_FRAME});
    $(this.smartTableFrame).data('parentFrame', panelCTRLer.getContentFrame());
    var newWidth = $(panelCTRLer.getContentFrame()).width() - 40;
    var newHieght = $(panelCTRLer.getContentFrame()).height() - 40;
    $(this.smartTableFrame).width(newWidth);
    $(this.smartTableFrame).height(newHieght);
    $(this.smartTableFrame).css('left', '20px');
    $(this.smartTableFrame).css('padding-top', '20px');
    $(this.smartTableFrame).css('padding-bottom', '20px');
    $(this.smartTableFrame).css('padding-right', '20px');
    this.smartTableCTRLer = new jLego.objectUI.nodeTable();
    this.smartTableCTRLer.add(this.smartTableFrame, {
        columnTitleList: this.myLanguage.SMART_NODE_TABLE_TITLE,
        widthType: this.myLanguage.SMART_NODE_TABLE_TITLE_WIDTH_TYPE,
        minWidthList: this.myLanguage.SMART_NODE_TABLE_TITLE_WIDTH,
        rowCount: Jdata.attributes.length
    });
    var container;
    for(var i=0; i<Jdata.attributes.length; i++){
        container = this.smartTableCTRLer.getContainer(i, 0);
        var id = 
            jLego.basicUI.addDiv(container, {id: jLego.func.getRandomString(), class: ""});
        $(id).text(Jdata.attributes[i].attrID);
        container = this.smartTableCTRLer.getContainer(i, 1);
        var attribute = 
            jLego.basicUI.addDiv(container, {id: jLego.func.getRandomString(), class: ""});
        $(attribute).text(Jdata.attributes[i].attrName);
        container = this.smartTableCTRLer.getContainer(i, 2);
        var value = 
            jLego.basicUI.addDiv(container, {id: jLego.func.getRandomString(), class: ""});
        $(value).text(Jdata.attributes[i].attrValue);
        container = this.smartTableCTRLer.getContainer(i, 3);
        var worst = 
            jLego.basicUI.addDiv(container, {id: jLego.func.getRandomString(), class: ""});
        $(worst).text(Jdata.attributes[i].attrWorst);
        container = this.smartTableCTRLer.getContainer(i, 4);
        var threshold = 
            jLego.basicUI.addDiv(container, {id: jLego.func.getRandomString(), class: ""});
        $(threshold).text(Jdata.attributes[i].attrThreshold);
        container = this.smartTableCTRLer.getContainer(i, 5);
        var rawValue = 
            jLego.basicUI.addDiv(container, {id: jLego.func.getRandomString(), class: ""});
        $(rawValue).text(Jdata.attributes[i].attrRawValue);
    }
    
    this.addSMARTDetail(panelCTRLer, Jdata);
    var seeDetailButton =
        this.buttonCTRLer.addFreeSingleButton(panelCTRLer.getFootFrame(), {type: 'positive', top: 4, right: 5, title: this.myLanguage.DETAIL});
    $(seeDetailButton).data('isDetailMode', false);
    $(seeDetailButton).data('parent', this);
    $(seeDetailButton).click(function(){
        var isDetailMode = $(this).data('isDetailMode');
        var parent = $(this).data('parent');
        if(isDetailMode==false){
             $(this).data('isDetailMode', true);
             $(parent.smartDetailTableFrame).show();
             $(parent.smartTableFrame).hide();
             $(this).text(parent.myLanguage.BACK);
        }
        else{
            $(this).data('isDetailMode', false);
            $(parent.smartTableFrame).show();
            $(parent.smartDetailTableFrame).hide();
            $(this).text(parent.myLanguage.DETAIL);
        }
    });
    this.smartTableCTRLer.resize();
}

popupPage.prototype.addSMARTDetail=function(panelCTRLer, Jdata){
    this.smartDetailTableFrame = 
            jLego.basicUI.addDiv(panelCTRLer.getContentFrame(), {id: jLego.func.getRandomString(), class: this.myClass.SMART_FRAME});
    $(this.smartDetailTableFrame).hide();
    var newWidth = $(panelCTRLer.getContentFrame()).width() - 40;
    var newHieght = $(panelCTRLer.getContentFrame()).height() - 40;
    $(this.smartDetailTableFrame).width(newWidth);
    $(this.smartDetailTableFrame).height(newHieght);
    $(this.smartDetailTableFrame).css('left', '20px');
    $(this.smartDetailTableFrame).css('padding-top', '20px');
    $(this.smartDetailTableFrame).css('padding-bottom', '20px');
    $(this.smartDetailTableFrame).css('padding-right', '20px');
    this.smartDetailTableCTRLer = new jLego.objectUI.nodeTable();
    this.smartDetailTableCTRLer.add(this.smartDetailTableFrame, {
        columnTitleList: this.myLanguage.SMART_DETAIL_NODE_TABLE_TITLE,
        widthType: this.myLanguage.SMART_DETAIL_NODE_TABLE_TITLE_WIDTH_TYPE,
        minWidthList: this.myLanguage.SMART_DETAIL_NODE_TABLE_TITLE_WIDTH,
        rowCount: 9
    });
    //smartHealth
    var titleContainer = this.smartDetailTableCTRLer.getContainer(0, 0);
    var valueContainer = this.smartDetailTableCTRLer.getContainer(0, 1);
    var title = 
        jLego.basicUI.addDiv(titleContainer, {id: jLego.func.getRandomString(), class: ""});
    $(title).text(this.myLanguage.SMART_HEALTH);
    var value = 
        jLego.basicUI.addDiv(valueContainer, {id: jLego.func.getRandomString(), class: ""});
    $(value).text(Jdata.smartHealth);
    //deviceModel
    titleContainer = this.smartDetailTableCTRLer.getContainer(1, 0);
    valueContainer = this.smartDetailTableCTRLer.getContainer(1, 1);
    title = 
        jLego.basicUI.addDiv(titleContainer, {id: jLego.func.getRandomString(), class: ""});
    $(title).text(this.myLanguage.DEVICE_MODEL);
    value = 
        jLego.basicUI.addDiv(valueContainer, {id: jLego.func.getRandomString(), class: ""});
    $(value).text(Jdata.deviceModel);
    //firmwareVersion
    titleContainer = this.smartDetailTableCTRLer.getContainer(2, 0);
    valueContainer = this.smartDetailTableCTRLer.getContainer(2, 1);
    title = 
        jLego.basicUI.addDiv(titleContainer, {id: jLego.func.getRandomString(), class: ""});
    $(title).text(this.myLanguage.FIRMWARE_VERSION);
    value = 
        jLego.basicUI.addDiv(valueContainer, {id: jLego.func.getRandomString(), class: ""});
    $(value).text(Jdata.firmwareVersion);
    //serialNumber
    titleContainer = this.smartDetailTableCTRLer.getContainer(3, 0);
    valueContainer = this.smartDetailTableCTRLer.getContainer(3, 1);
    title = 
        jLego.basicUI.addDiv(titleContainer, {id: jLego.func.getRandomString(), class: ""});
    $(title).text(this.myLanguage.SERIAL_NUMBER);
    value = 
        jLego.basicUI.addDiv(valueContainer, {id: jLego.func.getRandomString(), class: ""});
    $(value).text(Jdata.serialNumber);
    //smartSupport
    titleContainer = this.smartDetailTableCTRLer.getContainer(4, 0);
    valueContainer = this.smartDetailTableCTRLer.getContainer(4, 1);
    title = 
        jLego.basicUI.addDiv(titleContainer, {id: jLego.func.getRandomString(), class: ""});
    $(title).text(this.myLanguage.SMART_SUPPORT);
    value = 
        jLego.basicUI.addDiv(valueContainer, {id: jLego.func.getRandomString(), class: ""});
    $(value).text(Jdata.smartSupport);
    //localTime
    titleContainer = this.smartDetailTableCTRLer.getContainer(5, 0);
    valueContainer = this.smartDetailTableCTRLer.getContainer(5, 1);
    title = 
        jLego.basicUI.addDiv(titleContainer, {id: jLego.func.getRandomString(), class: ""});
    $(title).text(this.myLanguage.LOCAL_TIME);
    value = 
        jLego.basicUI.addDiv(valueContainer, {id: jLego.func.getRandomString(), class: ""});
    $(value).text(Jdata.localTime);
    //device
    titleContainer = this.smartDetailTableCTRLer.getContainer(6, 0);
    valueContainer = this.smartDetailTableCTRLer.getContainer(6, 1);
    title = 
        jLego.basicUI.addDiv(titleContainer, {id: jLego.func.getRandomString(), class: ""});
    $(title).text(this.myLanguage.DEVICE);
    value = 
        jLego.basicUI.addDiv(valueContainer, {id: jLego.func.getRandomString(), class: ""});
    $(value).text(Jdata.device);
    //ataStandard
    titleContainer = this.smartDetailTableCTRLer.getContainer(7, 0);
    valueContainer = this.smartDetailTableCTRLer.getContainer(7, 1);
    title = 
        jLego.basicUI.addDiv(titleContainer, {id: jLego.func.getRandomString(), class: ""});
    $(title).text(this.myLanguage.ATA_STANDARD);
    value = 
        jLego.basicUI.addDiv(valueContainer, {id: jLego.func.getRandomString(), class: ""});
    $(value).text(Jdata.ataStandard);
    //ataVersion
    titleContainer = this.smartDetailTableCTRLer.getContainer(8, 0);
    valueContainer = this.smartDetailTableCTRLer.getContainer(8, 1);
    title = 
        jLego.basicUI.addDiv(titleContainer, {id: jLego.func.getRandomString(), class: ""});
    $(title).text(this.myLanguage.ATA_VERSION);
    value = 
        jLego.basicUI.addDiv(valueContainer, {id: jLego.func.getRandomString(), class: ""});
    $(value).text(Jdata.ataVersion);
}
popupPage.prototype.close=function(){
    this.smartTableCTRLer = null;
}

popupPage.prototype.resize=function(){
    if(this.smartTableCTRLer!=null){ 
        var newWidth = $(this.myVariable.panelCTRLer.getContentFrame()).width() - 40;
        var newHieght = $(this.myVariable.panelCTRLer.getContentFrame()).height() - 40;
        $(this.smartTableFrame).width(newWidth);
        $(this.smartTableFrame).height(newHieght);
        this.smartTableCTRLer.resize();
    }
}  