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
package org.itri.sofaServlet;

/**
 *
 * @author A40385
 */
public class ServletConfig {
    private static String rootPath= "/SOFAMonitor/";
    public static String getSystemInfo= rootPath + "GetSystemInfoServlet";
    public static String getDiskSMART= rootPath + "GetDiskSMARTServlet";
    public static String getIOPS = rootPath + "GetIOPSServlet";
    public static String getSOFARunning = rootPath + "GetSOFARunningServlet";
    public static String setGCRatio = rootPath + "SetGCRatioServlet";
    public static String setGCReserve = rootPath + "SetGCReserveServlet";
    public static String setIOPSThreshold = rootPath + "SetIOPSThresholdServlet";
    public static String setFreeListThreshold = rootPath + "SetFreeListThresholdServlet";
    public static String startupSOFA = rootPath + "StartUpSOFAServlet";
    public static String responsePath= "?response=application/json";
    public static String getSystemProperties = rootPath + "GetSystemPropertiesServlet";
}
