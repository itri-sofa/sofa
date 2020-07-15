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
package itri.sofa.testBasicAPI;

import itri.sofa.systemMonitor.*;
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
@WebServlet(name = "TestGetDiskSMARTServlet", urlPatterns = {"/TestGetDiskSMARTServlet"})
public class TestGetDiskSMARTServlet extends HttpServlet{
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
        String diskUUID = request.getParameter(Key.DISK_UUID); 
        try {
            //Physical Capacity data
            JSONObject diskSMARTData = sofaAPI.getDiskSMART(diskUUID);
            returnJSON = new JSONObject();
            returnJSON.put(Key.ERR_CODE, Status.ERR_CODE_OK);
            JSONArray attributeArray = new JSONArray();
            for(int i=0; i<diskSMARTData.getJSONArray("SMART").length(); i++){
                String currentInfo = diskSMARTData.getJSONArray("SMART").getString(i);
                String value = "";
                if(currentInfo.contains("Device Model:")){
                    value = currentInfo.substring(currentInfo.indexOf(":")+1, currentInfo.length());
                    value = value.replaceAll("\\s+", " ");
                    value = value.trim();
                    returnJSON.put(Key.DEVICE_MODEL, value);
                }
                else if(currentInfo.contains("Serial Number:")){
                    value = currentInfo.substring(currentInfo.indexOf(":")+1, currentInfo.length());
                    value = value.replaceAll("\\s+", " ");
                    value = value.trim();
                    returnJSON.put(Key.SERIAL_NUMBER, value);
                }
                else if(currentInfo.contains("Firmware Version:")){
                    value = currentInfo.substring(currentInfo.indexOf(":")+1, currentInfo.length());
                    value = value.replaceAll("\\s+", " ");
                    value = value.trim();
                    returnJSON.put(Key.FIRMWARE_VERSION, value);
                }
                else if(currentInfo.contains("User Capacity:")){
                    value = currentInfo.substring(currentInfo.indexOf(":")+1, currentInfo.length());
                    value = value.replaceAll("\\s+", " ").replace("bytes", "").replace(",", "");
                    value = value.trim();
                    returnJSON.put(Key.USER_CAPACITY, value);
                }
                else if(currentInfo.contains("Device is:")){
                    value = currentInfo.substring(currentInfo.indexOf(":")+1, currentInfo.length());
                    value = value.replaceAll("\\s+", " ");
                    value = value.trim();
                    returnJSON.put(Key.DEVICE, value);
                }
                else if(currentInfo.contains("ATA Version is:")){
                    value = currentInfo.substring(currentInfo.indexOf(":")+1, currentInfo.length());
                    value = value.replaceAll("\\s+", " ");
                    value = value.trim();
                    returnJSON.put(Key.ATA_VERSION, value);
                }
                else if(currentInfo.contains("ATA Standard is:")){
                    value = currentInfo.substring(currentInfo.indexOf(":")+1, currentInfo.length());
                    value = value.replaceAll("\\s+", " ");
                    value = value.trim();
                    returnJSON.put(Key.ATA_STANDARD, value);
                }
                else if(currentInfo.contains("Local Time is:")){
                    value = currentInfo.substring(currentInfo.indexOf(":")+1, currentInfo.length());
                    value = value.replaceAll("\\s+", " ");
                    value = value.trim();
                    returnJSON.put(Key.LOCAL_TIME, value);
                }
                else if(currentInfo.contains("SMART support is:")){
                    value = currentInfo.substring(currentInfo.indexOf(":")+1, currentInfo.length());
                    value = value.replaceAll("\\s+", " ");
                    value = value.trim();
                    returnJSON.put(Key.SMART_SUPPORT, value);
                }
                else if(currentInfo.contains("SMART overall-health self-assessment test result:")){
                    value = currentInfo.substring(currentInfo.indexOf(":")+1, currentInfo.length());
                    value = value.replaceAll("\\s+", " ");
                    value = value.trim();
                    returnJSON.put(Key.SMART_HEALTH, value);
                }
                else if(currentInfo.contains("Please note the following marginal Attributes:")){
                    
                }
                else if(currentInfo.contains("ATTRIBUTE_NAME")){
                    
                }
                else{
                    try{
                        String attrID = currentInfo.substring(0, 3);
                        String attrName = currentInfo.substring(3, 27);
                        String attrFlag = currentInfo.substring(27, 36);
                        String attrValue = currentInfo.substring(36, 42);
                        String attrWorst = currentInfo.substring(42, 48);
                        String attrThreshold = currentInfo.substring(48, 55);
                        String attrType = currentInfo.substring(55, 65);
                        String attrUpdated = currentInfo.substring(65, 74);
                        String attrWhenFailed = currentInfo.substring(74, 86);
                        String attrRawValue = currentInfo.substring(86, currentInfo.length());
                        JSONObject attrObject = new JSONObject();
                        attrObject.put(Key.ATTR_ID, attrID.replaceAll("\\s+", " ").trim());
                        attrObject.put(Key.ATTR_NAME, attrName.replaceAll("\\s+", " ").trim());
                        attrObject.put(Key.ATTR_FLAG, attrFlag.replaceAll("\\s+", " ").trim());
                        attrObject.put(Key.ATTR_VALUE, attrValue.replaceAll("\\s+", " ").trim());
                        attrObject.put(Key.ATTR_WORST, attrWorst.replaceAll("\\s+", " ").trim());
                        attrObject.put(Key.ATTR_THRESHOLD, attrThreshold.replaceAll("\\s+", " ").trim());
                        attrObject.put(Key.ATTR_TYPE, attrType.replaceAll("\\s+", " ").trim());
                        attrObject.put(Key.ATTR_UPDATED, attrUpdated.replaceAll("\\s+", " ").trim());
                        attrObject.put(Key.ATTR_WHEN_FAILED, attrWhenFailed.replaceAll("\\s+", " ").trim());
                        attrObject.put(Key.ATTR_RAW_VALUE, attrRawValue.replaceAll("\\s+", " ").trim());
                        attributeArray.put(attrObject);
                    }catch(Exception e){
                        
                    }
                }
            }
            returnJSON.put(Key.ATTRIBUTES, attributeArray);
            out.print(returnJSON.toString());
        } catch (JSONException ex) {
            Logger.getLogger(GetDiskSMARTServlet.class.getName()).log(Level.SEVERE, null, ex);
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
