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

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 *
 * @author A40385
 */
public class SOFA {
    public String run(String commandCode) throws IOException{
        String result = "";
        Runtime rt = Runtime.getRuntime();
        String[] commands = {"/bin/sh", "-c", commandCode};
        Process proc = rt.exec(commands);
        try {
            int exitVal = proc.waitFor();
            BufferedReader stdInput = new BufferedReader(new 
                 InputStreamReader(proc.getInputStream()));

            BufferedReader stdError = new BufferedReader(new 
                 InputStreamReader(proc.getErrorStream()));

            // read the output from the command
            String s = null;
            while ((s = stdInput.readLine()) != null) {
                System.out.println(s);
                result += s;
            }
            stdInput.close();
            // read any errors from the attempted command
            while ((s = stdError.readLine()) != null) {
                System.out.println(s);
            }
            stdError.close();
        } catch (InterruptedException ex) {
            Logger.getLogger(SOFA.class.getName()).log(Level.SEVERE, null, ex);
        } finally{
            return result;
        }
            
    }
}
