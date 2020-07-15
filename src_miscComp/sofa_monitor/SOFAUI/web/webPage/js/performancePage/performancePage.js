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

performancePage = function(){
    var myID;
    var myClass;
    var myConsts;
    var myLanguage;
    var parentElement;
    var mainElement; 
    var perfTabFrame;
    
    //IOPS Part
    var iopsFrame;
    var currentIOPSChartCanvas;
    var iopsStartStopButton;
    var lastIOPSPointTimestamp;
    
    var writeFrame;
    var currentWriteChartCanvas;
    var writeStartStopButton;
    
    var readFrame;
    var readStartStopButton;
    
    var buttonCTRLer;
    
    var optionListCTRLer;
    var perfTabCTRLer;
    
    var queryTimer;
    var totalChartPoint = 0;
    
    var drawnIOPSPerformance = false;
    var drawnWritePerformance = false;
    var drawnReadPerformance = false;
    this.initialize();
}

performancePage.prototype.initialize=function(){
    this.myID=webView.id.performancePage;
    this.myClass=webView.cls.performancePage;
    this.myConsts=webView.constants.performancePage;
    this.myLanguage=webView.lang.performancePage;
    
    this.buttonCTRLer = new jLego.objectUI.buttonCTRLer();
    this.optionListCTRLer = new jLego.objectUI.optionListCTRLer();
    
    this.lastIOPSPointTimestamp = null;
}

performancePage.prototype.add=function(target, option){
    this.parentElement = target;
    this.mainElement = 
           jLego.basicUI.addDiv(target, {id: jLego.func.getRandomString(), class: this.myClass.MAIN_FRAME}); 
    var titleFrame = 
           jLego.basicUI.addDiv(this.mainElement, {id: jLego.func.getRandomString(), class: this.myClass.MAIN_TITLE_FRAME}); 
    $(titleFrame).text(this.myLanguage.PERFORMANCE_TITLE);
    webView.constants.performancePage.tempTarget = this.mainElement;
    this.perfTabFrame = 
            jLego.basicUI.addDiv(this.mainElement, {id: jLego.func.getRandomString(), class: this.myClass.MAIN_FRAME});
    $(this.perfTabFrame).data('titleFrame', titleFrame);
    $(this.perfTabFrame).height($(this.mainElement).height() - $($(this.perfTabFrame).data('titleFrame')).height() - parseInt($($(this.perfTabFrame).data('titleFrame')).css('margin-top')));
    $(this.perfTabFrame).css('overflow-x', 'hidden');
    this.perfTabCTRLer = new jLego.objectUI.tabPage();
    this.perfTabCTRLer.add(this.perfTabFrame, {mode: 'no-border'});
    var callbackObject ={
        arg:{
            parent: this,
        },
        callback: function(arg){
            var parent = arg.parent;
            if(!parent.drawnIOPSPerformance) parent.addIOPSPanel(parent.perfTabCTRLer.getTabPage(0));
        }
    }
    this.perfTabCTRLer.addTabElement(this.myLanguage.IOPS_TAB, callbackObject);
/*    var callbackObject ={
        arg:{
            parent: this,
        },
        callback: function(arg){
            var parent = arg.parent;
            if(!parent.drawnWritePerformance) parent.addWritePerfPanel(parent.perfTabCTRLer.getTabPage(1));

        }
    }
    this.perfTabCTRLer.addTabElement(this.myLanguage.WRITE_TAB, callbackObject);
    var callbackObject ={
        arg:{
            parent: this,
        },
        callback: function(arg){
            var parent = arg.parent;
            if(!parent.drawnReadPerformance) parent.addReadPerfPanel(parent.perfTabCTRLer.getTabPage(2));
        }
    }
    this.perfTabCTRLer.addTabElement(this.myLanguage.READ_TAB, callbackObject);
*/    
    $(this.perfTabCTRLer.getTab(0)).click();
    webView.performancePage.resize();
}

