<%--
  ~ Copyright (c) 2016 Google Inc. All Rights Reserved.
  ~
  ~ Licensed under the Apache License, Version 2.0 (the "License"); you
  ~ may not use this file except in compliance with the License. You may
  ~ obtain a copy of the License at
  ~
  ~     http://www.apache.org/licenses/LICENSE-2.0
  ~
  ~ Unless required by applicable law or agreed to in writing, software
  ~ distributed under the License is distributed on an "AS IS" BASIS,
  ~ WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
  ~ implied. See the License for the specific language governing
  ~ permissions and limitations under the License.
  --%>
<%@ page contentType="text/html;charset=UTF-8" language="java" %>
<%@ taglib prefix="fn" uri="http://java.sun.com/jsp/jstl/functions" %>
<%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core"%>


<html>
  <head>
    <title>Coverage Information</title>
    <link rel="icon" href="https://www.gstatic.com/images/branding/googleg/1x/googleg_standard_color_32dp.png" sizes="32x32">
    <link rel="stylesheet" href="https://fonts.googleapis.com/icon?family=Material+Icons">
    <link rel="stylesheet" href="https://fonts.googleapis.com/css?family=Roboto:100,300,400,500,700">
    <link rel="stylesheet" href="https://www.gstatic.com/external_hosted/materialize/all_styles-bundle.css">
    <link rel="stylesheet" href="/css/navbar.css">
    <link rel="stylesheet" href="/css/show_coverage.css">
    <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.1.0/jquery.min.js"></script>
    <script src="https://www.gstatic.com/external_hosted/materialize/materialize.min.js"></script>
    <nav id="navbar">
      <div class="nav-wrapper">
        <span>
          <a href="${pageContext.request.contextPath}/" class="breadcrumb">VTS Dashboard Home</a>
          <a href="${pageContext.request.contextPath}/show_table?testName=${testName}&startTime=${startTime}&endTime=${endTime}" class="breadcrumb">${testName}</a>
          <a href="#!" class="breadcrumb">Coverage</a>
        </span>
        <ul class='right'><li>
          <a id='dropdown-button' class='dropdown-button btn red lighten-3' href='#' data-activates='dropdown'>
            ${email}
          </a>
        </li></ul>
        <ul id='dropdown' class='dropdown-content'>
          <li><a href='${logoutURL}'>Log out</a></li>
        </ul>
      </div>
    </nav>
  </head>
  <body>
    <div class="container">
      <ul class="collapsible popout" data-collapsible="accordion">
        <c:forEach items="${coverageInfo}" var="entry">
          <li>
            <div class="collapsible-header">
              <i class="material-icons">library_books</i>
                ${entry[0]}
                <div class="right">${entry[1]}</div>
            </div>
            <div class="collapsible-body row">
              <div id="html_container" class="col s10 push-s1">${entry[2]}</div>
            </div>
          </li>
        </c:forEach>
      </ul>
    </div>
    <footer class="page-footer">
      <div class="footer-copyright">
        <div class="container">© 2016 - The Android Open Source Project
        </div>
      </div>
    </footer>
  </body>
</html>
