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

jLego.func.registerSite=function(name) {
    jLego[name] = {};
    jLego[name].id = {};
    jLego[name].cls = {};
    jLego[name].url = {};
    jLego[name].lang = {};
    jLego[name].variables = {};
    jLego[name].constants = {};
    jLego[name].func = {};
    jLego[name].servlet = {};
    jLego[name].ui = {};
    jLego[name].layout = {};
    return jLego[name];
}

jLego.func.getRandomString=function(length){
    var random_text = "";
    if(length==null)    length=jLego.constants.randomIDSize;
    var possible = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    for( var i=0; i < length; i++ ) random_text += possible.charAt(Math.floor(Math.random() * possible.length));
    return random_text;
}

jLego.func.getRandomIntBetween=function(min, max){
    return Math.floor(Math.random() * (max-min)) + min;
}

jLego.func.getRandomFloatBetween=function(min, max){
    return Math.floor((((Math.random() * (max-min)) + min)*100))/100;
}

jLego.func.getRootPath=function(){
    return jLego.url.rootUrl;
}
jLego.func.getBasePath=function(){
    return jLego.url.baseUrl;
}
jLego.func.getImgPath=function(option){
    if(typeof setNonServer == 'undefined'){
        var path = jLego.url.baseImgUrl;
    }
    else{
        if(setNonServer==true)  var path='../web/jLego/img';
        else var path = jLego.url.baseImgUrl;
    }
    var imgType;
    if(option==null)  return false;
    if(option.category==null)   return false;
    else{
        if(option.type==null)   imgType=jLego.url.baseImgExtensionPNG;
        else{
            if(option.type.toString().toLowerCase()==='jpg') imgType=jLego.url.baseImgExtensionJPG;
            else if(option.type.toString().toLowerCase()==='png') imgType=jLego.url.baseImgExtensionPNG;
            else if(option.type.toString().toLowerCase()==='gif') imgType=jLego.url.baseImgExtensionGIF;
        }
        switch(option.category){
            case 'buttonIcon':
                path+="/buttonIcon/"+option.name;
                path+=imgType;
                break;
            case 'statusLarge':
                path+="/statusLarge/"+option.name;
                path+=imgType;
                break;
            case 'statusSmall':
                path+="/statusSmall/"+option.name;
                path+=imgType;
                break;
            case 'nowLoading':
                path+="/nowLoading/"+option.name;
                path+=imgType;
                break;
            case 'popoutPanel':
                path+="/popoutPanel/"+option.name;
                path+=imgType;
                break;
            case 'optionCTRLer':
                path+="/optionCTRLer/"+option.name;
                path+=imgType;
                break;
            default:
                path+="/"+option.category+"/"+option.name;
                path+=imgType;
                break;
        }
    }
    return path;
}

jLego.func.isMobile=function(){
    if( /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent) ) {
        return true;
    }
    else return false;
}

jLego.func.lookup=function(obj, key) {
    var type = typeof key;
    if (type == 'string' || type == "number") key = ("" + key).replace(/\[(.*?)\]/, function(m, key){//handle case where [1] may occur
        return '.' + key;
    }).split('.');
    for (var i = 0, l = key.length, currentkey; i < l; i++) {
        if (obj.hasOwnProperty(key[i])) obj = obj[key[i]];
        else return undefined;
    }
    return obj;
}

jLego.func.transDateTime=function(datetime){
    var split_datetime=datetime.split(" ");
    var date=split_datetime[0].split("-");
    if(date.length<3) date=split_datetime[0].split("/");
    var time=split_datetime[1].split(":");
    var result_date = new Date();
    result_date.setYear(parseInt(date[0]));
    result_date.setMonth(parseInt(date[1])-1);
    result_date.setDate(parseInt(date[2]));
    result_date.setHours(parseInt(time[0]));
    result_date.setMinutes(parseInt(time[1]));
    result_date.setSeconds(parseInt(time[2]));
    return result_date;
}