performancePage.prototype.addIOPSPanel = function(target, Jdata){
    this.drawnIOPSPerformance = true;
    this.iopsFrame = 
            jLego.basicUI.addDiv(target, {id: jLego.func.getRandomString(), class: this.myClass.IOPS_OUTTER_FRAME});
    //title
    var titleFrame = 
            jLego.basicUI.addDiv(this.iopsFrame, {id: jLego.func.getRandomString(), class: this.myClass.TITTLE_FRAME});
    $(titleFrame).text(this.myLanguage.IOPS_TITLE);
    //menu
    var menuFrame = 
            jLego.basicUI.addDiv(this.iopsFrame, {id: jLego.func.getRandomString(), class: this.myClass.MENU_FRAME});
    this.iopsStartStopButton = jLego.basicUI.addImg(menuFrame, 
                                 {id: jLego.func.getRandomString(), 
                                  class: this.myClass.STARTSTOP_ICON, 
                                  src: jLego.func.getImgPath({category: 'statusSmall', name: 'pause', type: 'png'})});
    $(this.iopsStartStopButton).data('status', 'start');
    $(this.iopsStartStopButton).data('parent', this);
    $(this.iopsStartStopButton).click(function(){
        var parent = $(this).data('parent');
        if($(this).data('status')=='start'){
            $(this).data('status', 'stop');
            $(this).attr('src', jLego.func.getImgPath({category: 'statusSmall', name: 'start', type: 'png'}));
            if(parent.writeStartStopButton){
                $(parent.writeStartStopButton).data('status', 'stop');
                $(parent.writeStartStopButton).attr('src', jLego.func.getImgPath({category: 'statusSmall', name: 'start', type: 'png'}));
            }
            if(parent.readStartStopButton){
                $(parent.readStartStopButton).data('status', 'stop');
                $(parent.readStartStopButton).attr('src', jLego.func.getImgPath({category: 'statusSmall', name: 'start', type: 'png'}));
            }
            parent.stopUpdateLineChart();
        }
        else{
            $(this).data('status', 'start');
            $(this).attr('src', jLego.func.getImgPath({category: 'statusSmall', name: 'pause', type: 'png'}));
            if(parent.writeStartStopButton){
                $(parent.writeStartStopButton).data('status', 'start');
                $(parent.writeStartStopButton).attr('src', jLego.func.getImgPath({category: 'statusSmall', name: 'pause', type: 'png'}));
            }
            if(parent.readStartStopButton){
                $(parent.readStartStopButton).data('status', 'start');
                $(parent.readStartStopButton).attr('src', jLego.func.getImgPath({category: 'statusSmall', name: 'pause', type: 'png'}));
            }
            parent.startUpdateLineChart();
        }
    });
    //chart
    var marginTop = 0;
    var chartWidth = 700;
    var chartHeight = $(this.iopsFrame).height() - $(titleFrame).height() - marginTop;
    
    this.myConsts.tempMarginTop = marginTop;
    this.myConsts.tempChartWidth = chartWidth;
    this.myConsts.tempChartHeight = chartHeight;
    webView.connection.getIOPSTrend(function(Jdata){
        var parent = webView.performancePage;
        if(Jdata.errCode==0){
            //prepare data
            var dataList = []; var dataLabel = []; var xLabel = []; var value = []; var threshold = [];
            for(var i=0; i<Jdata.data.length; i++){
                var currentData = Jdata.data[i];
                
                xLabel[i] = moment.unix(currentData.timestamp).format("HH:mm:ss");
                value[i] = currentData.iops;
                threshold[i] = webView.variables.iopsThreshold
                webView.performancePage.lastIOPSPointTimestamp = currentData.timestamp;
            }
            parent.totalChartPoint = Jdata.data.length;
            //iops data
            dataList[0] = {
                label: "IOPS",
                data: value,
                lineConfig: {fill: true, hasPoint: true}
            };
            //threshold data
            /*dataList[1] = {
                label: "Threshold",
                data: threshold,
                lineConfig: {fill: false, hasPoint: false}
            }*/
            
            var dataCollection = {
                xLabel: xLabel,
                dataList: dataList
            }
            
            var chartData = parent.prepareLineChartData(dataCollection);
            var chartCanvas = parent.addLineChart(parent.iopsFrame, parent.myConsts.tempChartWidth, parent.myConsts.tempChartHeight, parent.myConsts.tempMarginTop, chartData);
            webView.performancePage.currentIOPSChartCanvas = chartCanvas;
            parent.startUpdateLineChart();
        }
    });
}

