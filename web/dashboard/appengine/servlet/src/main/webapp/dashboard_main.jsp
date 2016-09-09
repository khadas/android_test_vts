<%-- //[START all]--%>
<%@ page contentType="text/html;charset=UTF-8" language="java" %>
<%@ taglib prefix="fn" uri="http://java.sun.com/jsp/jstl/functions" %>
<%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core"%>


<html>
  <link rel="icon" href="https://www.gstatic.com/images/branding/googleg/1x/googleg_standard_color_32dp.png" sizes="32x32">
  <link rel="stylesheet" href="https://fonts.googleapis.com/icon?family=Material+Icons">
  <link rel="stylesheet" href="https://fonts.googleapis.com/css?family=Roboto:100,300,400,500,700">
  <link rel="stylesheet" href="https://www.gstatic.com/external_hosted/materialize/all_styles-bundle.css">
  <link rel="stylesheet" href="/css/navbar.css">
  <link rel="stylesheet" href="/css/dashboard_main.css">
  <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.1.0/jquery.min.js"></script>
  <script src="https://www.gstatic.com/external_hosted/materialize/materialize.min.js"></script>
  <head>
    <title>VTS Dashboard</title>

    <nav id="navbar">
      <div class="nav-wrapper">
        <div class="col s12">
          <a href="#!" class="breadcrumb">VTS Dashboard Home</a>
        </div>
      </div>
    </nav>
  </head>

  <body>
    <div class="container">
      <div class="row" id="options">
        <c:forEach items="${testNames}" var="test">
          <a href="${pageContext.request.contextPath}/show_table?testName=${test}">
            <div class="col s12 card hoverable option valign-wrapper waves-effect">
              <span class="entry valign">
                ${test}
              </span>
            </div>
          </a>
        </c:forEach>
      </div>
    </div>
    <footer class="page-footer">
      <div class="footer-copyright">
        <div class="container">
          © 2016 - The Android Open Source Project
        </div>
      </div>
    </footer>
  </body>
</html>
<%-- //[END all]--%>
