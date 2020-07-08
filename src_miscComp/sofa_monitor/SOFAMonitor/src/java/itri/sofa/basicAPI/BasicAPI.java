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
package itri.sofa.basicAPI;

import java.nio.charset.Charset;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.Properties;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import org.codehaus.jettison.json.JSONException;
import org.codehaus.jettison.json.JSONObject;

/**
 *
 * @author A40385
 */
public class BasicAPI implements BasicAPIInterface {
    private boolean fakeDataMode = false;
    private String fileName = "sofa_ui.cfg";
    
    private String writeUIConfig(String key, String data)
        throws IOException {
        File file = new File(fileName);
        if(file.exists()) {
            FileReader reader = new FileReader(file);
            Properties props = new Properties();
            props.load(reader);
            reader.close();
            props.setProperty(key, data);
            FileWriter writer = new FileWriter(file);
            props.store(writer, "SOFA Web UI config");
            writer.close();
        }
        return "{" +
                   "\"" + "errCode" + "\"" + ":" + "0" +
               "}";
    }
    private String readUIConfig(String key)
        throws IOException {
        File file = new File(fileName);
        String result = null;
        int pos;
        if(file.exists()) {
            FileReader reader = new FileReader(file);
            Properties props = new Properties();
            props.load(reader);
            result = props.getProperty(key);
 
            reader.close();
        }
        return result;
    }

    public BasicAPI(){
        this.fakeDataMode = false;
    }
    public BasicAPI(boolean fakeDataMode){
        this.fakeDataMode = fakeDataMode;
    }
    @Override
    public JSONObject getPhysicalCapacity() {
        JSONObject jsonResult = null;
        try {
            String rawResult = "";
            if(!fakeDataMode)
                rawResult = new SOFA().run("/usr/sofa/bin/sofaadm --lfsm --getphycap");
            else
                rawResult = FakeDATA.getPhysicalCap();
            String result = rawResult.substring(rawResult.indexOf("{"), rawResult.lastIndexOf("}")+1);
            jsonResult = new JSONObject(result);
            
        } catch (JSONException ex) {
            Logger.getLogger(BasicAPI.class.getName()).log(Level.SEVERE, null, ex);
        } finally{
            return jsonResult;
        }
    }

    @Override
    public JSONObject getCapacity() {
        JSONObject jsonResult = null;
        try {
            String rawResult = "";
            if(!fakeDataMode)
                rawResult = new SOFA().run("/usr/sofa/bin/sofaadm --lfsm --getcap");
            else
                rawResult = FakeDATA.getCapacity();
            String result = rawResult.substring(rawResult.indexOf("{"), rawResult.lastIndexOf("}")+1);
            jsonResult = new JSONObject(result);
            
        } catch (JSONException ex) {
            Logger.getLogger(BasicAPI.class.getName()).log(Level.SEVERE, null, ex);
        } finally{
            return jsonResult;
        }
    }

    @Override
    public JSONObject getGroups() {
        JSONObject jsonResult = null;
        try {
            String rawResult = "";
            if(!fakeDataMode)
                rawResult = new SOFA().run("/usr/sofa/bin/sofaadm --lfsm --getgroup");
            else
                rawResult = FakeDATA.getGroups();
            String result = rawResult.substring(rawResult.indexOf("{"), rawResult.lastIndexOf("}")+1);
            jsonResult = new JSONObject(result);
            
        } catch (JSONException ex) {
            Logger.getLogger(BasicAPI.class.getName()).log(Level.SEVERE, null, ex);
        } finally{
            return jsonResult;
        }
    }

    @Override
    public JSONObject getGroupDisks(int groupID) {
        JSONObject jsonResult = null;
        try {
            String rawResult = "";
            System.out.print("fakeDataMode: " + fakeDataMode);
            if(!fakeDataMode)
                rawResult = new SOFA().run("/usr/sofa/bin/sofaadm --lfsm --getgroupdisk " + groupID);
            else
                rawResult = FakeDATA.getGroupDisks(groupID);
            System.out.println(rawResult);
            String result = rawResult.substring(rawResult.indexOf("{"), rawResult.lastIndexOf("}")+1);
            System.out.println(result);
            jsonResult = new JSONObject(result);
            
        } catch (JSONException ex) {
            Logger.getLogger(BasicAPI.class.getName()).log(Level.SEVERE, null, ex);
        } finally{
            return jsonResult;
        }
    }