performancePage.prototype.addWritePerfPanel = function(target, Jdata){
    this.drawnWritePerformance = true;
    this.writeFrame = 
            jLego.basicUI.addDiv(target, {id: jLego.func.getRandomString(), class: this.myClass.IOPS_OUTTER_FRAME});
    //title
    var titleFrame = 
            jLego.basicUI.addDiv(this.writeFrame, {id: jLego.func.getRandomString(), class: this.myClass.TITTLE_FRAME});
    $(titleFrame).text(this.myLanguage.WRITE_TITLE);
    //menu
    var menuFrame = 
            jLego.basicUI.addDiv(this.writeFrame, {id: jLego.func.getRandomString(), class: this.myClass.MENU_FRAME});
    this.writeStartStopButton = jLego.basicUI.addImg(menuFrame, 
                                 {id: jLego.func.getRandomString(), 
                                  class: this.myClass.STARTSTOP_ICON, 
                                  src: jLego.func.getImgPath({category: 'statusSmall', name: 'pause', type: 'png'})});
    $(this.writeStartStopButton).data('status', 'start');
    $(this.writeStartStopButton).data('parent', this);
    $(this.writeStartStopButton).click(function(){
        var parent = $(this).data('parent');
        if($(this).data('status')=='start'){
            $(this).data('status', 'stop');
            $(this).attr('src', jLego.func.getImgPath({category: 'statusSmall', name: 'start', type: 'png'}));
            if(parent.iopsStartStopButton){
                $(parent.iopsStartStopButton).data('status', 'stop');
                $(parent.iopsStartStopButton).attr('src', jLego.func.getImgPath({category: 'statusSmall', name: 'start', type: 'png'}));
            }
            if(parent.readStartStopButton){
                $(parent.readStartStopButton).data('status', 'stop');
                $(parent.readStartStopButton).attr('src', jLego.func.getImgPath({category: 'statusSmall', name: 'start', type: 'png'}));
            }
            parent.stopUpdateLineChart();
        }
        else{
            $(this).data('status', 'start');
            $(this).attr('src', jLego.func.getImgPath({category: 'statusSmall', name: 'pause', type: 'png'}));
            if(parent.iopsStartStopButton){
                $(parent.iopsStartStopButton).data('status', 'start');
                $(parent.iopsStartStopButton).attr('src', jLego.func.getImgPath({category: 'statusSmall', name: 'pause', type: 'png'}));
            }
            if(parent.readStartStopButton){
                $(parent.readStartStopButton).data('status', 'start');
                $(parent.readStartStopButton).attr('src', jLego.func.getImgPath({category: 'statusSmall', name: 'pause', type: 'png'}));
            }
            parent.startUpdateLineChart();
        }
    });
    //chart
    var marginTop = 0;
    var chartWidth = 700;
    var chartHeight = $(this.writeFrame).height() - $(titleFrame).height() - marginTop;
    
    this.myConsts.tempMarginTop = marginTop;
    this.myConsts.tempChartWidth = chartWidth;
    this.myConsts.tempChartHeight = chartHeight;
    webView.connection.getIOPSTrend(function(Jdata){
        var parent = webView.performancePage;
        if(Jdata.errCode==0){
            //prepare data
            var dataList = []; var dataLabel = []; var xLabel = []; var value = []; var threshold = [];
            for(var i=0; i<Jdata.data.length; i++){
                var currentData = Jdata.data[i];
                
                xLabel[i] = moment.unix(currentData.timestamp).format("HH:mm:ss");
                value[i] = currentData.iops;
                threshold[i] = webView.variables.iopsThreshold
                webView.performancePage.lastIOPSPointTimestamp = currentData.timestamp;
            }
            parent.totalChartPoint = Jdata.data.length;
            //iops data
            dataList[0] = {
                label: "Write",
                data: value,
                lineConfig: {fill: true, hasPoint: true}
            };
            //threshold data
            /*dataList[1] = {
                label: "Threshold",
                data: threshold,
                lineConfig: {fill: false, hasPoint: false}
            }*/
            
            var dataCollection = {
                xLabel: xLabel,
                dataList: dataList
            }
            
            var chartData = parent.prepareLineChartData(dataCollection);
            var chartCanvas = parent.addLineChart(parent.writeFrame, parent.myConsts.tempChartWidth, parent.myConsts.tempChartHeight, parent.myConsts.tempMarginTop, chartData);
            webView.performancePage.currentWriteChartCanvas = chartCanvas;
            parent.startUpdateLineChart();
        }
    });
}

