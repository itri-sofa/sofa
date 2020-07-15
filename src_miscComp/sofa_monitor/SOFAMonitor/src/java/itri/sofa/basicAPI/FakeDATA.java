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

import itri.sofa.utils.Utils;

/**
 *
 * @author A40385
 */
public class FakeDATA {
    public static String getPhysicalCap(){
        String fakeResult = "{" +
                                "\"" + "physicalCapacity" + "\"" + ":" + "5096000000000" + "," +
                                 "\"" + "capacity" + "\"" + ":" + "5096000000000" +
                            "}";
        return fakeResult;
    }
    
    public static String getCapacity(){
        String fakeResult = "{" +
                                 "\"" + "capacity" + "\"" + ":" + "4096000000000" +
                            "}";
        return fakeResult;
    }
    
    public static String getGroups(){
        String fakeResult = "{" +
                                 "\"" + "groupNum" + "\"" + ":" + "2" + "," +
                                 "\"" + "groups" + "\"" + ":" + "[" + 
                                                                   "{" +
                                                                        "\"" + "capacity" + "\"" + ":" + "4096000000000" + "," +
                                                                        "\"" + "physicalCapacity" + "\"" + ":" + "6096000000000" + "," +
                                                                        "\"" + "usedSpace" + "\"" + ":" + "66000000000" +
                                                                   "}" +
                                                                   "," + 
                                                                   "{" +
                                                                        "\"" + "capacity" + "\"" + ":" + "4096000000000" + "," +
                                                                        "\"" + "physicalCapacity" + "\"" + ":" + "6096000000000" + "," +
                                                                        "\"" + "usedSpace" + "\"" + ":" + "86000000000" +
                                                                   "}" +
                                                                "]" +
                            "}";
        return fakeResult;
    }
    
    public static String getGroupDisks(int groupIndex){
        String fakeResult = "{" +
                                 "\"" + "physicalCapacity" + "\"" + ":" + "100" + "," +
                                 "\"" + "capacity" + "\"" + ":" + "100" + "," +
                                 "\"" + "usedSpace" + "\"" + ":" + "50" + "," +
                                 "\"" + "disks" + "\"" + ":" + "[" + 
                                                                   "{" +
                                                                        "\"" + "id" + "\"" + ":" + "\"" + "sda" + "\"" + "," +
                                                                        "\"" + "diskSize" + "\"" + ":" + "5120000000" + "," +
                                                                        "\"" + "status" + "\"" + ":" + "\"" + "ok" + "\"" +
                                                                   "}" +
                                                                   "," + 
                                                                   "{" +
                                                                        "\"" + "id" + "\"" + ":" + "\"" + "sdb" + "\"" + "," +
                                                                        "\"" + "diskSize" + "\"" + ":" + "5120000000" + "," +
                                                                        "\"" + "status" + "\"" + ":" + "\"" + "ok" + "\"" +
                                                                   "}" +
                                                                   "," + 
                                                                   "{" +
                                                                        "\"" + "id" + "\"" + ":" + "\"" + "sdc" + "\"" + "," +
                                                                        "\"" + "diskSize" + "\"" + ":" + "5120000000" + "," +
                                                                        "\"" + "status" + "\"" + ":" + "\"" + "ok" + "\"" +
                                                                   "}" +
                                                                   "," + 
                                                                   "{" +
                                                                        "\"" + "id" + "\"" + ":" + "\"" + "sdd" + "\"" + "," +
                                                                        "\"" + "diskSize" + "\"" + ":" + "5120000000" + "," +
                                                                        "\"" + "status" + "\"" + ":" + "\"" + "ok" + "\"" +
                                                                   "}" +
                                                                   "," + 
                                                                   "{" +
                                                                        "\"" + "id" + "\"" + ":" + "\"" + "sde" + "\"" + "," +
                                                                        "\"" + "diskSize" + "\"" + ":" + "5120000000" + "," +
                                                                        "\"" + "status" + "\"" + ":" + "\"" + "ok" + "\"" +
                                                                   "}" +
                                                                   "," + 
                                                                   "{" +
                                                                        "\"" + "id" + "\"" + ":" + "\"" + "sdf" + "\"" + "," +
                                                                        "\"" + "diskSize" + "\"" + ":" + "5120000000" + "," +
                                                                        "\"" + "status" + "\"" + ":" + "\"" + "ok" + "\"" +
                                                                   "}" +
                                                                   "," + 
                                                                   "{" +
                                                                        "\"" + "id" + "\"" + ":" + "\"" + "sdg" + "\"" + "," +
                                                                        "\"" + "diskSize" + "\"" + ":" + "5120000000" + "," +
                                                                        "\"" + "status" + "\"" + ":" + "\"" + "ok" + "\"" +
                                                                   "}" +
                                                                   "," + 
                                                                   "{" +
                                                                        "\"" + "id" + "\"" + ":" + "\"" + "sdh" + "\"" + "," +
                                                                        "\"" + "diskSize" + "\"" + ":" + "5120000000" + "," +
                                                                        "\"" + "status" + "\"" + ":" + "\"" + "ok" + "\"" +
                                                                   "}" +
                                                                   "," + 
                                                                   "{" +
                                                                        "\"" + "id" + "\"" + ":" + "\"" + "sdi" + "\"" + "," +
                                                                        "\"" + "diskSize" + "\"" + ":" + "5120000000" + "," +
                                                                        "\"" + "status" + "\"" + ":" + "\"" + "ok" + "\"" +
                                                                   "}" +
                                                                   "," + 
                                                                   "{" +
                                                                        "\"" + "id" + "\"" + ":" + "\"" + "sdj" + "\"" + "," +
                                                                        "\"" + "diskSize" + "\"" + ":" + "5120000000" + "," +
                                                                        "\"" + "status" + "\"" + ":" + "\"" + "ok" + "\"" +
                                                                   "}" +
                                                                "]" +
                            "}";
        return fakeResult;
    }
    
