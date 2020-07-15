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

var jLego = {};
jLego.id={};
jLego.cls={};
jLego.url={};
jLego.lang={};
jLego.variables={};
jLego.constants={};
jLego.func={};
jLego.servlet={};
jLego.ui={};
jLego.layout={};

document.write("<script type=\"text/javascript\" src=\"jLego/js/addOns/jQuery/jquery-1.10.2.js\"></script>");
document.write("<script type=\"text/javascript\" src=\"jLego/js/addOns/jQuery/jquery.min.js\"></script>");
document.write("<script type=\"text/javascript\" src=\"jLego/js/addOns/jQuery/jquery-ui.js\"></script>");
document.write("<link rel=\"stylesheet\" href=\"jLego/js/addOns/jQuery/jquery-ui.css\"/>");

document.write("<script type=\"text/javascript\" src=\"jLego/js/addOns/jQuery/jquery.foggy.min.js\"></script>");

document.write("<script type=\"text/javascript\" src=\"jLego/js/addOns/pieChart/Chart.js\"></script>");
document.write("<script type=\"text/javascript\" src=\"jLego/js/data.js\"></script>");
document.write("<script type=\"text/javascript\" src=\"jLego/js/func.js\"></script>");
        
jLego.init = function(){
    jLego.func.registerSite('basicUI');
    jLego.func.registerSite('objectUI');
}

jLego.resize = function(){
    for(var i=0; i< jLego.variables.resizableObjectList.length; i++)
        jLego.variables.resizableObjectList[i].resizeHandler();
}

jLego.setLanguage = function(language){
    switch(language){
        case 'EN':
        case 'en':
        case 'English':
        case 'en-US':
        case 'en_US':
            jLego.lang.currentLanguage="en_US";
            break;
        case 'ja':
        case 'ja_JP':
            jLego.lang.currentLanguage="ja_JP";
            break;
        case 'zh':
        case 'TW':
        case 'Taiwan':
        case 'Chinese':
        case 'Tradition_Chinese':
        case 'zh-TW':
        case 'zh_TW':
        default:
            jLego.lang.currentLanguage="zh_TW";
            break;
    }
}

jLego.getCurrentLanguage = function(){
    return jLego.lang.currentLanguage;
}



