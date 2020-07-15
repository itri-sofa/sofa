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
//********************** VARIABLES *************************
jLego.variables.isInit = false;
jLego.variables.resizableObjectList =[];

//********************** CONSTANTS *************************
jLego.constants.projectName = window.location.pathname.substr(1, window.location.pathname.lastIndexOf('/')-1);
jLego.constants.randomIDSize = 12;

//********************** URL *************************
jLego.url.rootUrl = '../' + jLego.constants.projectName;
jLego.url.baseUrl = '../' + jLego.constants.projectName +'/jLego';
jLego.url.baseImgUrl = '../' + jLego.constants.projectName +'/jLego/img';
jLego.url.baseImgUrlBlur = "_blur";
jLego.url.baseImgExtensionJPG = ".jpg";
jLego.url.baseImgExtensionPNG = ".png";
jLego.url.baseImgExtensionGIF = ".gif";