performancePage.prototype.addReadPerfPanel = function(target, Jdata){
    this.drawnReadPerformance = true;
    this.readFrame = 
            jLego.basicUI.addDiv(target, {id: jLego.func.getRandomString(), class: this.myClass.IOPS_OUTTER_FRAME});
    //title
    var titleFrame = 
            jLego.basicUI.addDiv(this.readFrame, {id: jLego.func.getRandomString(), class: this.myClass.TITTLE_FRAME});
    $(titleFrame).text(this.myLanguage.READ_TITLE);
    //menu
    var menuFrame = 
            jLego.basicUI.addDiv(this.readFrame, {id: jLego.func.getRandomString(), class: this.myClass.MENU_FRAME});
    this.readStartStopButton = jLego.basicUI.addImg(menuFrame, 
                                 {id: jLego.func.getRandomString(), 
                                  class: this.myClass.STARTSTOP_ICON, 
                                  src: jLego.func.getImgPath({category: 'statusSmall', name: 'pause', type: 'png'})});
    $(this.readStartStopButton).data('status', 'start');
    $(this.readStartStopButton).data('parent', this);
    $(this.readStartStopButton).click(function(){
        var parent = $(this).data('parent');
        if($(this).data('status')=='start'){
            $(this).data('status', 'stop');
            $(this).attr('src', jLego.func.getImgPath({category: 'statusSmall', name: 'start', type: 'png'}));
            if(parent.iopsStartStopButton){
                $(parent.iopsStartStopButton).data('status', 'stop');
                $(parent.iopsStartStopButton).attr('src', jLego.func.getImgPath({category: 'statusSmall', name: 'start', type: 'png'}));
            }
            if(parent.writeStartStopButton){
                $(parent.writeStartStopButton).data('status', 'stop');
                $(parent.writeStartStopButton).attr('src', jLego.func.getImgPath({category: 'statusSmall', name: 'start', type: 'png'}));
            }
            parent.stopUpdateLineChart();
        }
        else{
            $(this).data('status', 'start');
            $(this).attr('src', jLego.func.getImgPath({category: 'statusSmall', name: 'pause', type: 'png'}));
            if(parent.writeStartStopButton){
                $(parent.writeStartStopButton).data('status', 'start');
                $(parent.writeStartStopButton).attr('src', jLego.func.getImgPath({category: 'statusSmall', name: 'pause', type: 'png'}));
            }
            if(parent.iopsStartStopButton){
                $(parent.iopsStartStopButton).data('status', 'start');
                $(parent.iopsStartStopButton).attr('src', jLego.func.getImgPath({category: 'statusSmall', name: 'pause', type: 'png'}));
            }
            parent.startUpdateLineChart();
        }
    });
    //chart
    var marginTop = 0;
    var chartWidth = 700;
    var chartHeight = $(this.readFrame).height() - $(titleFrame).height() - marginTop;
    
    this.myConsts.tempMarginTop = marginTop;
    this.myConsts.tempChartWidth = chartWidth;
    this.myConsts.tempChartHeight = chartHeight;
    webView.connection.getIOPSTrend(function(Jdata){
        var parent = webView.performancePage;
        if(Jdata.errCode==0){
            //prepare data
            var dataList = []; var dataLabel = []; var xLabel = []; var value = []; var threshold = [];
            for(var i=0; i<Jdata.data.length; i++){
                var currentData = Jdata.data[i];
                
                xLabel[i] = moment.unix(currentData.timestamp).format("HH:mm:ss");
                value[i] = currentData.iops;
                threshold[i] = webView.variables.iopsThreshold
                webView.performancePage.lastIOPSPointTimestamp = currentData.timestamp;
            }
            parent.totalChartPoint = Jdata.data.length;
            //iops data
            dataList[0] = {
                label: "Read",
                data: value,
                lineConfig: {fill: true, hasPoint: true}
            };
            //threshold data
            /*dataList[1] = {
                label: "Threshold",
                data: threshold,
                lineConfig: {fill: false, hasPoint: false}
            }*/
            
            var dataCollection = {
                xLabel: xLabel,
                dataList: dataList
            }
            
            var chartData = parent.prepareLineChartData(dataCollection);
            var chartCanvas = parent.addLineChart(parent.readFrame, parent.myConsts.tempChartWidth, parent.myConsts.tempChartHeight, parent.myConsts.tempMarginTop, chartData);
            webView.performancePage.currentReadChartCanvas = chartCanvas;
            parent.startUpdateLineChart();
        }
    });
}

