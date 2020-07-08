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
var language = {};

//TAB
language.TAB_DASHBOARD = "系統面板";
language.TAB_PERFORMANCE = "效能";
language.TAB_SYSTEM_SETTING = "系統設定";
//Dashboard Page
language.DASHBOARD_STATUS_TITLE = "系統狀態";

//Performance Page
language.PERFORMANCE_TITLE = "效能";
language.IOPS_TITLE = "IOPS趨勢";
language.WRITE_TITLE = "寫入趨勢";
language.READ_TITLE = "讀取趨勢";
language.IOPS_TAB = "IOPS";
language.WRITE_TAB = "寫入";
language.READ_TAB = "讀取";
//Setting Page
language.SETTING_TITLE = "系統設定";
language.SETTING_GC_SETTING_TITLE = "GC";
language.SETTING_GC_PRESERVE_TITLE = "空間保留比例";
language.SETTING_GC_START_TITLE ="GC啟動，當剩餘空間小於(%)";
language.SETTING_GC_STOP_TITLE = "GC關閉，當剩餘空間大於(%)";
language.SETTING_SET_BUTTON = "設定";
language.SETTING_SETRESTART_BUTTON = "設定";
language.SETTING_PERFORMANCE_SETTING_TITLE = "告警";
language.SETTING_IOPS_ThRESHOLD_TITLE = "通知當 IOPS 小於";
language.SETTING_FREESPACE_THRESHOLD_TITLE = "通知當剩餘空間 (GB) 小於";

language.SETTING_SUCCESS_SET_TITLE = "成功！";
language.SETTING_FAIL_SET_TITLE = "糟糕...";
language.SETTING_SUCCESS_SET_CONTENT = "設定成功。";
language.SETTING_FAIL_SET_CONTENT = "設定失敗...";

//STATUS
language.LOADING = "讀取中...";
language.OK_STATUS = "良好";
language.DEGRADED = "警告";
language.CLOSE = "關閉";
language.INIT = "初始中";
language.UNKNOWN = "未知";

language.REMOVED = "移除";
language.REBUILDING = "重建中";
language.READONLY = "唯讀";
language.FAILED = "毀損";

//INFORMATION
language.SPACE_USAGE_STRING = "已用 !usage !uUnit / !quota !qUnit (!percent%)";
language.SPACE_USAGE_SHORT_STRING = "!usage / !quota !qUnit (!percent%)";

language.SYSTEM_HEALTH = "系統健康度";

language.GROUP_SPACE = "磁碟陣列 ";
language.SPARE = "備用磁碟";

language.SPACE_USAGE = "空間使用率";
language.GC_INFO = "預留空間";
language.RESERVED_COMMON = "保留: ";

language.PHYSICAL = "實體";
language.CAPACITY = "容量";

language.LOGICAL = "用戶";

language.ALL = "所有";

language.SMART = "S.M.A.R.T.";

language.GROUP_ID = "磁碟陣列 ID";
language.STATUS = "狀態";
language.PHYSICAL_CAPACITY = "實體容量";
language.ACTIONS = "";

language.ATA_STANDARD = "ATA 標準";
language.ATA_VERSION = "ATA 版本";
language.DEVICE = "裝置";
language.DEVICE_MODEL = "裝置型號";
language.FIRMWARE_VERSION = "韌體版本";
language.LOCAL_TIME = "時間";
language.SERIAL_NUMBER = "序號";
language.SMART_HEALTH = "S.M.A.R.T. 健康度";
language.SMART_SUPPORT = "S.M.A.R.T. 支援";
language.USER_CAPACITY = "用戶容量";

language.TITLE = "標題";
language.DESCRIPTION = "描述";

language.ID = "ID";
language.ATTRIBUTE = "屬性";
language.VALUE = "數值";
language.WORST = "最差值";
language.THRESHOLD = "門檻值";
language.RAW_VALUES = "來源數值";

language.DETAIL = "細節";
language.BACK = "返回";

language.POPUP_WARNING_TITLE = "警告";
language.WARNING_SOFA_RESTART = "請注意！SOFA將重新啟動，並清除所有資料，確定執行？ 執行前請確定您已備份當前SOFA所有設定與資料。";
language.WARNING_SOFA_STARTUP = "SOFA尚未啟動或未啟動完成，是否啟動SOFA？";
language.AUTO_STARTUP_SOFA = "自動啟動SOFA..";
language.CANNOT_STARTUP_SOFA = "無法啟動SOFA..";

language.OK = "確定";
language.CANCEL = "取消";

//Date
language.JAN = "Jan";
language.FEB = "Feb";
language.MAR = "Mar";
language.APR = "Apr";
language.MAY = "May";
language.JUN = "Jun";
language.JUL = "Jul";
language.AUG = "Aug";
language.SEP = "Sep";
language.OCT = "Oct";
language.NOV = "Nov";
language.DEC = "Dec";

language.IOPS_TOO_LOW_CONTENT_1 = "IOPS過低，";
language.IOPS_TOO_LOW_CONTENT_2 = " IO/s < ";
language.IOPS_TOO_LOW_CONTENT_3 = " IO/s。";

language.FREESPACE_TOO_LOW_CONTENT_1 = "剩餘空間過少，";
language.FREESPACE_TOO_LOW_CONTENT_2 = " GB < ";
language.FREESPACE_TOO_LOW_CONTENT_3 = " GB。";

language.CANNOT_GET_IOPS = "無法取得IOPS資訊。";
language.CANNOT_GET_FREESPACE = "無法取得剩餘空間資訊。";
language.FREESPACE = "剩餘空間";
language.COLON = "：";
language.SELECT_LANGUAGE = "選擇語言";