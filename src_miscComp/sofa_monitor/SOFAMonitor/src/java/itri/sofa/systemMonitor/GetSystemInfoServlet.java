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
package itri.sofa.systemMonitor;

import itri.sofa.basicAPI.BasicAPI;
import itri.sofa.data.Key;
import itri.sofa.data.Status;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.servlet.ServletException;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.http.HttpSession;
import org.codehaus.jettison.json.JSONArray;
import org.codehaus.jettison.json.JSONException;
import org.codehaus.jettison.json.JSONObject;

/**
 *
 * @author A40385
 */
@WebServlet(name = "GetSystemInfoServlet", urlPatterns = {"/GetSystemInfoServlet"})
public class GetSystemInfoServlet extends HttpServlet{
    protected void processRequest(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
        HttpSession session = request.getSession();        
        response.setContentType("application/json;charset=UTF-8");        
        PrintWriter out = response.getWriter();        
        boolean fakeDataMode = false;
        String fakeDataModeString = getServletContext().getInitParameter("fakeDataMode");
        if(fakeDataModeString!=null && fakeDataModeString.toLowerCase().matches("true"))
            fakeDataMode = true;
        BasicAPI sofaAPI = new BasicAPI(fakeDataMode);
        JSONObject returnJSON = new JSONObject();   
        try {
            long totalUsedSpace = 0;
            //Group data
            JSONObject groupData = sofaAPI.getGroups();
            boolean systemStatusIsOK = true;
            System.out.println(groupData.toString());
            int groupCount = (int) groupData.get(Key.GROUP_NUMBER);
            JSONArray resultGroupJSON = new JSONArray();
            JSONArray groups;
            if(!groupData.has(Key.GROUPS)){
                groups = new JSONArray();
                for(int i=0; i<groupCount; i++){
                    JSONObject groupDetail = new JSONObject();
                    groupDetail.put(Key.CAPACITY, 0);
                    groupDetail.put(Key.PHYSICAL_CAPACITY, 0);
                    groupDetail.put(Key.USED_SPACE, 0);
                    groups.put(groupDetail);
                }
            }
            else{
                groups = groupData.getJSONArray(Key.GROUPS);
            }
            int failGroupCount = 0;
            for(int i=0; i<groups.length(); i++){
                JSONObject currentGroup = groups.getJSONObject(i);
                JSONObject groupDiskData = sofaAPI.getGroupDisks(i);
                //Disk data
                JSONArray disks = groupDiskData.getJSONArray(Key.DISKS);
                boolean statusIsOK = true;
                int failDiskCount = 0;
                for(int j=0; j<disks.length(); j++){
                    JSONObject currentDisk = disks.getJSONObject(j);
                    if(currentDisk.getString(Key.STATUS).equals(Status.NORMAL)) currentDisk.put(Key.STATUS, Status.OK);
                    //else if(currentDisk.getString(Key.STATUS).equals(Status.REMOVED)) currentDisk.put(Key.STATUS, Status.REMOVED);
                    //else if(currentDisk.getString(Key.STATUS).equals(Status.FAILED)) currentDisk.put(Key.STATUS, Status.FAILED);
                    //else if(currentDisk.getString(Key.STATUS).equals(Status.REBUILDING)) currentDisk.put(Key.STATUS, Status.REBUILDING);
                    //else if(currentDisk.getString(Key.STATUS).equals(Status.READ_ONLY)) currentDisk.put(Key.STATUS, Status.READ_ONLY);
                    //else;
                    if(!currentDisk.getString(Key.STATUS).equals(Status.OK) && !currentDisk.getString(Key.STATUS).equals(Status.READ_ONLY)){
                        statusIsOK = false;
                        systemStatusIsOK = false;
                        failDiskCount++;
                    }
                }
                currentGroup.put(Key.DISKS, disks);
                currentGroup.put(Key.PHYSICAL_CAPACITY, groupDiskData.get(Key.PHYSICAL_CAPACITY));
                currentGroup.put(Key.CAPACITY, groupDiskData.get(Key.CAPACITY));
                currentGroup.put(Key.USED_SPACE, groupDiskData.get(Key.USED_SPACE));
                totalUsedSpace += currentGroup.getLong(Key.USED_SPACE);
                if(disks.length()==0) currentGroup.put(Key.STATUS, Status.REMOVED);
                else if(statusIsOK) currentGroup.put(Key.STATUS, Status.OK);
                else if(failDiskCount == disks.length()){
                     currentGroup.put(Key.STATUS, Status.FAILED);
                     failGroupCount++;
                }
                else currentGroup.put(Key.STATUS, Status.READ_ONLY);
                resultGroupJSON.put(currentGroup);
            }
            returnJSON.put(Key.GROUPS, resultGroupJSON);
            //Status
            if(systemStatusIsOK) returnJSON.put(Key.STATUS, Status.OK);
            else if(failGroupCount == groups.length()) returnJSON.put(Key.STATUS, Status.FAILED);
            else returnJSON.put(Key.STATUS, Status.DEGRADED);
            //Spared disk
            JSONObject sparedDiskData = sofaAPI.getSparedDisks();
            JSONArray sparedDisks = sparedDiskData.getJSONArray(Key.DISKS);
            returnJSON.put(Key.SPARED, sparedDisks);
            //Spared Status
            boolean statusIsOK = true;
            int failSpareDiskCount = 0;
            for(int i=0; i<sparedDisks.length(); i++){
                JSONObject currentDisk = sparedDisks.getJSONObject(i);
                if(currentDisk.getString(Key.STATUS).equals(Status.NORMAL)) currentDisk.put(Key.STATUS, Status.OK);
                //else if(currentDisk.getString(Key.STATUS).equals(Status.REMOVED)) currentDisk.put(Key.STATUS, Status.REMOVED);
                //else if(currentDisk.getString(Key.STATUS).equals(Status.FAILED)) currentDisk.put(Key.STATUS, Status.FAILED);
                //else if(currentDisk.getString(Key.STATUS).equals(Status.REBUILDING)) currentDisk.put(Key.STATUS, Status.REBUILDING);
                //else if(currentDisk.getString(Key.STATUS).equals(Status.READ_ONLY)) currentDisk.put(Key.STATUS, Status.READ_ONLY);
                //else;
                if(!currentDisk.getString(Key.STATUS).equals(Status.OK)){
                    statusIsOK = false;
                    failSpareDiskCount++;
                }
            }
            if(sparedDisks.length()==0) returnJSON.put(Key.SPARED_STATUS, Status.REMOVED);
            else if(statusIsOK) returnJSON.put(Key.SPARED_STATUS, Status.OK);
            else if(failSpareDiskCount == sparedDisks.length()){
                     returnJSON.put(Key.SPARED_STATUS, Status.FAILED);
            }
            else returnJSON.put(Key.SPARED_STATUS, Status.DEGRADED);
            //Physical Capacity data
            JSONObject physicalCapacityData = sofaAPI.getPhysicalCapacity();
            long physicalCapacity = physicalCapacityData.getLong(Key.PHYSICAL_CAPACITY);
            returnJSON.put(Key.PHYSICAL_CAPACITY, physicalCapacity);
            //Capacity data
            JSONObject capacityData = sofaAPI.getCapacity();
            long capacity = capacityData.getLong(Key.CAPACITY);
            returnJSON.put(Key.CAPACITY, capacity);
            //GC Info
            JSONObject gcData = sofaAPI.getGCInfo();
            String reservedRate = gcData.getString(Key.RESERVED);
            returnJSON.put(Key.RESERVED_RATE, Float.valueOf(reservedRate) / 100);
            //Used Space
            returnJSON.put(Key.USED_SPACE, totalUsedSpace);
            //Error Code
            returnJSON.put(Key.ERR_CODE, Status.ERR_CODE_OK);
            out.print(returnJSON.toString());
        } catch (JSONException ex) {
            Logger.getLogger(GetSystemInfoServlet.class.getName()).log(Level.SEVERE, null, ex);
        } finally {
            out.close();
        }  
        
    }
    
    /**
     * Handles the HTTP <code>GET</code> method.
     *
     * @param request servlet request
     * @param response servlet response
     * @throws ServletException if a servlet-specific error occurs
     * @throws IOException if an I/O error occurs
     */
    @Override
    protected void doGet(HttpServletRequest request, HttpServletResponse response)
            throws ServletException, IOException {
        processRequest(request, response);
    }

    /**
     * Handles the HTTP <code>POST</code> method.
     *
     * @param request servlet request
     * @param response servlet response
     * @throws ServletException if a servlet-specific error occurs
     * @throws IOException if an I/O error occurs
     */
    @Override
    protected void doPost(HttpServletRequest request, HttpServletResponse response)
            throws ServletException, IOException {
        processRequest(request, response);
    }

    /**
     * Returns a short description of the servlet.
     *
     * @return a String containing servlet description
     */
    @Override
    public String getServletInfo() {
        return "Short description";
    }// </editor-fold>
}
