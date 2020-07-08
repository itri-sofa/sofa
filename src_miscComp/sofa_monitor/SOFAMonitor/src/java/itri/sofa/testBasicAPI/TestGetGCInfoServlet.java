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
@WebServlet(name = "TestGetGCInfoServlet", urlPatterns = {"/TestGetGCInfoServlet"})
public class TestGetGCInfoServlet extends HttpServlet{
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
            JSONObject diskSMARTData = sofaAPI.getGCInfo();
            returnJSON = diskSMARTData;
            returnJSON.put(Key.ERR_CODE, Status.ERR_CODE_OK);
            out.print(returnJSON.toString());
        } catch (JSONException ex) {
            Logger.getLogger(TestGetGCInfoServlet.class.getName()).log(Level.SEVERE, null, ex);
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