jLego.func.transTimestampToTime=function(timestamp){
    var date = new Date(timestamp*1000);
    var hours = date.getHours();
    // minutes part from the timestamp
    var minutes = "0" + date.getMinutes();
    // seconds part from the timestamp
    var seconds = "0" + date.getSeconds();
    // will display time in 10:30:23 format
    var formattedTime = hours + ':' + minutes.substr(-2) + ':' + seconds.substr(-2);
    return formattedTime;
}
jLego.func.transDateToFormatDate=function(myDate){
    var year = myDate.getYear()+1900;
    var month = myDate.getMonth()+1;
    var day = myDate.getDate();
    var hours = myDate.getHours();
    // minutes part from the timestamp
    var minutes = "0" + myDate.getMinutes();
    // seconds part from the timestamp
    var seconds = "0" + myDate.getSeconds();
    // will display time in 10:30:23 format
    var formattedTime = year+ "/" + month+"/" + day + " "+hours + ':' + minutes.substr(-2) + ':' + seconds.substr(-2);
    return formattedTime;
}
jLego.func.formatFloat=function(num, pos){
    var size = Math.pow(10, pos);
    return Math.round(num * size) / size;
}

jLego.func.moveArrayPosition = function (arrayObject, oldIndex, newIndex) {
    while (oldIndex < 0) {
        oldIndex += arrayObject.length;
    }
    while (newIndex < 0) {
        newIndex += arrayObject.length;
    }
    if (newIndex >= arrayObject.length) {
        var k = newIndex - arrayObject.length;
        while ((k--) + 1) {
            arrayObject.push(undefined);
        }
    }
    arrayObject.splice(newIndex, 0, arrayObject.splice(oldIndex, 1)[0]);
    return arrayObject;
};

jLego.func.isInIframe = function () {
    try {
        return window.self !== window.top;
    } catch (e) {
        return true;
    }
}

jLego.func.getBrowser= function(){
    var ua= navigator.userAgent, tem, 
    M= ua.match(/(opera|chrome|safari|firefox|msie|trident(?=\/))\/?\s*(\d+)/i) || [];
    if(/trident/i.test(M[1])){
        tem=  /\brv[ :]+(\d+)/g.exec(ua) || [];
        return 'IE '+(tem[1] || '');
    }
    if(M[1]=== 'Chrome'){
        tem= ua.match(/\bOPR\/(\d+)/);
        if(tem!= null) return 'Opera '+tem[1];
    }
    M= M[2]? [M[1], M[2]]: [navigator.appName, navigator.appVersion, '-?'];
    if((tem= ua.match(/version\/(\d+)/i))!= null) M.splice(1, 1, tem[1]);
    return M.join(' ');
};

jLego.func.browserIs=function(browser){
    if(jLego.func.getBrowser().toLowerCase().indexOf(browser.toString().toLowerCase()) > -1){
        return true;
    }
    else return false
}

jLego.func.getSpaceInByte=function(spaceValue, unit){
    var Th_KB = 1024;
    var Th_MB = 1048576;
    var Th_GB = 1073741824;
    var Th_TB = 1099511627776;
    if(unit=='B' || unit=='b'){
        return parseFloat(spaceValue);
    }
    else if(unit=='KB' || unit=='kb'){
        var result = parseFloat(spaceValue) * Th_KB;
        return result;
    }
    else if(unit=='MB' || unit=='mb'){
        var result = parseFloat(spaceValue) * Th_MB;
        return result;
    }
    else if(unit=='GB' || unit=='gb'){
        var result = parseFloat(spaceValue) * Th_GB;
        return result;
    }
    else if(unit=='TB' || unit=='tb'){
        var result = parseFloat(spaceValue) * Th_TB;
        return result;
    }
    else return parseFloat(spaceValue);
}

