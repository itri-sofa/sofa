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

jLego.objectUI.nodeTable=function(){
    var myID;
    var myClass;
    var myConsts;
    var tableBaseID;
    
    var parentElement;
    var tableElement;
    var inputOption;
    var columnCount;
    this.initialize();
}

jLego.objectUI.nodeTable.prototype.initialize=function(){
    this.myID=jLego.objectUI.id.nodeTable;
    this.myClass=jLego.objectUI.cls.nodeTable;
    this.myConsts=jLego.objectUI.constants.nodeTable;
}

jLego.objectUI.nodeTable.prototype.add=function(target, option){
    //check option
    if(target==null) return null;
    
    if(option==null){
        var option={columnTitleList: [], widthType: [], minWidthList: []};
        return null;
    }
    else{
        if(option.columnTitleList==null)   return null;
        else if(option.widthType==null)   return null;
        else if(option.minWidthList==null)   return null;
        else if(option.rowCount==null)   return null;
    }
    var columnNumber = Math.min(option.columnTitleList.length, option.widthType.length, option.minWidthList.length);
    
    var sizeWidth = $(target).width() - 4 * columnNumber;
    var widthList =[];
    var widthSum = 0;
    var dynamicIndex = -1;
    for(var i=0; i<columnNumber; i++){
        widthList[i] = parseInt(option.minWidthList[i]);
        if(option.widthType[i]=='fix')  widthSum += parseInt(option.minWidthList[i]);
        else if(option.widthType[i]=='dynamic') dynamicIndex = i;
    }

    if(i!=-1 && (sizeWidth-widthSum)>widthList[dynamicIndex])   widthList[dynamicIndex] = sizeWidth - widthSum;
    //draw table
    var defID = this.myID.NODETABLE_ID + jLego.func.getRandomString();
    this.tableBaseID = defID;
    //add table
    var nodeTable, tableTr, tableTh, tableTd;
    nodeTable = 
            jLego.basicUI.addTable(target, {id: defID+"_table", class: this.myClass.NODETABLE_TABLE_FRAME});
    $(nodeTable).width($(target).width());
    //add table header
    tableTr = 
            jLego.basicUI.addTableTr(nodeTable);

    for(var i=0;i<columnNumber;i++){   
        tableTh = 
            jLego.basicUI.addTableTh(tableTr, {content: option.columnTitleList[i]});
        $(tableTh).width(widthList[i]);
    }
    //add table content
    for(var i=0;i<option.rowCount;i++){   
        tableTr = 
            jLego.basicUI.addTableTr(nodeTable);
        for(var j=0;j<columnNumber;j++){   
            tableTd =
                    jLego.basicUI.addTableTd(tableTr, {content: ""});
            $(tableTd).width(widthList[j]);
            var container = 
                jLego.basicUI.addDiv(tableTd, {id: defID+"_"+ i +"_"+ j, class: this.myClass.NODETABLE_TABLE_CONTAINER_FRAME});
        }
    }
    
    //save element for resizing
    this.tableElement = nodeTable;
    this.parentElement = target;
    this.inputOption = option;
    this.columnCount = columnNumber;
}

jLego.objectUI.nodeTable.prototype.getContainer = function(row, column){
    if(row==null || column==null)   return null;
    return document.getElementById(this.tableBaseID+"_"+ row +"_"+ column);
}

jLego.objectUI.nodeTable.prototype.getTr = function(index){
    if(index==null )   return null;
    return document.getElementById(this.tableBaseID+"_table").rows[index];
}

jLego.objectUI.nodeTable.prototype.resize=function(){
    var columnNumber = Math.min(this.inputOption.columnTitleList.length, this.inputOption.widthType.length, this.inputOption.minWidthList.length);
    
    $(this.tableElement).width($(this.parentElement).width()-2);
    
    var sizeWidth = $(this.parentElement).width() - 4 * columnNumber;
    var widthList =[];
    var widthSum = 0;
    var dynamicIndex = -1;
    for(var i=0; i<columnNumber; i++){
        widthList[i] = parseInt(this.inputOption.minWidthList[i]);
        if(this.inputOption.widthType[i]=='fix')  widthSum += parseInt(this.inputOption.minWidthList[i]);
        else if(this.inputOption.widthType[i]=='dynamic') dynamicIndex = i;
    }

    if(i!=-1 && (sizeWidth-widthSum)>widthList[dynamicIndex])   widthList[dynamicIndex] = sizeWidth - widthSum;
    var tempTable = document.getElementById($(this.tableElement).attr('id'));
    if(tempTable!=null){
        var rows = tempTable.rows;
        for(var i=0; i<rows.length; i++){
            var cells = rows[i].cells;
            for(var j=0; j<cells.length; j++){
                $(cells[j]).width(widthList[j]);
            }
        }
    }
    
}