    public static String getSpareDisk(){
        String fakeResult = "{" +
                                 "\"" + "disks" + "\"" + ":" + "[" + 
                                                                   "{" +
                                                                        "\"" + "id" + "\"" + ":" + "\"" +"sdx" + "\"" + "," +
                                                                        "\"" + "diskSize" + "\"" + ":" + "5120000000" + "," +
                                                                        "\"" + "status" + "\"" + ":" + "\"" + "ok" + "\"" +
                                                                   "}" +
                                                                   "," + 
                                                                   "{" +
                                                                        "\"" + "id" + "\"" + ":" + "\"" + "sdy" + "\"" + "," +
                                                                        "\"" + "diskSize" + "\"" + ":" + "5120000000" + "," +
                                                                        "\"" + "status" + "\"" + ":" + "\"" + "ok" + "\"" +
                                                                   "}" +
                                                                "]" +
                            "}";
        return fakeResult;
    }
    
    public static String getDiskSMART(String diskID){
        String fakeResult = "{" +
                                 "\"" + "diskModel" + "\"" + ":" + "\"" + "M4-CT256M4SSD2" + "\"" +  "," +
                                 "\"" + "firmWareVersion" + "\"" + ":" + "\"" + "0309" + "\"" +  "," +
                                 "\"" + "serialNumber" + "\"" + ":" + "\"" + "000000001207906A1D4" + "\"" +  "," +
                                 "\"" + "reportDate" + "\"" + ":" + "\"" + "08-Nov-2012 16:55:18" + "\"" +  "," +
                                 "\"" + "SMART" + "\"" + ":" +  "[" + "\"Device Model:     ST3500630NS\""+ "," 
                                                                   + "\"Serial Number:    9QG68F8N\""+ "," 
                                                                   + "\"Firmware Version: 3.AEK\""+ "," 
                                                                   + "\"User Capacity:    500,107,862,016 bytes\""+ "," 
                                                                   + "\"Device is:        In smartctl database [for details use: -P show]\""+ "," 
                                                                   + "\"ATA Version is:   7\""+ "," 
                                                                   + "\"ATA Standard is:  Exact ATA specification draft version not indicated\""+ "," 
                                                                   + "\"Local Time is:    Thu Feb 16 09:51:47 2017 CST\""+ "," 
                                                                   + "\"SMART support is: Available - device has SMART capability.\""+ ","
                                                                    + "\"SMART support is: Enabled\""+ ","
                                                                    + "\"SMART overall-health self-assessment test result: PASSED\""+ ","
                                                                    + "\"Please note the following marginal Attributes:\""+ ","
                                                                    + "\"  1 Raw_Read_Error_Rate     0x000f   117   084   006    Pre-fail  Always       -       163683519\""+ "," 
                                                                    + "\"  3 Spin_Up_Time            0x0003   094   093   000    Pre-fail  Always       -       0\""+ "," 
                                                                    + "\"  4 Start_Stop_Count        0x0032   098   098   020    Old_age   Always       -       2330\""+ "," 
                                                                    + "\"  5 Reallocated_Sector_Ct   0x0033   100   100   036    Pre-fail  Always       -       0\""+ ","
                                                                    + "\"  7 Seek_Error_Rate         0x000f   090   060   030    Pre-fail  Always       -       973747294\""+ ","
                                                                    + "\"  9 Power_On_Hours          0x0032   072   072   000    Old_age   Always       -       24762\""+ ","
                                                                    + "\" 10 Spin_Retry_Count        0x0013   100   100   097    Pre-fail  Always       -       0\""+ ","
                                                                    + "\" 12 Power_Cycle_Count       0x0032   100   100   020    Old_age   Always       -       458\""+ ","
                                                                    + "\"187 Reported_Uncorrect      0x0032   100   100   000    Old_age   Always       -       0\""+ ","
                                                                    + "\"189 High_Fly_Writes         0x003a   100   100   000    Old_age   Always       -       0\""+ ","
                                                                    + "\"190 Airflow_Temperature_Cel 0x0022   066   034   045    Old_age   Always   In_the_past 34 (Min/Max 21/35)\""+ ","
                                                                    + "\"194 Temperature_Celsius     0x0022   034   066   000    Old_age   Always       -       34 (0 19 0 0)\""+ ","
                                                                    + "\"195 Hardware_ECC_Recovered  0x001a   066   057   000    Old_age   Always       -       82870317\""+ ","
                                                                    + "\"197 Current_Pending_Sector  0x0012   100   100   000    Old_age   Always       -       0\""+ ","
                                                                    + "\"198 Offline_Uncorrectable   0x0010   100   100   000    Old_age   Offline      -       0\""+ ","
                                                                    + "\"199 UDMA_CRC_Error_Count    0x003e   200   200   000    Old_age   Always       -       0\""+ ","
                                                                    + "\"200 Multi_Zone_Error_Rate   0x0000   100   253   000    Old_age   Offline      -       0\""+ ","
                                                                    + "\"202 Data_Address_Mark_Errs  0x0032   100   253   000    Old_age   Always       -       0\"]" +
                            "}";
        return fakeResult;
    }
    
