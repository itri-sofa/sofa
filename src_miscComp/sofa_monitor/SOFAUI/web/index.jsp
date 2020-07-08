<%-- 
    Document   : index
    Created on : 2016/8/1, 上午 10:59:19
    Author     : A40385
--%>
<%@page import="org.itri.jLego.JLego"%>
<%@page import="org.itri.webPage.WebPage"%>
<%@page contentType="text/html" pageEncoding="UTF-8"%>
<!DOCTYPE html>
<html>
    <head>
        <meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
        <link rel="icon" type="image/png" href="webPage/img/favicon/favicon-32x32.png">
        <title>SOFA UI</title>
        <!--Import jLego-->
        <link rel="stylesheet" href="jLego/css/jLego.css" type="text/css"/>
        <script type="text/javascript" src="jLego/js/portal.js"></script>
        <!--Import webPage-->
        <script type="text/javascript" src="webPage/js/portal.js"></script>
        <!--Get Input Argument (Language) -->
        <script>
            var tempSystemLanguage = "en_US";
            $.extend({
                    getUrlVars: function(){
                      var vars = [], hash;
                      var hashes = window.location.href.slice(window.location.href.indexOf('?') + 1).split('&');
                      for(var i = 0; i < hashes.length; i++)
                      {
                        hash = hashes[i].split('=');
                        vars.push(hash[0]);
                        vars[hash[0]] = hash[1];
                      }
                      return vars;
                    },
                    getUrlVar: function(name){
                      return $.getUrlVars()[name];
                    }
            });
            if ($.getUrlVar("lang")!=null){
                var langString = $.getUrlVar("lang");
                langString=langString.replace(/%27/g, "");
                langString=langString.replace(/%22/g, "");
                tempSystemLanguage=langString.toString();
            }
            else{
                var cookieLanguage = jLego.func.readCookie("sofaLanguage");
                if(cookieLanguage != null){
                    tempSystemLanguage = cookieLanguage;
                }
                else{
                    var userLang = navigator.language || navigator.userLanguage; 
                    if(userLang=='zh-TW') tempSystemLanguage='zh_TW';
                    else if(userLang=='ja-JP' || userLang=='ja') tempSystemLanguage='ja_JP';
                    else tempSystemLanguage='en_US';
                }
                 //tempSystemLanguage='en_US';
            }  
            webView.setLanguage(tempSystemLanguage);
            jLego.setLanguage(tempSystemLanguage);
        </script>
        <!--Mixin jLego objectUI-->
        <%new JLego().mixin(out, new String[]{"nowLoading", 
                                              "nodeTable",
                                              "nodeCard",
                                              "tabPage",  
                                              "tabPageOrg", 
                                              "statusMonitor", 
                                              "stepPage", 
                                              "buttonCTRLer", 
                                              "popoutPanel", 
                                              "optionCTRLer", 
                                              "optionListCTRLer", 
                                              "searchCTRLer", 
                                              "levelPanel",
                                              "toastCTRLer"});%>
        <!--Mixin Pages-->
        <%new WebPage().mixin(out, new String[]{"common",
                                                "mainPage",
                                                "dashboardPage",
                                                "performancePage",
                                                "settingPage",
                                                "performancePage",
                                                "popupPage"});%>
    </head>
    <body>
        <script>
            webView.mainPage.add(document.body);
            webView.mainPage.resizeHandler();
        </script>
    </body>
</html>
