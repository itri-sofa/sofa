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

import itri.sofa.testBasicAPI.*;
import itri.sofa.systemMonitor.*;
import itri.sofa.basicAPI.BasicAPI;
import itri.sofa.data.Key;
import itri.sofa.data.Status;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringReader;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.servlet.ServletException;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.http.HttpSession;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import org.codehaus.jettison.json.JSONArray;
import org.codehaus.jettison.json.JSONException;
import org.codehaus.jettison.json.JSONObject;
import org.w3c.dom.Document;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;

/**
 *
 * @author A40385
 */
@WebServlet(name = "GetSystemPropertiesServlet", urlPatterns = {"/GetSystemPropertiesServlet"})
public class GetSystemPropertiesServlet extends HttpServlet{
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
            //Physical Capacity data
            String xmlString = sofaAPI.getSystemProperties();
            DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();  
            DocumentBuilder builder;  
            try {  
                builder = factory.newDocumentBuilder();
                InputSource inputSource = new InputSource(new StringReader(xmlString));
                Document document = builder.parse(inputSource);
                NodeList nodeList = document.getElementsByTagName("property");
                String maxGCRatio="1000", minGCRatio="0", gcReserve="", startGCRatio="", stopGCRatio="", iopsThreshold = "", freeSpaceThreshold = "";
                for(int i=0; i<nodeList.getLength(); i++){
                    NodeList subNodeList  = nodeList.item(i).getChildNodes();
                    String targetName = null;
                    String targetValue = null;
                    for(int j=0; j<subNodeList.getLength(); j++){
                        if(subNodeList.item(j).getNodeName().matches("name")){
                            targetName = subNodeList.item(j).getTextContent();
                        }
                        else if(subNodeList.item(j).getNodeName().matches("value")){
                            targetValue = subNodeList.item(j).getTextContent();
                        }
                    }
                    if(targetValue != null && targetName != null){
                        if(targetName.matches("lfsm_gc_off_upper_ratio")) stopGCRatio = targetValue;
                        else if(targetName.matches("lfsm_gc_on_lower_ratio")) startGCRatio = targetValue;
                        else if(targetName.matches("lfsm_seu_reserved")) gcReserve = targetValue;
                    }
                    
                }
                // Get warnning threshold value
                iopsThreshold = sofaAPI.getIOPSThreshold();
                freeSpaceThreshold = sofaAPI.getFreeListThreshold();
                
                JSONObject gcObject = new JSONObject();
                gcObject.put(Key.MAX_RATIO, maxGCRatio);
                gcObject.put(Key.MIN_RATIO, minGCRatio);
                gcObject.put(Key.START_RATIO, startGCRatio);
                gcObject.put(Key.STOP_RATIO, stopGCRatio);
                gcObject.put(Key.RESERVE, gcReserve);
                returnJSON.put(Key.GC, gcObject);
                JSONObject iopsObject = new JSONObject();
                iopsObject.put(Key.THRESHOLD, iopsThreshold);
                returnJSON.put(Key.IOPS, iopsObject);
                JSONObject freeListObject = new JSONObject();
                freeListObject.put(Key.THRESHOLD, freeSpaceThreshold);
                returnJSON.put(Key.FREELIST, freeListObject);
                returnJSON.put(Key.ERR_CODE, Status.ERR_CODE_OK);
            } catch (ParserConfigurationException ex) {
                Logger.getLogger(GetSystemPropertiesServlet.class.getName()).log(Level.SEVERE, null, ex);
                returnJSON.put(Key.ERR_CODE, Status.ERR_CODE_FAIL);
            } catch (SAXException ex) {
                Logger.getLogger(GetSystemPropertiesServlet.class.getName()).log(Level.SEVERE, null, ex);
                returnJSON.put(Key.ERR_CODE, Status.ERR_CODE_FAIL);
            }
            out.print(returnJSON.toString());
        } catch (JSONException ex) {
            Logger.getLogger(GetSystemPropertiesServlet.class.getName()).log(Level.SEVERE, null, ex);
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