    public static String getGCInfo(){
        String fakeResult = "{" +
                                 "\"" + "reserved" + "\"" + ":" + "0.65" +
                            "}";
        return fakeResult;
    }
    
    public static String getIOPS(int pointCount){
        String[] fakeIOPS = {"43961", "60000", "72130", "53368", "65247"};
        String[] fakeFreeLists = {"21200", "28134", "29134", "27134", "26664"};
        String fakeResult = "{" +
                                 "\"" + "data" + "\"" + ":" + "[";
        for(int i=0; i<pointCount; i++){
            fakeResult = fakeResult + "{" +
                                        "\"iops\" : " + fakeIOPS[Utils.getRandomInteger(0, 4)] + "," + 
                                        "\"latency\" : " + "0.2" + "," +
                                        "\"freelist\" : " + fakeFreeLists[Utils.getRandomInteger(0, 4)] + "," +   // (freelist *64 ) M / 1024  --> free space
                                        "\"timestamp\" : " + String.valueOf(Utils.getRandomInteger(1482818000, 1482818000 + 1000)) +
                                    "}";
            if(i != (pointCount-1)){
                fakeResult = fakeResult + ",";
            }
        }
        fakeResult = fakeResult +                            
                                 "]" +
                            "}";
        return fakeResult;
    }
    