performancePage.prototype.resumeLineChart=function(){
    if(this.iopsStartStopButton){
        if($(this.iopsStartStopButton).data('status') == "start"){
            this.startUpdateLineChart();
        }
    }
    else if(this.writeStartStopButton){
        if($(this.writeStartStopButton).data('status') == "start"){
            this.startUpdateLineChart();
        }
    }
    else if(this.readStartStopButton){
        if($(this.readStartStopButton).data('status') == "start"){
            this.startUpdateLineChart();
        }
    }
}
performancePage.prototype.startUpdateLineChart = function(){
    if(this.queryTimer == null){
        this.queryTimer = setInterval(function(){
            webView.connection.getNewIOPSTrend(function(Jdata){
                if(Jdata.errCode==0){
                    var newXLabel = "";
                    //iops
                    var iopsNewData = [];
                    var writeNewData = [];
                    var readNewData = [];
                    for(var i=0; i<Jdata.data.length; i++){
                        var currentData = Jdata.data[i];
                        if(webView.performancePage.lastIOPSPointTimestamp==null || webView.performancePage.lastIOPSPointTimestamp!=currentData.timestamp){
                            webView.performancePage.lastIOPSPointTimestamp = currentData.timestamp;
                            newXLabel = moment.unix(currentData.timestamp).format("HH:mm:ss");
                            iopsNewData[0] = currentData.iops;
                            //iopsNewData[1] = webView.variables.iopsThreshold;
                            writeNewData[0] = currentData.iops;
                            readNewData[0] = currentData.iops;
                            if(webView.performancePage.totalChartPoint >= 60){
                                if(webView.performancePage.currentIOPSChartCanvas) webView.performancePage.currentIOPSChartCanvas.removeData();
                                if(webView.performancePage.currentWriteChartCanvas) webView.performancePage.currentWriteChartCanvas.removeData();
                                if(webView.performancePage.currentReadChartCanvas) webView.performancePage.currentReadChartCanvas.removeData();
                                
                                
                            }
                            else{
                                webView.performancePage.totalChartPoint++;
                            }
                            if(webView.performancePage.currentIOPSChartCanvas) webView.performancePage.currentIOPSChartCanvas.addData(iopsNewData, newXLabel);
                            if(webView.performancePage.currentWriteChartCanvas) webView.performancePage.currentWriteChartCanvas.addData(writeNewData, newXLabel);
                            if(webView.performancePage.currentReadChartCanvas) webView.performancePage.currentReadChartCanvas.addData(readNewData, newXLabel);
                        }

                    }
                } 
            });
        }, 1000);
    }
}

performancePage.prototype.stopUpdateLineChart = function(){
    if(this.queryTimer!=null){
        clearInterval(this.queryTimer);
        this.queryTimer=null;
    }
}


performancePage.prototype.addLineChart= function(target, chartWidth, chartHeight, marginTop, chartData){
    //outter frame
    var chartElement = 
            jLego.basicUI.addCanvas(target);
    $(chartElement).width(chartWidth);
    $(chartElement).height(chartHeight);
    $(chartElement).css('position', 'relative');
    $(chartElement).css('display', 'table');
    $(chartElement).css('top', marginTop + 'px');
    $(chartElement).css('margin', '0 auto');
    var ctx=chartElement.getContext("2d");
    var myChart = new Chart(ctx).Line(chartData, {
        showTooltips: true
    });
    return myChart;
    //this.currentChartCanvas = myChart;
    /*if(drawLegend!=false)
        $(this.legendFrame).html(myBarChart.generateLegend());

    this.currentChartCanvas = myBarChart;*/
}

