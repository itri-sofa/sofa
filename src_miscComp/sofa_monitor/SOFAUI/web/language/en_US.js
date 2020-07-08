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
language.TAB_DASHBOARD = "Dashboard";
language.TAB_PERFORMANCE = "Performance";
language.TAB_SYSTEM_SETTING = "Setting";
//Dashboard Page
language.DASHBOARD_STATUS_TITLE = "System Info";
language.DASHBOARD_STATUS_TITLE_RAID = "RAID Info";

//Performance Page
language.PERFORMANCE_TITLE = "Performance";
language.IOPS_TITLE = "IOPS Trend";
language.WRITE_TITLE = "Write Trend";
language.READ_TITLE = "Read Trend";
language.IOPS_TAB = "IOPS";
language.WRITE_TAB = "Write";
language.READ_TAB = "Read";
//Setting Page
language.SETTING_TITLE = "System Setting";
language.SETTING_GC_SETTING_TITLE = "GC";
language.SETTING_GC_PRESERVE_TITLE = "Space reservation percentage";
language.SETTING_GC_START_TITLE ="GC will start when free space percentage is less than(%)";
language.SETTING_GC_STOP_TITLE = "GC will stop when free space percentage is greater than(%)";
language.SETTING_SET_BUTTON = "Set";
language.SETTING_SETRESTART_BUTTON = "Set";
language.SETTING_PERFORMANCE_SETTING_TITLE = "Alarm";
language.SETTING_IOPS_ThRESHOLD_TITLE = "System alarm when IOPS is lower than";
language.SETTING_FREESPACE_THRESHOLD_TITLE = "System alarm when free space (GB) is lower than";

language.SETTING_SUCCESS_SET_TITLE = "SUCCESS";
language.SETTING_FAIL_SET_TITLE = "OOPS...";
language.SETTING_SUCCESS_SET_CONTENT = "Set Success.";
language.SETTING_FAIL_SET_CONTENT = "Set Failed";
//STATUS
language.LOADING = "Loading...";
language.OK_STATUS = "OK";
language.DEGRADED = "Degraded";
language.CLOSE = "Close";
language.INIT = "Recovery";
language.UNKNOWN = "Unknown";

language.REMOVED = "Removed";
language.REBUILDING = "Rebuilding";
language.READONLY = "Read Only";
language.FAILED = "Failed";

//INFORMATION
language.SPACE_USAGE_STRING = "!usage !uUnit of !quota !qUnit Used (!percent%)";
language.SPACE_USAGE_SHORT_STRING = "!usage / !quota !qUnit (!percent%)";

language.SYSTEM_HEALTH = "System Health";

language.GROUP_SPACE = "Raid ";
language.SPARE = "Spare Disks";

language.SPACE_USAGE = "Space Usage";
language.GC_INFO = "Over-provisioning";
language.RESERVED_COMMON = "Reserved: ";

language.PHYSICAL = "Physical";
language.CAPACITY = "Capacity";

language.LOGICAL = "User";

language.ALL = "All";

language.SMART = "S.M.A.R.T.";

language.GROUP_ID = "Raid ID";
language.STATUS = "Status";
language.PHYSICAL_CAPACITY = "Physical Capacity";
language.ACTIONS = "";

language.ATA_STANDARD = "ATA Standard";
language.ATA_VERSION = "ATA Version";
language.DEVICE = "Device";
language.DEVICE_MODEL = "Device Model";
language.FIRMWARE_VERSION = "Firmware Version";
language.LOCAL_TIME = "Local Time";
language.SERIAL_NUMBER = "Serial Number";
language.SMART_HEALTH = "S.M.A.R.T. Health";
language.SMART_SUPPORT = "S.M.A.R.T. Support";
language.USER_CAPACITY = "User Capacity";

language.TITLE = "Title";
language.DESCRIPTION = "Description";

language.ID = "ID";
language.ATTRIBUTE = "Attribute";
language.VALUE = "Value";
language.WORST = "Worst";
language.THRESHOLD = "Threshold";
language.RAW_VALUES = "Raw Values";

language.DETAIL = "Detail";
language.BACK = "Back";

language.POPUP_WARNING_TITLE = "WARNING";
language.WARNING_SOFA_RESTART = "All data and SOFA configurations will be cleared! Please make sure all data has been backed up before continuing.";
language.WARNING_SOFA_STARTUP = "SOFA is either not running or not ready, do you want to start SOFA?";
language.AUTO_STARTUP_SOFA = "Start SOFA..";
language.CANNOT_STARTUP_SOFA = "Cannot start SOFA..";

language.OK = "OK";
language.CANCEL = "Cancel";

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

language.IOPS_TOO_LOW_CONTENT_1 = "IOPS too low, ";
language.IOPS_TOO_LOW_CONTENT_2 = " IO/s < ";
language.IOPS_TOO_LOW_CONTENT_3 = " IO/s.";

language.FREESPACE_TOO_LOW_CONTENT_1 = "Free space too low, ";
language.FREESPACE_TOO_LOW_CONTENT_2 = " GB < ";
language.FREESPACE_TOO_LOW_CONTENT_3 = " GB.";

language.CANNOT_GET_IOPS = "Cannot get IOPS.";
language.CANNOT_GET_FREESPACE = "Cannot get free space.";
language.FREESPACE = "Free space";
language.COLON = ":";
language.SELECT_LANGUAGE = "Select Language";