    public static String getSystemRunningStatus(){
        String fakeResult = "{" +
                                 "\"" + "errCode" + "\"" + ":" + "0" + "," +
                                 "\"" + "isRunning" + "\"" + ":" + "1" +       
                            "}";
        return fakeResult;
    }
    
    public static String startupSOFA(){
        String fakeResult = "{" +
                                 "\"" + "errCode" + "\"" + ":" + "0" +
                            "}";
        return fakeResult;
    }
    
    public static String setGCRatio(int minGCRatio, int maxGCRatio){
        String fakeResult = "{" +
                                 "\"" + "errCode" + "\"" + ":" + "0" +
                            "}";
        return fakeResult;
    }
    
    public static String setGCReserve(int gcReserve){
        String fakeResult = "{" +
                                 "\"" + "errCode" + "\"" + ":" + "0" +
                            "}";
        return fakeResult;
    }
    
    public static String setIOPSThreshold(double iopsThreshold){
        String fakeResult = "{" +
                                 "\"" + "errCode" + "\"" + ":" + "0" +
                            "}";
        return fakeResult;
    }
    
    public static String setFreeListThreshold(double freeListThreshold){
        String fakeResult = "{" +
                                 "\"" + "errCode" + "\"" + ":" + "0" +
                            "}";
        return fakeResult;
    }
    
    public static String getSystemProperties(){
        String fakeResult =  "<?xml version=\"1.0\"?>" +
                                "<?xml-stylesheet type=\"text/xsl\" href=\"configuration.xsl\"?>" +
                                "<configuration>" +
                                "    <property>" +
                                "        <name>lfsm_reinstall</name>" +
                                "        <value>1</value>" +
                                "    </property>" +
                                "    <property>" +
                                "        <name>lfsm_cn_ssds</name>" +
                                "        <value>8</value>" +
                                "        <setting>b-i</setting>" +
                                "    </property>" +
                                "    <property>" +
                                "        <name>cn_ssds_per_hpeu</name>" +
                                "        <value>4</value>" +
                                "    </property>" +
                                "    <property>" +
                                "        <name>lfsm_cn_pgroup</name>" +
                                "        <value>2</value>" +
                                "    </property>" +
                                "    <property>" +
                                "        <name>lfsm_io_thread</name>" +
                                "        <value>7</value>" +
                                "        <setting>-1</setting>" +
                                "    </property>" +
                                "    <property>" +
                                "        <name>lfsm_bh_thread</name>" +
                                "        <value>2</value>" +
                                "        <setting>-1</setting>" +
                                "    </property>" +
                                "    <property>" +
                                "        <name>lfsm_seu_reserved</name>" +
                                "        <value>250</value>" +
                                "    </property>" +
                                "    <property>" +
                                "        <name>lfsm_module_fn</name>" +
                                "        <value>lfsmdr</value>" +
                                "    </property>" +
                                "    <property>" +
                                "        <name>sofa_module_fn</name>" +
                                "        <value>sofa</value>" +
                                "    </property>" +
                                "    <property>" +
                                "        <name>sofa_deploy_dir</name>" +
                                "        <value>/usr/sofa/</value>" +
                                "    </property>" +
                                "    <property>" +
                                "        <name>lfsm_gc_chk_intvl</name>" +
                                "        <value>1000</value>" +
                                "    </property>" +
                                "    <property>" +
                                "        <name>lfsm_gc_off_upper_ratio</name>" +
                                "        <value>750</value>" +
                                "    </property>" +
                                "    <property>" +
                                "        <name>lfsm_gc_on_lower_ratio</name>" +
                                "        <value>250</value>" +
                                "    </property>" +
                                "</configuration>";
        return fakeResult;
    }
    
    
    
}