performancePage.prototype.prepareLineChartData=function(dataCollection){
    var colorSet = [
        {
            fillColor: "rgba(180,180,180,0.2)", 
            strokeColor: "rgba(180,180,180,1)",
            pointColor: "rgba(180,180,180,1)",
            pointStrokeColor: "#fff",
            pointHighlightFill: "#fff",
            pointHighlightStroke: "#ccc"
        },
        {
            fillColor: "rgba(220,150,150,0.2)", 
            strokeColor: "rgba(220,150,150,1)",
            pointColor: "rgba(220,150,150,1)",
            pointStrokeColor: "#fff",
            pointHighlightFill: "#fff",
            pointHighlightStroke: "#ccc"
        }, 
         {
            fillColor: "rgba(21,174,174,0.2)", 
            strokeColor: "rgba(21,174,174,1)",
            pointColor: "rgba(21,174,174,1)",
            pointStrokeColor: "#fff",
            pointHighlightFill: "#fff",
            pointHighlightStroke: "#ccc"
        }, 
         {
            fillColor: "rgba(132,176,57,0.2)", 
            strokeColor: "rgba(132,176,57,1)",
            pointColor: "rgba(132,176,57,1)",
            pointStrokeColor: "#fff",
            pointHighlightFill: "#fff",
            pointHighlightStroke: "#ccc"
        }, 
         {
            fillColor: "rgba(174,97,21,0.2)", 
            strokeColor: "rgba(174,97,21,1)",
            pointColor: "rgba(174,97,21,1)",
            pointStrokeColor: "#fff",
            pointHighlightFill: "#fff",
            pointHighlightStroke: "#ccc"
        }, 
         {
            fillColor: "rgba(114,59,152,0.2)", 
            strokeColor: "rgba(114,59,152,1)",
            pointColor: "rgba(114,59,152,1)",
            pointStrokeColor: "#fff",
            pointHighlightFill: "#fff",
            pointHighlightStroke: "#ccc"
        }
    ];
    
    var chartData = {
        labels: dataCollection.xLabel,
        datasets: []
    };
    //prepare chart data
    for(var i=0; i<dataCollection.dataList.length; i++){
        var currentData = dataCollection.dataList[i];
        if(i>colorSet.length)   this.createRandomColor(i, colorSet);
        chartData.datasets[i] = {};
        chartData.datasets[i].label = currentData.label;
        if(currentData.lineConfig.fill == false) chartData.datasets[i].fillColor = "rgba(0,0,0,0)";
        else chartData.datasets[i].fillColor = colorSet[i].fillColor;
        if(currentData.lineConfig.hasPoint == false){
             chartData.datasets[i].strokeColor = "rgba(0,0,0,0)";
             chartData.datasets[i].pointStrokeColor = "rgba(0,0,0,0)";
             chartData.datasets[i].enableHighlight = false;
        }
        else{ 
            chartData.datasets[i].pointColor = colorSet[i].pointColor;
            chartData.datasets[i].pointStrokeColor = colorSet[i].pointStrokeColor;
            chartData.datasets[i].pointHighlightFill = colorSet[i].pointHighlightFill;
            chartData.datasets[i].pointHighlightStroke = colorSet[i].pointHighlightStroke;
            chartData.datasets[i].enableHighlight = true;
        } 
        chartData.datasets[i].strokeColor = colorSet[i].strokeColor;
        chartData.datasets[i].data = currentData.data;
    }
    return chartData;
}

performancePage.prototype.createRandomColor=function(index, colorSet){
    var red = Lego.func.getRandomIntBetween(0, 255);
    var green = Lego.func.getRandomIntBetween(0, 255);
    var blue = Lego.func.getRandomIntBetween(0, 255);
    colorSet[index] = {
            fillColor: "rgba(" + red + "," + green + "," + blue + ",0.2)", 
            strokeColor: "rgba(" + red + "," + green + "," + blue + ",1)",
            pointColor: "rgba(" + red + "," + green + "," + blue + ",1)",
            pointStrokeColor: "#fff",
            pointHighlightFill: "#fff",
            pointHighlightStroke: "#ccc"
        };
}

performancePage.prototype.resize=function(){
    $(this.perfTabFrame).height($(this.mainElement).height() - $($(this.perfTabFrame).data('titleFrame')).height() - parseInt($($(this.perfTabFrame).data('titleFrame')).css('margin-top')));
    this.perfTabCTRLer.resize();
    this.buttonCTRLer.resize();
}  

