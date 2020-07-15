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
package itri.sofa.basicAPI;

import org.codehaus.jettison.json.JSONObject;

/**
 *
 * @author A40385
 */
public interface BasicAPIInterface {
    public JSONObject getPhysicalCapacity();
    public JSONObject getCapacity();
    public JSONObject getGroups();
    public JSONObject getGroupDisks(int groupID);
    public JSONObject getSparedDisks();
    public JSONObject getDiskSMART(String diskUUID);
    public JSONObject getGCInfo();
    public JSONObject getIOPS(int pointCount);
    public JSONObject getSystemRunningStatus();
    public String getSystemProperties();
    public String getIOPSThreshold();
    public String getFreeListThreshold();
    public JSONObject setGCRatio(int minGCRatio, int maxGCRatio);
    public JSONObject setGCReserve(int gcReserve);
    public JSONObject setIOPSThreshold(double iopsThreshold);
    public JSONObject setFreeListThreshold(double freeListThreshold);
    public JSONObject startupSOFA();
}
