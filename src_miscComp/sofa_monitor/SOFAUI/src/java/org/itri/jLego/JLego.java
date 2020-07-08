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
package org.itri.jLego;

import java.io.IOException;
import java.io.PrintWriter;
import javax.servlet.jsp.JspWriter;

/**
 *
 * @author jkl
 */
public class JLego {
    public void mixin(JspWriter out, String[] modules) throws IOException{
        boolean isImportedDateTimePicker = false;
        out.append("<script type=\"text/javascript\" src=\"jLego/js/addOns/jQuery/jquery-1.10.2.js\"></script>");
        out.append("<script type=\"text/javascript\" src=\"jLego/js/addOns/jQuery/jquery.min.js\"></script>");
        out.append("<script type=\"text/javascript\" src=\"jLego/js/addOns/jQuery/jquery-ui.js\"></script>");
        out.append("<link rel=\"stylesheet\" href=\"jLego/js/addOns/jQuery/jquery-ui.css\" type=\"text/css\"/>");
        
        out.append("<script type=\"text/javascript\" src=\"jLego/js/addOns/jQuery/jquery.foggy.min.js\"></script>");
        out.append("<script type=\"text/javascript\" src=\"jLego/js/addOns/moment/moment.js\"></script>");
        out.append("<script type=\"text/javascript\" src=\"jLego/js/addOns/pieChart/Chart.js\"></script>");
        out.append("<script type=\"text/javascript\" src=\"jLego/js/data.js\"></script>");
        out.append("<script type=\"text/javascript\" src=\"jLego/js/func.js\"></script>");
        
        out.append("<script>jLego.init();</script>");
        out.append("<script type=\"text/javascript\" src=\"jLego/js/basicUI/basicUI.js\"></script>");
        out.append("<script>jLego.variables.isInit=true;</script>");
        for(int i=0; i<modules.length; i++){
            switch(modules[i]){
                case "tabPageOrg":
                    out.append("<link rel=\"stylesheet\" href=\"jLego/css/objectUI/tabPageOrg.css\" type=\"text/css\"/>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/tabPageOrg/data.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/tabPageOrg/tabPageOrg.js\"></script>");
                    out.append("<script>"+
                    "jLego.tabPageOrg = new jLego.objectUI.tabPageOrg();"+
                    "jLego.variables.resizableObjectList[jLego.variables.resizableObjectList.length] = jLego.tabPageOrg;"+
                    "</script>");
                    break;
                case "ccmaTabPage":
                    out.append("<link rel=\"stylesheet\" href=\"jLego/css/objectUI/ccmaTabPage.css\" type=\"text/css\"/>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/ccmaTabPage/data.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/ccmaTabPage/ccmaTabPage.js\"></script>");
                    out.append("<script>"+
                    "jLego.ccmaTabPage = new jLego.objectUI.ccmaTabPage();"+
                    "jLego.variables.resizableObjectList[jLego.variables.resizableObjectList.length] = jLego.ccmaTabPage;"+
                    "</script>");
                    break;
                case "tabPage":
                    out.append("<link rel=\"stylesheet\" href=\"jLego/css/objectUI/tabPage.css\" type=\"text/css\"/>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/tabPage/data.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/tabPage/tabPage.js\"></script>");
                    /*out.append("<script>"+
                    "jLego.ccmaTabPage = new jLego.objectUI.tabPage();"+
                    "jLego.variables.resizableObjectList[jLego.variables.resizableObjectList.length] = jLego.tabPage;"+
                    "</script>");*/
                    //jLego.tabPage = new jLego.objectUI.tabPage();
                    //jLego.variables.resizableObjectList[jLego.variables.resizableObjectList.length] = jLego.tabPage;
                    break;
                case "statusMonitor":
                    out.append("<link rel=\"stylesheet\" href=\"jLego/css/objectUI/statusMonitor.css\" type=\"text/css\"/>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/statusMonitor/languages/autoLanguage.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/statusMonitor/data.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/statusMonitor/statusMonitor.js\"></script>");
                    out.append("<script>"+
                    "jLego.statusMonitor = new jLego.objectUI.statusMonitor();"+
                    "</script>");
                    break;
                case "nodeTable":
                    out.append("<link rel=\"stylesheet\" href=\"jLego/css/objectUI/nodeTable.css\" type=\"text/css\"/>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/nodeTable/data.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/nodeTable/nodeTable.js\"></script>");
                    out.append("<script>"+
                    "jLego.nodeTable = new jLego.objectUI.nodeTable();"+
                    "</script>");
                    break;
                case "toastCTRLer":
                    out.append("<link rel=\"stylesheet\" href=\"jLego/css/objectUI/toastCTRLer.css\" type=\"text/css\"/>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/toastCTRLer/data.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/toastCTRLer/toastCTRLer.js\"></script>");
                    out.append("<script>"+
                    "jLego.toastCTRLer = new jLego.objectUI.toastCTRLer();"+
                    "</script>");
                    break;
                case "stepPage":
                    out.append("<link rel=\"stylesheet\" href=\"jLego/css/objectUI/stepPage.css\" type=\"text/css\"/>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/stepPage/data.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/stepPage/stepPage.js\"></script>");
                    break;
                case "buttonCTRLer":
                    out.append("<link rel=\"stylesheet\" href=\"jLego/css/objectUI/buttonCTRLer.css\" type=\"text/css\"/>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/buttonCTRLer/data.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/buttonCTRLer/buttonCTRLer.js\"></script>");
                    break;
                case "nowLoading":
                    out.append("<link rel=\"stylesheet\" href=\"jLego/css/objectUI/nowLoading.css\" type=\"text/css\"/>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/nowLoading/languages/autoLanguage.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/nowLoading/data.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/nowLoading/nowLoading.js\"></script>");
                    break;
                case "popoutPanel":
                    out.append("<link rel=\"stylesheet\" href=\"jLego/css/objectUI/popoutPanel.css\" type=\"text/css\"/>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/popoutPanel/languages/autoLanguage.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/popoutPanel/data.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/popoutPanel/popoutPanel.js\"></script>");
                    break;
                case "optionCTRLer":
                    if(!isImportedDateTimePicker){
                        out.append("<link rel=\"stylesheet\" href=\"jLego/css/addOns/jquery-ui-timepicker-addon.css\" type=\"text/css\"/>");
                        out.append("<script type=\"text/javascript\" src=\"jLego/js/addOns/dateTimePicker/jquery-ui-timepicker-addon.js\"></script>");
                        out.append("<script type=\"text/javascript\" src=\"jLego/js/addOns/dateTimePicker/jquery-ui-sliderAccess.js\"></script>");
                        isImportedDateTimePicker = true;
                    }
                    //out.append("<link rel=\"stylesheet\" href=\"jLego/css/objectUI/custom.css\" type=\"text/css\"/>");
                    out.append("<link rel=\"stylesheet\" href=\"jLego/css/objectUI/optionCTRLer.css\" type=\"text/css\"/>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/optionCTRLer/data.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/optionCTRLer/optionCTRLer.js\"></script>");
                    break;
                case "optionListCTRLer":
                    out.append("<link rel=\"stylesheet\" href=\"jLego/css/objectUI/optionListCTRLer.css\" type=\"text/css\"/>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/optionListCTRLer/data.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/optionListCTRLer/languages/autoLanguage.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/optionListCTRLer/optionListCTRLer.js\"></script>");
                    break;
                case "searchCTRLer":
                    out.append("<link rel=\"stylesheet\" href=\"jLego/css/objectUI/searchCTRLer.css\" type=\"text/css\"/>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/searchCTRLer/data.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/searchCTRLer/searchCTRLer.js\"></script>");
                    break;
                case "levelPanel":
                    out.append("<link rel=\"stylesheet\" href=\"jLego/css/objectUI/levelPanel.css\" type=\"text/css\"/>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/levelPanel/data.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/levelPanel/levelPanel.js\"></script>");
                    break;
                case "nodeCard":
                    out.append("<link rel=\"stylesheet\" href=\"jLego/css/objectUI/nodeCard.css\" type=\"text/css\"/>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/nodeCard/data.js\"></script>");
                    out.append("<script type=\"text/javascript\" src=\"jLego/js/objectUI/nodeCard/nodeCard.js\"></script>");
                    break;
                default:
                    break;
            }
        }
    }
}