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

var webView = {};

webView.id={};
webView.cls={};
webView.url={};
webView.lang={};
webView.variables={};
webView.constants={};
webView.func={};
webView.servlet={};
webView.ui={};
webView.layout={};

webView.constants.PAGE_DASHBOARD = 0;
webView.constants.PAGE_VOLUMES = 1;
webView.constants.PAGE_SERVICE = 2;
webView.constants.PAGE_RAID = 3;

webView.initPageIndex = webView.constants.PAGE_DASHBOARD;
webView.currentLanguage = "en_US";

webView.setLanguage=function(language){
    switch(language){
        case 'zh':
        case 'zh_TW':
            $('head').append('<script type=\"text/javascript\" src=\"language/zh_TW.js\"></script>');
            webView.currentLanguage = "zh_TW";
            break;
        case 'en':
        case 'en-US':
        case 'en_US':
            $('head').append('<script type=\"text/javascript\" src=\"language/en_US.js\"></script>');
            webView.currentLanguage = "en_US";
            break;
        default:
            $('head').append('<script type=\"text/javascript\" src=\"language/en_US.js\"></script>');
            webView.currentLanguage = "en_US";
            break;
    }
}

webView.mixin = function(){
    var args = Array.prototype.slice.call(arguments);
    for(var i=0; i<args.length; i++){
        switch(args[i]){
            case 'mainPage':
                $('head').append('<link rel=\"stylesheet\" href=\"webPage/css/mainPage.css\" type=\"text/css\"/>');
                $('head').append('<script type=\"text/javascript\" src=\"webPage/js/mainPage/languages/autoLanguage.js\"></script>');
                $('head').append('<script type=\"text/javascript\" src=\"webPage/js/mainPage/data.js\"></script>');
                $('head').append('<script type=\"text/javascript\" src=\"webPage/js/mainPage/mainPage.js\"></script>');
                webView.mainPage = new mainPage();
                break;
            case 'dashboardPage':
                $('head').append('<link rel=\"stylesheet\" href=\"webPage/css/dashboardPage.css\" type=\"text/css\"/>');
                $('head').append('<script type=\"text/javascript\" src=\"webPage/js/dashboardPage/languages/autoLanguage.js\"></script>');
                $('head').append('<script type=\"text/javascript\" src=\"webPage/js/dashboardPage/data.js\"></script>');
                $('head').append('<script type=\"text/javascript\" src=\"webPage/js/dashboardPage/dashboardPage.js\"></script>');
                webView.dashboardPage = new dashboardPage();
                break;
            case 'common':
                $('head').append('<script type=\"text/javascript\" src=\"webPage/js/common/languages/autoLanguage.js\"></script>');
                $('head').append('<script type=\"text/javascript\" src=\"webPage/js/common/data.js\"></script>');
                $('head').append('<script type=\"text/javascript\" src=\"webPage/js/common/url.js\"></script>');
                $('head').append('<script type=\"text/javascript\" src=\"webPage/js/common/connection.js\"></script>');
                webView.common = {
                    myID: webView.id.common,
                    myClass: webView.cls.common,
                    myConsts: webView.constants.common,
                    myLanguage: webView.lang.common 
                };
                webView.connection = new connection();
                break;
            default:
                break;
        }
    }
}