    @Override
    public JSONObject getSparedDisks() {
        JSONObject jsonResult = null;
        try {
            String rawResult = "";
            if(!fakeDataMode)
                rawResult = new SOFA().run("/usr/sofa/bin/sofaadm --lfsm --getsparedisk");
            else
                rawResult = FakeDATA.getSpareDisk();
            String result = rawResult.substring(rawResult.indexOf("{"), rawResult.lastIndexOf("}")+1);
            jsonResult = new JSONObject(result);
            
        } catch (JSONException ex) {
            Logger.getLogger(BasicAPI.class.getName()).log(Level.SEVERE, null, ex);
        } finally{
            return jsonResult;
        }
    }

    @Override
    public JSONObject getDiskSMART(String diskUUID) {
        JSONObject jsonResult = null;
        try {
            String rawResult = "";
            if(!fakeDataMode)
                rawResult = new SOFA().run("/usr/sofa/bin/sofaadm --lfsm --getsmart " + diskUUID);
            else
                rawResult = FakeDATA.getDiskSMART(diskUUID);
            String result = rawResult.substring(rawResult.indexOf("{"), rawResult.lastIndexOf("}")+1);
            jsonResult = new JSONObject(result);
            
        } catch (JSONException ex) {
            Logger.getLogger(BasicAPI.class.getName()).log(Level.SEVERE, null, ex);
        } finally{
            return jsonResult;
        }
    }

    @Override
    public JSONObject getGCInfo() {
        JSONObject jsonResult = null;
        try {
            String rawResult = "";
            if(!fakeDataMode)
                rawResult = new SOFA().run("/usr/sofa/bin/sofaadm --lfsm --getgcinfo");
            else
                rawResult = FakeDATA.getGCInfo();
            String result = rawResult.substring(rawResult.indexOf("{"), rawResult.lastIndexOf("}")+1);
            jsonResult = new JSONObject(result);
            
        } catch (JSONException ex) {
            Logger.getLogger(BasicAPI.class.getName()).log(Level.SEVERE, null, ex);
        } finally{
            return jsonResult;
        }
    }

    @Override
    public JSONObject getIOPS(int pointCount) {
        JSONObject jsonResult = null;
        try {
            String rawResult = "";
            if(!fakeDataMode)
                rawResult = new SOFA().run("/usr/sofa/bin/sofaadm --lfsm --getperf " + pointCount);
            else
                rawResult = FakeDATA.getIOPS(pointCount);
            String result = rawResult.substring(rawResult.indexOf("{"), rawResult.lastIndexOf("}")+1);
            jsonResult = new JSONObject(result);
            
        } catch (JSONException ex) {
            Logger.getLogger(BasicAPI.class.getName()).log(Level.SEVERE, null, ex);
        } finally{
            return jsonResult;
        }
    }

    @Override
    public JSONObject getSystemRunningStatus() {
        JSONObject jsonResult = null;
        try {
            String rawResult = "";
            if(!fakeDataMode)
                rawResult = new SOFA().run("/usr/sofa/bin/sofaadm --lfsm --getstatus");
            else
                rawResult = FakeDATA.getSystemRunningStatus();
            String result = rawResult.substring(rawResult.indexOf("{"), rawResult.lastIndexOf("}")+1);
            jsonResult = new JSONObject(result);
            
        } catch (JSONException ex) {
            Logger.getLogger(BasicAPI.class.getName()).log(Level.SEVERE, null, ex);
        } finally{
            return jsonResult;
        }
    }

    @Override
    public String getSystemProperties() {
        JSONObject jsonResult = null;
        String rawResult = "";
        try {
            if(!fakeDataMode)
                rawResult = new SOFA().run("/usr/sofa/bin/sofaadm --lfsm --getproperties");
            else
                rawResult = FakeDATA.getSystemProperties();
            
        } catch (Exception ex) {
            Logger.getLogger(BasicAPI.class.getName()).log(Level.SEVERE, null, ex);
        } finally{
            return rawResult;
        }
    }

