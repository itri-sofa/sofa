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

jLego.objectUI.id.statusMonitor={};
jLego.objectUI.cls.statusMonitor={};
jLego.objectUI.constants.statusMonitor={};

//********************** ID **************************
jLego.objectUI.id.statusMonitor.STATUSMONITOR_MONITORFRAME="statusMonitor_MonitorFrame_";

//********************** CLASS **************************
jLego.objectUI.cls.statusMonitor.STATUSMONITOR_ICONMONITOR_FRAME="statusMonitor_IconMonitorFrame";
jLego.objectUI.cls.statusMonitor.STATUSMONITOR_ICONMONITOR_ICON="statusMonitor_Icon";
jLego.objectUI.cls.statusMonitor.STATUSMONITOR_ICONMONITOR_TITLE="statusMonitor_Title";
jLego.objectUI.cls.statusMonitor.STATUSMONITOR_ICONMONITOR_STATUSTEXT="statusMonitor_StatusText";
jLego.objectUI.cls.statusMonitor.STATUSMONITOR_ICONMONITOR_STATUSTEXT_OK="statusMonitor_StatusText_OK";
jLego.objectUI.cls.statusMonitor.STATUSMONITOR_ICONMONITOR_STATUSTEXT_DEGRADED="statusMonitor_StatusText_Degraded";
jLego.objectUI.cls.statusMonitor.STATUSMONITOR_ICONMONITOR_STATUSTEXT_CLOSE="statusMonitor_StatusText_Close";
jLego.objectUI.cls.statusMonitor.STATUSMONITOR_ICONMONITOR_STATUSTEXT_FAILED="statusMonitor_StatusText_Failed";
jLego.objectUI.cls.statusMonitor.STATUSMONITOR_ICONMONITOR_STATUSTEXT_INIT="statusMonitor_StatusText_Init";

jLego.objectUI.cls.statusMonitor.STATUSMONITOR_SPIN = "statusMonitor_Icon_Spin";

jLego.objectUI.cls.statusMonitor.STATUSMONITOR_PIEBACKGROUND_FRAME="statusMonitor_PieBackground";
jLego.objectUI.cls.statusMonitor.STATUSMONITOR_PIE_FRAME="statusMonitor_Pie";
jLego.objectUI.cls.statusMonitor.STATUSMONITOR_ICONMONITOR_PIE_STATUSTEXT="statusMonitor_Pie_StatusText";

jLego.objectUI.cls.statusMonitor.STATUSMONITOR_BARMONITOR_FRAME="statusMonitor_BarMonitor_Frame";
jLego.objectUI.cls.statusMonitor.STATUSMONITOR_BARMONITOR_TITLE="statusMonitor_BarMonitor_Title";
jLego.objectUI.cls.statusMonitor.STATUSMONITOR_BARMONITOR_BARFRAME="statusMonitor_BarMonitor_BarFrame";
jLego.objectUI.cls.statusMonitor.STATUSMONITOR_BARMONITOR_BARELEMENT="statusMonitor_BarMonitor_BarElement";

jLego.objectUI.cls.statusMonitor.STATUSMONITOR_ARRAYMONITOR_FRAME="statusMonitor_ArrayMonitor_Frame";
jLego.objectUI.cls.statusMonitor.STATUSMONITOR_ARRAYMONITOR_ICON="statusMonitor_ArrayMonitor_Icon";
jLego.objectUI.cls.statusMonitor.STATUSMONITOR_ARRAYMONITOR_TEXT="statusMonitor_ArrayMonitor_Text";
jLego.objectUI.cls.statusMonitor.STATUSMONITOR_SINGLEMONITOR_FRAME="statusMonitor_SingleMonitor_Frame";
jLego.objectUI.cls.statusMonitor.STATUSMONITOR_SINGLEMONITOR_TEXT="statusMonitor_SingleMonitor_Text";

jLego.objectUI.cls.statusMonitor.STATUSMONITOR_TWOLINE_MONITOR_FRAME="statusMonitor_TwoLineMonitor_Frame";
jLego.objectUI.cls.statusMonitor.STATUSMONITOR_TWOLINE_MONITOR_TEXT="statusMonitor_TwoLineMonitor_Text";

jLego.objectUI.cls.statusMonitor.STATUSMONITOR_ICONWARNING="statusMonitor_IconWarning";

jLego.objectUI.cls.statusMonitor.STATUSMONITOR_BIGROUND_VALUE_FRAME="statusMonitor_BigRound_ValueFrame";
jLego.objectUI.cls.statusMonitor.STATUSMONITOR_BIGROUND_UNIT_FRAME="statusMonitor_BigRound_UnitFrame";
//********************** CONSTANTS **************************
jLego.objectUI.constants.statusMonitor.BAR_WARN_PERCENTAGE=75;
jLego.objectUI.constants.statusMonitor.BAR_ERROR_PERCENTAGE=90;

jLego.objectUI.constants.statusMonitor.BAR_COLOR_SAFE = '#006CCF';
jLego.objectUI.constants.statusMonitor.BAR_COLOR_WARN = '#edc211';
jLego.objectUI.constants.statusMonitor.BAR_COLOR_ERROR = '#FF5252';

jLego.objectUI.constants.statusMonitor.TEXT_COLOR_OK = '#009900';
jLego.objectUI.constants.statusMonitor.TEXT_COLOR_INIT = '#055893';
jLego.objectUI.constants.statusMonitor.TEXT_COLOR_DEGRADED = '#B7950D';
jLego.objectUI.constants.statusMonitor.TEXT_COLOR_CLOSE = '#547D94';
jLego.objectUI.constants.statusMonitor.TEXT_COLOR_FAILED = '#FF5252';