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
package org.itri.webPage;

import java.io.IOException;
import javax.servlet.jsp.JspWriter;

/**
 *
 * @author A40385
 */
public class WebPage {
    public void mixin(JspWriter out, String[] modules) throws IOException{
        for(int i=0; i<modules.length; i++){
            switch(modules[i]){
                case "mainPage":
                    out.append("<link rel=\"stylesheet\" href=\"webPage/css/mainPage.css\" type=\"text/css\"/>");
                    out.append("<script type=\"text/javascript\" src=\"webPage/js/mainPage/languages/autoLanguage.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"webPage/js/mainPage/data.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"webPage/js/mainPage/mainPage.js\"></script>");
                    out.append("<script>"+
                    "webView.mainPage = new mainPage();"+
                    "</script>");
                    break;
                case "dashboardPage":
                    out.append("<link rel=\"stylesheet\" href=\"webPage/css/dashboardPage.css\" type=\"text/css\"/>");
                    out.append("<script type=\"text/javascript\" src=\"webPage/js/dashboardPage/languages/autoLanguage.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"webPage/js/dashboardPage/data.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"webPage/js/dashboardPage/dashboardPage.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"webPage/js/dashboardPage/drawStatusPanel.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"webPage/js/dashboardPage/drawGroupPanel.js\"></script>");
                    out.append("<script>"+
                    "webView.dashboardPage = new dashboardPage();"+
                    "</script>");
                    break;
                case "performancePage":
                    out.append("<link rel=\"stylesheet\" href=\"webPage/css/performancePage.css\" type=\"text/css\"/>");
                    out.append("<script type=\"text/javascript\" src=\"webPage/js/performancePage/languages/autoLanguage.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"webPage/js/performancePage/data.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"webPage/js/performancePage/performancePage.js\"></script>");
                    out.append("<script>"+
                    "webView.performancePage = new performancePage();"+
                    "</script>");
                    break;
                case "settingPage":
                    out.append("<link rel=\"stylesheet\" href=\"webPage/css/settingPage.css\" type=\"text/css\"/>");
                    out.append("<script type=\"text/javascript\" src=\"webPage/js/settingPage/languages/autoLanguage.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"webPage/js/settingPage/data.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"webPage/js/settingPage/settingPage.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"webPage/js/settingPage/doSetting.js\"></script>");
                    out.append("<script>"+
                    "webView.settingPage = new settingPage();"+
                    "</script>");
                    break;
                case "popupPage":
                    out.append("<link rel=\"stylesheet\" href=\"webPage/css/popupPage.css\" type=\"text/css\"/>");
                    out.append("<script type=\"text/javascript\" src=\"webPage/js/popupPage/languages/autoLanguage.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"webPage/js/popupPage/data.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"webPage/js/popupPage/popupPage.js\"></script>");
                    out.append("<script>"+
                    "webView.popupPage = new popupPage();"+
                    "</script>");
                    break;
                case "common":
                    out.append("<script type=\"text/javascript\" src=\"webPage/js/common/languages/autoLanguage.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"webPage/js/common/data.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"webPage/js/common/url.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"webPage/js/common/connection.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"webPage/js/common/utils.js\"></script>");
                    out.append("<script>"+
                        "webView.common = {" +
                        "myID: webView.id.common," +
                        "myClass: webView.cls.common," +
                        "myConsts: webView.constants.common," +
                        "myLanguage: webView.lang.common " +
                        "};" +
                        "webView.connection = new connection();"+
                        "webView.utils = new utils();"+
                    "</script>");
                    break;
                default:
                    break;
            }
        }
    }
}
