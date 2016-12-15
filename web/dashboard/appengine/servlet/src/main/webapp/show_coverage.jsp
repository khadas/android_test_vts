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
        <div class="col s12">
          <a href="${pageContext.request.contextPath}/" class="breadcrumb">VTS Dashboard Home</a>
          <a href="${pageContext.request.contextPath}/show_table?testName=${testName}&buildIdStartTime=${buildIdStartTime}&buildIdEndTime=${buildIdEndTime}" class="breadcrumb">${testName}</a>
          <a href="#!" class="breadcrumb">Coverage</a>
        </div>
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