    @Override
    public String getIOPSThreshold() {
        String rawResult = "";
        try {
            if(!fakeDataMode)
                rawResult = readUIConfig("iops_Threshold");
            else
                rawResult = "0";
        } catch (Exception ex) {
            Logger.getLogger(BasicAPI.class.getName()).log(Level.SEVERE, null, ex);
        } finally{
            return rawResult;
        }
    }

    @Override
    public String getFreeListThreshold() {
        String rawResult = "";
        try {
            if(!fakeDataMode)
                rawResult = readUIConfig("freelist_Threshold");
            else
                rawResult = "0";
        } catch (Exception ex) {
            Logger.getLogger(BasicAPI.class.getName()).log(Level.SEVERE, null, ex);
        } finally{
            return rawResult;
        }
    }

    @Override
    public JSONObject setGCRatio(int minGCRatio, int maxGCRatio) {
       JSONObject jsonResult = null;
        try {
            String rawResult = "";
            if(!fakeDataMode)
                rawResult = new SOFA().run("/usr/sofa/bin/sofaadm --lfsm --setgcratio " + minGCRatio + " " + maxGCRatio);
            else
                rawResult = FakeDATA.setGCRatio(minGCRatio, maxGCRatio);
            String result = rawResult.substring(rawResult.indexOf("{"), rawResult.lastIndexOf("}")+1);
            jsonResult = new JSONObject(result);
            
        } catch (JSONException ex) {
            Logger.getLogger(BasicAPI.class.getName()).log(Level.SEVERE, null, ex);
        } finally{
            return jsonResult;
        }
    }

    @Override
    public JSONObject setGCReserve(int gcReserve) {
        JSONObject jsonResult = null;
        try {
            String rawResult = "";
            if(!fakeDataMode)
                rawResult = new SOFA().run("/usr/sofa/bin/sofaadm --lfsm --setgcreserve " + gcReserve);
            else
                rawResult = FakeDATA.setGCReserve(gcReserve);
            String result = rawResult.substring(rawResult.indexOf("{"), rawResult.lastIndexOf("}")+1);
            jsonResult = new JSONObject(result);
            
        } catch (JSONException ex) {
            Logger.getLogger(BasicAPI.class.getName()).log(Level.SEVERE, null, ex);
        } finally{
            return jsonResult;
        }
    }

    @Override
    public JSONObject setIOPSThreshold(double iopsThreshold) {
        JSONObject jsonResult = null;
        try {
            String rawResult = "";
            if(!fakeDataMode)
                rawResult = writeUIConfig("iops_Threshold", "" + iopsThreshold);
            else
                rawResult = FakeDATA.setIOPSThreshold(iopsThreshold);
            String result = rawResult.substring(rawResult.indexOf("{"), rawResult.lastIndexOf("}")+1);
            jsonResult = new JSONObject(result);
            
        } catch (JSONException ex) {
            Logger.getLogger(BasicAPI.class.getName()).log(Level.SEVERE, null, ex);
        } finally{
            return jsonResult;
        }
    }

    @Override
    public JSONObject startupSOFA() {
        JSONObject jsonResult = null;
        try {
            String rawResult = "";
            if(!fakeDataMode)
                rawResult = new SOFA().run("/usr/sofa/bin/sofaadm --lfsm --activate");
            else
                rawResult = FakeDATA.startupSOFA();
            String result = rawResult.substring(rawResult.indexOf("{"), rawResult.lastIndexOf("}")+1);
            jsonResult = new JSONObject(result);
            
        } catch (JSONException ex) {
            Logger.getLogger(BasicAPI.class.getName()).log(Level.SEVERE, null, ex);
        } finally{
            return jsonResult;
        }
    }

    @Override
    public JSONObject setFreeListThreshold(double freeListThreshold) {
        JSONObject jsonResult = null;
        try {
            String rawResult = "";
            if(!fakeDataMode)
                rawResult = writeUIConfig("freelist_Threshold", "" + freeListThreshold);
            else
                rawResult = FakeDATA.setFreeListThreshold(freeListThreshold);
            String result = rawResult.substring(rawResult.indexOf("{"), rawResult.lastIndexOf("}")+1);
            jsonResult = new JSONObject(result);
            
        } catch (JSONException ex) {
            Logger.getLogger(BasicAPI.class.getName()).log(Level.SEVERE, null, ex);
        } finally{
            return jsonResult;
        }
    }
    
}