jLego.func.getSpaceFromByteToBestUnit=function(spaceValue){
    var Th_KB = 1024;
    var Th_MB = 1048576;
    var Th_GB = 1073741824;
    var Th_TB = 1099511627776;
    var Th_PB = 1125899906842624;
    var space = spaceValue;
    var unit;
    if(spaceValue<Th_KB) return space;
    else if(spaceValue>=Th_KB && spaceValue<Th_MB){
        unit = 'KB';
        space = parseInt((space/Th_KB)*100)/100;
        return space;
    }
    else if(spaceValue>=Th_MB && spaceValue<Th_GB){
        unit = 'MB';
        space = parseInt((space/Th_MB)*100)/100;
        return space;
    }
    else if(spaceValue>=Th_GB && spaceValue<Th_TB){
        unit = 'GB';
        space = parseInt((space/Th_GB)*100)/100;
        return space;
    }
    else if(spaceValue>=Th_TB && spaceValue<Th_PB){
        unit = 'TB';
        space = parseInt((space/Th_TB)*100)/100;
        return space;
    }
    else if(spaceValue>=Th_PB){
        unit = 'PB';
        space = parseInt((space/Th_PB)*100)/100;
        return space;
    }
    else return -1;
}

jLego.func.getBestUnit=function(spaceValue){
    var Th_KB = 1024;
    var Th_MB = 1048576;
    var Th_GB = 1073741824;
    var Th_TB = 1099511627776;
    var Th_PB = 1125899906842624;
    var unit = 'B';
    var space = spaceValue;
    if(spaceValue<Th_KB) return unit;
    else if(spaceValue>=Th_KB && spaceValue<Th_MB){
        unit = 'KB';
        return unit;
    }
    else if(spaceValue>=Th_MB && spaceValue<Th_GB){
        unit = 'MB';
        return unit;
    }
    else if(spaceValue>=Th_GB && spaceValue<Th_TB){
        unit = 'GB';
        return unit;
    }
    else if(spaceValue>=Th_TB && spaceValue<Th_PB){
        unit = 'TB';
        return unit;
    }
    else if(spaceValue>=Th_PB){
        unit = 'PB';
        return unit;
    }
    else return 'B';
}

jLego.func.getSpaceFromByteToTargetUnit=function(spaceValue, targetUnit){
    var space;
    var Th_KB = 1024;
    var Th_MB = 1048576;
    var Th_GB = 1073741824;
    var Th_TB = 1099511627776;
    var Th_PB = 1125899906842624;
    if(targetUnit=='B') return spaceValue;
    else if(targetUnit=='KB'){
        space = parseInt((spaceValue/Th_KB)*100)/100;
        return space;
    }
    else if(targetUnit=='MB'){
        space = parseInt((spaceValue/Th_MB)*100)/100;
        return space;
    }
    else if(targetUnit=='GB'){
        space = parseInt((spaceValue/Th_GB)*100)/100;
        return space;
    }
    else if(targetUnit=='TB'){
        space = parseInt((spaceValue/Th_TB)*100)/100;
        return space;
    }
    else if(targetUnit=='PB'){
        space = parseInt((spaceValue/Th_PB)*100)/100;
        return space;
    }
    else return spaceValue;
}

jLego.func.createCookie=function(name, value, days){
    var expires = "";
    var targetDay = 365;
    if (days) {
        targetDay = days;
    }
    var date = new Date();
    date.setTime(date.getTime() + (targetDay*24*60*60*1000));
    expires = "; expires=" + date.toUTCString();
    document.cookie = name + "=" + value + expires + "; path=/";
}

jLego.func.readCookie=function(name){
    var nameEQ = name + "=";
    var ca = document.cookie.split(';');
    for(var i=0;i < ca.length;i++) {
        var c = ca[i];
        while (c.charAt(0)==' ') c = c.substring(1,c.length);
        if (c.indexOf(nameEQ) == 0) return c.substring(nameEQ.length,c.length);
    }
    return null;
}

jLego.func.eraseCookie=function(name){
    this.createCookie(name,"",-1);
}